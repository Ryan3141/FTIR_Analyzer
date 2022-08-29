#include "Lifetime_Interactive_Graph.h"

#include "Optimize.h"

namespace Lifetime
{

void Axes::Set_X_Units( X_Units units )
{
	if( units == x_units )
		return;

	x_units = units;

	graph_function();
}

void Axes::Set_Y_Units( Y_Units units )
{
	if( units == y_units )
		return;

	y_units = units;

	graph_function();
}

Prepared_Data Axes::Prepare_XY_Data( const Single_Graph & graph_data ) const
{
	arma::vec x = arma::conv_to<arma::vec>::from( graph_data.x_data.toStdVector() );
	arma::vec y = arma::conv_to<arma::vec>::from( graph_data.y_data.toStdVector() );
	//if( x_units == X_Units::TEMPERATURE_K )
	//	return { toQVec( 1E6 * x ), graph_data.y_data };

	if( graph_data.x_units == X_Units::DONT_CHANGE )
		return { toQVec( 1E6 * x ), graph_data.y_data };

	double x_offset = x( y.index_max() ); // Align all of the peaks
	x = 1E6 * (x - x_offset);
	y -= y.min();
	return { toQVec( x ), toQVec( y ) };
}

void Axes::Graph_XY_Data( QString measurement_name, const Single_Graph & graph )
{
	auto[ x_data, y_data ] = this->Prepare_XY_Data( graph );
	graph.graph_pointer->setData( x_data, y_data );
}

Interactive_Graph::Interactive_Graph( QWidget* parent ) :
	Graph_Base( parent )
{
	this->x_axis_menu_functions.emplace_back( [ this ]( QMenu* menu, QPoint pos ) mutable
	{
		for( size_t index = 0; index < sizeof( Axes::X_Unit_Names ) / sizeof( Axes::X_Unit_Names[ 0 ] ); index++ )
		{
			if( X_Units( index ) == this->axes.x_units )
				continue;
			menu->addAction( Axes::Change_To_X_Unit_Names[ index ], [ index, this ]
			{
				Change_Axes( index );
			} );
		}
	} );
	this->y_axis_menu_functions.emplace_back( [ this ]( QMenu* menu, QPoint pos ) mutable
	{
		for( size_t index = 0; index < sizeof( Axes::Y_Unit_Names ) / sizeof( Axes::Y_Unit_Names[ 0 ] ); index++ )
		{
			if( Y_Units( index ) == this->axes.y_units )
				continue;
			menu->addAction( Axes::Change_To_Y_Unit_Names[ index ], [ index, this ]
			{
				Change_Axes( index );
			} );
		}
	} );

	this->general_menu_functions.emplace_back( [ this ]( QMenu* menu, QPoint pos ) mutable
	{
		menu->addAction( "Paste From Clipboard", [ this ]
		{
			const QClipboard *clipboard = QApplication::clipboard();
			const QMimeData *mimeData = clipboard->mimeData();

			if( mimeData->hasText() )
			{
				auto[ x_data, y_data ] = Load_XY_CSV_Data( mimeData->text().toStdString(), ",\t" );
				this->Graph<X_Units::TIME_US, Y_Units::DONT_CHANGE>( toQVec( x_data ),
																	toQVec( y_data ), "Clipboard Data", "Clipboard Data" );
				this->replot();
			}
		} );
	} );
}

struct Fit_Results {
	double amplitude;
	double x_offset;
	double lifetime;
};

Fit_Results Fit_Lifetime( const QVector<double> & x_data, const QVector<double> & y_data )
{
	if( x_data.empty() || y_data.empty() )
		return { arma::datum::nan, arma::datum::nan, arma::datum::nan };
	arma::vec x = arma::conv_to<arma::vec>::from( x_data.toStdVector() );
	arma::vec y = arma::conv_to<arma::vec>::from( y_data.toStdVector() );

	double x_offset = x( y.index_max() ); // Align all of the peaks
	x = x - x_offset;
	auto selection_region = arma::find( x > 0.2E-6 && x < 5.0E-6 );
	//arma::vec q = x.elem(  );
	auto exponential_function = []( const arma::vec & fit_params, const arma::vec & time ) -> arma::vec
	{
		double amplitude = fit_params[ 0 ];
		double offset    = fit_params[ 1 ];
		double tau       = fit_params[ 2 ];
		return amplitude * arma::exp( -( time - offset ) / tau );
	};

	arma::vec fit_params = Fit_Data_To_Function( exponential_function, x( selection_region ), y( selection_region ), { 1E-3, -20E-6, 500E-9 }, { 100, +20E-6, 100E-6 } );
	return { fit_params[ 0 ], fit_params[ 1 ] ,fit_params[ 2 ] };
}

void Interactive_Graph::Change_Axes( int index )
{
	X_Units x_units = X_Units( index );
	Y_Units y_units = Y_Units( index );
	//std::array<double, 2> bounds = { xAxis->range().lower, xAxis->range().upper };
	//std::array<double, 2> original = { xAxis->range().lower, xAxis->range().upper };

	if( x_units == X_Units::TEMPERATURE_K )
	{
	}
	else if( x_units == X_Units::TIME_US )
	{
		for( auto &[ name, graph ] : remembered_graphs )
			if( graph.x_units == X_Units::TIME_US )
				this->Hide_Graph( name, false );
		this->Hide_Graph( "Vs_Temperature", true );
	}
	else if( x_units == X_Units::FIT_TIME_US )
	{
		//for( auto & [name, graph] : remembered_graphs )
		//	if( graph.x_units == X_Units::TIME_US )
		//		this->Hide_Graph( name, true );



		const auto relevant_graphs = remembered_graphs % fn::where( [ this ]( const auto & x ) { const auto &[ name, graph ] = x; return graph.x_units == X_Units::TIME_US; } );
		QVector<double> temperatures = relevant_graphs % fn::transform( [ this ]( const auto & x )
		{
			const auto &[ name, graph ] = x;
			auto y = graph.meta.find( "temperature_in_k" );
			if( y == graph.meta.end() )
				return 0.0;
			else
				return y->second.toDouble(); } ) % fn::to( QVector<double>{} );
			QVector<double> lifetimes = relevant_graphs % fn::transform( [ this ]( const auto & x )
			{
				const auto &[ name, graph ] = x;
				auto[ amplitude, x_offset, lifetime ] = Fit_Lifetime( graph.x_data, graph.y_data );
				return lifetime;
			} ) % fn::to( QVector<double>{} );

			auto exponential_function = []( const arma::vec & fit_params, const arma::vec & time ) -> arma::vec
			{
				double amplitude = fit_params[ 0 ];
				double offset = fit_params[ 1 ];
				double tau = fit_params[ 2 ];
				return amplitude * arma::exp( -( time - offset ) / tau );
			};
			auto & graph = relevant_graphs.begin()->second;
			arma::vec x = arma::conv_to<arma::vec>::from( graph.x_data.toStdVector() );
			auto[ amplitude, x_offset, lifetime ] = Fit_Lifetime( graph.x_data, graph.y_data );
			auto selection_region = arma::find( x > 0.2E-6 && x < 5.0E-6 );

			//Graph<X_Units::TEMPERATURE_K, Y_Units::TIME_US>( toQVec( x( selection_region ) ), toQVec( exponential_function( { amplitude, x_offset, lifetime }, x )( selection_region ) ), "Vs_Temperature", "Lifetime vs Temperature", {} );
			Graph<X_Units::DONT_CHANGE, Y_Units::DONT_CHANGE>( toQVec( x( selection_region ) ), toQVec( exponential_function( { amplitude, x_offset, lifetime }, x( selection_region ) ) ), "Vs_Temperature", "Lifetime vs Temperature", {} );
			//Graph<X_Units::TEMPERATURE_K, Y_Units::TIME_US>( temperatures, temperatures, "Vs_Temperature", "Lifetime vs Temperature", {} );
	}


	//for( double & x : bounds )
	//	x = Convert_Units( this->axes.x_units, x_units, x );

	//if( original[ 0 ] < 0 && bounds[ 0 ] > 0 )
	//	original[ 0 ] = original[ 0 ];
	//if( bounds[ 0 ] < 0 && std::abs( bounds[ 0 ] ) > bounds[ 1 ] )
	//	bounds[ 0 ] = Axes::X_Unit_Sensible_Maximum[ int( x_units ) ];

	//xAxis->setRange( std::min( bounds[ 0 ], bounds[ 1 ] ), std::max( bounds[ 0 ], bounds[ 1 ] ) );
	//yAxis->setRange( 0.0, 100.0 );

	this->xAxis->setLabel( Axes::X_Unit_Names[ int( x_units ) ] );
	this->yAxis->setLabel( Axes::Y_Unit_Names[ int( y_units ) ] );

	this->axes.Set_X_Units( x_units );
	this->axes.Set_Y_Units( y_units );
	emit X_Units_Changed();
	emit Y_Units_Changed();
}

const QString Axes::X_Unit_Names[ 3 ] = { "Time (" + QString( QChar( 0x03BC ) ) + "s)",
										  "Temperature (K)",
										  "Time (" + QString( QChar( 0x03BC ) ) + "s)", };
											//"1/Temperature (K" + QString( QChar( 0x207B ) ) + QString( QChar( 0x00B9 ) ) + ")" };

const QString Axes::Y_Unit_Names[ 3 ] = { "Voltage (V)",
										  "Lifetime (" + QString( QChar( 0x03BC ) ) + "s)",
										  "Voltage (V)", };

const QString Axes::Change_To_X_Unit_Names[ 3 ] = { "Change to Time",
													"Change to Temperature",
													"Change to Fit", };

const QString Axes::Change_To_Y_Unit_Names[ 3 ] = { "Change to Voltage",
													"Change to Lifetime",
													"Change to Fit", };


}