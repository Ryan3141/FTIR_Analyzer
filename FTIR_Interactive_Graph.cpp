#include "FTIR_Interactive_Graph.h"

namespace FTIR
{

void Axes_Scales::Set_X_Units( FTIR::X_Units units )
{
	if( units == x_units )
		return;

	x_units = units;

	graph_function();
}

void Axes_Scales::Set_Y_Units( FTIR::Y_Units units )
{
	if( units == y_units )
		return;

	y_units = units;

	graph_function();
}

void Axes_Scales::Set_As_Background( XY_Data xy )
{
	background_x_data = std::get<0>( xy );
	background_y_data = std::get<1>( xy );

	graph_function();
}

Prepared_Data Axes_Scales::Prepare_XY_Data( const Single_Graph & graph_data ) const
{
	const QVector<double> x_data = [ &graph_data, this ] {
		if( this->x_units == graph_data.x_units )
			return graph_data.x_data;

		QVector<double> x_data = graph_data.x_data;
		for( double & x : x_data )
			x = Convert_Units( graph_data.x_units, this->x_units, x );
		return x_data;
	}( );

	const QVector<double> y_data = [ &graph_data, this ] {
		if( this->y_units == graph_data.y_units || graph_data.y_units == Y_Units::DONT_CHANGE )
			return graph_data.y_data;

		if( graph_data.y_units == Y_Units::RAW_SENSOR )
		{
			if( background_y_data.size() == 0 )
				return graph_data.y_data;

			QVector<double> transmission_data( graph_data.y_data.size() );
			//auto y_data_min_element = std::min_element( y_data.constBegin(), y_data.constEnd(), []( double a, double b ) { return std::abs( a ) < std::abs( b ); } );
			//double y_data_min = std::min( 0.0004, std::abs( *y_data_min_element ) );
			//auto background_min_element = std::min_element( y_data.constBegin(), y_data.constEnd(), []( double a, double b ) { return std::abs( a ) < std::abs( b ); } );
			//double background_min = std::min( 0.0004, std::abs( *background_min_element ) );

			auto cleanup_low_resolution_input = [ & ]( double y )
			{
				const double minimum_reliable_reading = 0.003;
				if( std::abs( y ) < minimum_reliable_reading )
					return 0.0;
				//return std::pow( std::abs( ( std::abs( y ) - minimum_reliable_reading ) / y ), 1 );
				return 1.0;
			};
			for( auto[ transmission, x, y ] : fn::zip( transmission_data, graph_data.x_data, graph_data.y_data ) )
			{
				int i = std::distance( background_x_data.begin(), std::lower_bound( background_x_data.begin(), background_x_data.end(), x ) );
				i = std::min( background_y_data.size() - 1, i );
				transmission = 100 * y / background_y_data[ i ] * cleanup_low_resolution_input( y ) * cleanup_low_resolution_input( background_y_data[ i ] );
			}

			if( this->y_units == Y_Units::ABSORPTION )
			{
				arma::cx_vec ft = arma::fft( arma::conv_to<arma::vec>::from( graph_data.y_data.toStdVector() ) );
				return toQVec( arma::imag( ft ) );
				double max_transmission = *std::max_element( transmission_data.constBegin(), transmission_data.constEnd() );
				QVector<double> & absorption_data = transmission_data;
				for( double & y : absorption_data )
					y = max_transmission - y;
				return absorption_data;
			}
			else
				return transmission_data;
		}

		if( ( this->y_units == Y_Units::TRANSMISSION && graph_data.y_units == Y_Units::ABSORPTION ) ||
			( this->y_units == Y_Units::ABSORPTION   && graph_data.y_units == Y_Units::TRANSMISSION ) )
		{
			QVector<double> y_data = graph_data.y_data;
			for( double & y : y_data )
				y = 100.0 - y;
			return y_data;
		}

		if( ( this->y_units == Y_Units::RAW_SENSOR && graph_data.y_units == Y_Units::ABSORPTION ) ||
			( this->y_units == Y_Units::RAW_SENSOR && graph_data.y_units == Y_Units::TRANSMISSION ) )
			return graph_data.y_data;

		throw "Didn't consider this condition in " __FILE__ " in function " __FUNCTION__;
	}( );

	return { std::move( x_data ), std::move( y_data ) };
}

Interactive_Graph::Interactive_Graph( QWidget* parent ) :
	Graph_Base( parent )
{
	this->x_axis_menu_functions.push_back( [ this ]( Graph_Base* graph, QMenu* menu, QPoint pos )
	{
		auto Fix_X_Range = [ graph, this ]( X_Units new_type )
		{
			std::array<double, 2> bounds = { xAxis->range().lower, xAxis->range().upper };
			for( double & x : bounds )
				x = Convert_Units( this->axes.x_units, new_type, x );
			xAxis->setRange( std::min( bounds[ 0 ], bounds[ 1 ] ), std::max( bounds[ 0 ], bounds[ 1 ] ) );
			graph->xAxis->setLabel( Axes_Scales::X_Unit_Names[ int( new_type ) ] );
			this->axes.Set_X_Units( new_type );
			emit X_Units_Changed();
		};
		menu->addAction( "Change to Wavelength", [ this, Fix_X_Range ]
		{
			Fix_X_Range( X_Units::WAVELENGTH_MICRONS );
		} );
		menu->addAction( "Change to Wave Number", [ this, Fix_X_Range ]
		{
			Fix_X_Range( X_Units::WAVE_NUMBER );
		} );
		menu->addAction( "Change to Energy", [ this, Fix_X_Range ]
		{
			Fix_X_Range( X_Units::ENERGY_EV );
		} );
	} );
	this->y_axis_menu_functions.push_back( [ this ]( Graph_Base* graph, QMenu* menu, QPoint pos )
	{
		auto Fix_Y_Range = [ graph, this ]( FTIR::Y_Units new_type )
		{
			graph->yAxis->setLabel( Axes_Scales::Y_Unit_Names[ int( new_type ) ] );
			this->axes.Set_Y_Units( new_type );
			emit Y_Units_Changed();
		};
		menu->addAction( "Change to Raw Sensor Data", [ this, Fix_Y_Range ]
		{
			Fix_Y_Range( Y_Units::RAW_SENSOR );
		} );
		menu->addAction( "Change to Transmission", [ this, Fix_Y_Range ]
		{
			if( this->axes.y_units == FTIR::Y_Units::RAW_SENSOR )
				yAxis->setRange( 0.0, 100.0 );
			Fix_Y_Range( Y_Units::TRANSMISSION );
		} );
		menu->addAction( "Change to Absorption", [ this, Fix_Y_Range ]
		{
			if( this->axes.y_units == FTIR::Y_Units::RAW_SENSOR )
				yAxis->setRange( 0.0, 100.0 );
			Fix_Y_Range( Y_Units::ABSORPTION );
		} );
		menu->addAction( "Change to Derivative", [ this, Fix_Y_Range ]
		{
			Fix_Y_Range( Y_Units::ABSORPTION_DERIVATIVE );
		} );
	} );

	this->general_menu_functions.push_back( [ this ]( Graph_Base* graph, QMenu* menu, QPoint pos )
	{
		menu->addAction( "Clear Background", this, [ this ] { axes.Set_Y_Units( Y_Units::RAW_SENSOR ); } );

		menu->addAction( "Paste From Clipboard", [this]
		{
			const QClipboard *clipboard = QApplication::clipboard();
			const QMimeData *mimeData = clipboard->mimeData();

			if( mimeData->hasText() )
			{
				auto [x_data, y_data] = Load_XY_CSV_Data( mimeData->text().toStdString(), ",\t" );
				this->Graph<X_Units::WAVELENGTH_MICRONS, Y_Units::TRANSMISSION>( toQVec( x_data ),
							 toQVec( arma::vec( y_data ) * 100.0 ), "Clipboard Data", "Clipboard Data" );
				this->replot();
			}
			else
			{
				//setText( tr( "Cannot display data" ) );
				return;
			}
		} );

		menu->addAction( "Align Y-Levels Here", [ this, pos ]
		{
			double x = this->xAxis->pixelToCoord( pos.x() );
			double y = this->yAxis->pixelToCoord( pos.y() );

		} );
	} );
}

}