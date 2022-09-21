﻿#include "Lifetime_Interactive_Graph.h"
# include <cppad/ipopt/solve.hpp>

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


Prepared_Data Axes::Prepare_Any_Data( const arma::vec & x, const arma::vec & y, X_Units x_units ) const
{
	//if( x_units == X_Units::TEMPERATURE_K )
	//	return { toQVec( 1E6 * x ), graph_data.y_data };

	if ( this->x_units == X_Units::FIT_TIME_US || this->x_units == X_Units::LOG_Y )
	{
		if ( x_units == X_Units::FIT_TIME_US )
			return { toQVec( 1E6 * x ), toQVec( y ) };
		else if ( x_units == X_Units::TIME_US )
		{
			double x_offset = x( y.index_max() ); // Align all of the peaks
			arma::vec x_adjusted = 1E6 * (x - x_offset);
			arma::vec y_adjusted = y - y.min();
			return { toQVec( x_adjusted ), toQVec( y_adjusted ) };
		}
	}
	else if ( this->x_units == X_Units::TEMPERATURE_K )
	{
		return { toQVec( x ), toQVec( y ) };
	}
	return { toQVec( 1E6 * x ), toQVec( y ) };
}

Prepared_Data Axes::Prepare_Fit_Data( const arma::vec & x, const arma::vec & y ) const
{
	return Prepare_Any_Data( x, y, X_Units::FIT_TIME_US );
}

Prepared_Data Axes::Prepare_XY_Data( const Single_Graph & graph_data ) const
{
	arma::vec x = arma::conv_to<arma::vec>::from( graph_data.x_data.toStdVector() );
	arma::vec y = arma::conv_to<arma::vec>::from( graph_data.y_data.toStdVector() );
	return Prepare_Any_Data( x, y, graph_data.x_units );
}

void Axes::Graph_XY_Data( QString measurement_name, const Single_Graph & graph )
{
	auto[ x_data, y_data ] = this->Prepare_XY_Data( graph );
	graph.graph_pointer->setData( x_data, y_data );
}

Interactive_Graph::Interactive_Graph( QWidget* parent ) :
	Graph_Base( parent ),
	linearTicker( new QCPAxisTicker ),
	logTicker( new QCPAxisTickerLog )
{
	this->yAxis->setTicker( linearTicker );
	this->yAxis2->setTicker( linearTicker );

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

const std::string ipopt_options =
"Integer print_level  1\n"
"String  sb           yes\n"
// maximum number of iterations
"Integer max_iter     100\n"
// approximate accuracy in first order necessary conditions;
// see Mathematical Programming, Volume 106, Number 1,
// Pages 25-57, Equation (6)
"Numeric tol          1e-12\n"
// derivative testing
"String  derivative_test            second-order\n"
// maximum amount of random pertubation; e.g.,
// when evaluation finite diff
"Numeric point_perturbation_radius 0.\n";

template< typename Vec_Type >
arma::vec to_arma( Vec_Type vec )
{
	return arma::conv_to<arma::vec>::from( vec );
}
template<>
arma::vec to_arma( QVector<double> vec )
{
	return arma::conv_to<arma::vec>::from( vec.toStdVector() );
}
template<>
arma::vec to_arma( CppAD::vector< CppAD::AD<double> > vec )
{
	arma::vec out_vec( vec.size() );
	for( int i = 0; i < vec.size(); i++ )
		out_vec[ i ] = CppAD::Value( CppAD::Var2Par( vec[ i ] ) );
	return out_vec;
	//return arma::vec( reinterpret_cast<double*>( vec.data() ), vec.size(), true );
}

template< typename Vec_Type >
Vec_Type from_arma( arma::vec vec )
{
	return arma::conv_to<Vec_Type>::from( vec );
}

template<>
CppAD::vector< CppAD::AD<double> > from_arma( arma::vec vec )
{
	CppAD::vector< CppAD::AD<double> > out_vec{ vec.size() };
	for( int i = 0; i < vec.size(); i++ )
		out_vec[ i ] = vec[ i ];
	return out_vec;
}

namespace {
	using CppAD::AD;

	template< typename Minimizing_Function >
	class FG_eval {
	public:
		Minimizing_Function func;
		FG_eval( Minimizing_Function func ) : func( func ) {}
		//using ADvector = std::vector<double>;
		typedef CPPAD_TESTVECTOR( AD<double> ) ADvector;
		void operator()( ADvector & fg, const ADvector & x )
		{
			fg = func( x );
		}
	};
}


std::array<Fit_Results, 2> Fit_Lifetime( const Single_Graph & graph )
{
	if( graph.x_data.empty() || graph.y_data.empty() )
		return { arma::datum::nan, arma::datum::nan, arma::datum::nan };
	arma::vec x = arma::conv_to<arma::vec>::from( graph.x_data.toStdVector() );
	arma::vec y = arma::conv_to<arma::vec>::from( graph.y_data.toStdVector() );

	double x_offset = x( y.index_max() ); // Align all of the peaks
	x = x - x_offset;
	y = y - y.min() + 1E-9;
	//arma::vec q = x.elem(  );
	//auto exponential_function = []( const arma::vec & fit_params, const arma::vec & time ) -> arma::vec
	//{
	//	double amplitude = fit_params[ 0 ];
	//	double offset    = fit_params[ 1 ];
	//	double tau       = fit_params[ 2 ];
	//	return amplitude * arma::exp( -( time - offset ) / tau );
	//};

	//auto exponential_function = []( const arma::vec & fit_params, const arma::vec & time ) -> arma::vec
	//{
	//	double ln_amplitude = fit_params[0];
	//	double offset = fit_params[1];
	//	double tau = fit_params[2];
	//	return ln_amplitude + (-(time - offset) / tau);
	//};

	//auto selection_region = arma::find( x > graph.lower_x_fit && x < graph.upper_x_fit );
	//arma::vec fit_params = Fit_Data_To_Function( exponential_function, x( selection_region ), arma::log( y( selection_region ) ), { -1E-3, -20E-6, 500E-9 }, { +100, +20E-6, 100E-6 } );
	//auto selection_region2 = arma::find( x > graph.upper_x_fit && x < graph.upper_x_fit2 );
	//arma::vec fit_params2 = Fit_Data_To_Function( exponential_function, x( selection_region2 ), arma::log( y( selection_region2 ) ), { -1E-3, -20E-6, 500E-9 }, { +100, +20E-6, 100E-6 } );
	//return { Fit_Results{ std::exp( fit_params[0] ), fit_params[1], fit_params[2] }, Fit_Results{ std::exp( fit_params2[0] ), fit_params2[1], fit_params2[2] } };

	//auto sum_of_exponential_functions = []( const arma::vec& fit_params, const arma::vec& time ) -> arma::vec
	//{
	//	double ln_A = fit_params[ 0 ];
	//	double offset1 = fit_params[ 1 ];
	//	double tau1 = fit_params[ 2 ];
	//	double ln_B = fit_params[ 3 ];
	//	double offset2 = fit_params[ 4 ];
	//	double tau2 = fit_params[ 5 ];
	//	arma::vec x1 = -(time - offset1) / tau1;
	//	arma::vec x2 = -(time - offset2) / tau2;
	//	//return arma::log( A * arma::exp( x1 ) + B * arma::exp( x2 ) );
	//	return ln_B + x2 + arma::log( std::exp( ln_A - ln_B ) * arma::exp( x1 - x2 ) + 1 );
	//};

	using ADvector = CPPAD_TESTVECTOR( AD<double> );
	auto sum_of_exponential_functions = []( const ADvector & fit_params, const arma::vec & time ) -> ADvector
	{
		AD<double> A = fit_params[ 0 ];
		AD<double> offset1 = fit_params[ 1 ];
		AD<double> tau1 = fit_params[ 2 ];
		AD<double> B = fit_params[ 3 ];
		AD<double> offset2 = fit_params[ 4 ];
		AD<double> tau2 = fit_params[ 5 ];
		ADvector result( time.size() );
		for( auto i = 0; i < time.size(); i++ )
			result[ i ] = A * CppAD::exp( -(time[ i ] - offset1) / tau1 ) + B * CppAD::exp( -(time[ i ] - offset2) / tau2 );
		return result;
		//return ln_B + x2 + arma::log( std::exp( ln_A - ln_B ) * arma::exp( x1 - x2 ) + 1 );
	};

	arma::uvec selection_region = arma::find( x > graph.lower_x_fit && x < graph.upper_x_fit2 );
	arma::vec x_fit_data = x( selection_region );
	arma::vec y_fit_data = y( selection_region );

	auto error_function = [&x_fit_data, &y_fit_data, sum_of_exponential_functions]( const ADvector & fit_params ) -> ADvector {
		ADvector exp_results = sum_of_exponential_functions( fit_params, x_fit_data );
		AD<double> sum_of_squares = 0;
		for( auto i = 0; i < exp_results.size(); i++ )
		{
			auto difference = y_fit_data[ i ] - exp_results[ i ];
			sum_of_squares += difference * difference;
		}
		ADvector results( 1 );
		results[ 0 ] = sum_of_squares;
		return results;
		//arma::vec diff = y_fit_data - sum_of_exponential_functions( fit_params, x_fit_data );
		//std::cout << fit_params[ 0 ] << " " << fit_params[ 1 ] << " " << fit_params[ 2 ] << ": " << arma::dot( diff, diff ) << "\n";
		//return { arma::dot( diff, diff ) };
	};

	const arma::vec initial_guess = { 1, 0E-6, 1E-6, 1, 0E-6, 1E-6 };
	const arma::vec lower_limits = { 0.0, -20E-6, 500E-9, 0.0, -20E-6, 500E-9 };
	const arma::vec upper_limits = { +10, +20E-6, 40E-6, +10, +20E-6, 40E-6 };
	//arma::vec fit_params = Fit_Data_To_Function( sum_of_exponential_functions, x( selection_region ), arma::log( y( selection_region ) ), lower_limits, upper_limits );
	CppAD::ipopt::solve_result<arma::vec> solution;
	FG_eval fg_eval{ error_function };
	CppAD::ipopt::solve( ipopt_options, initial_guess, lower_limits, upper_limits, {}, {}, fg_eval, solution );
	arma::vec fit_params = solution.x;

	if( fit_params[ 2 ] < fit_params[ 5 ] )
		return { Fit_Results{ ( fit_params[ 0 ] ), fit_params[ 1 ], fit_params[ 2 ] }, Fit_Results{ ( fit_params[ 3 ] ), fit_params[ 4 ], fit_params[ 5 ] } };
	else
		return { Fit_Results{ ( fit_params[ 3 ] ), fit_params[ 4 ], fit_params[ 5 ] }, Fit_Results{ ( fit_params[ 0 ] ), fit_params[ 1 ], fit_params[ 2 ] } };
}

void Interactive_Graph::Redo_Fits( std::vector<std::tuple<QString, Single_Graph &>> graphs_for_fit )
{
	auto exponential_function = [](const arma::vec& fit_params, const arma::vec& time) -> arma::vec
	{
		double amplitude = fit_params[0];
		double offset = fit_params[1];
		double tau = fit_params[2];
		return amplitude * arma::exp(-(time - offset) / tau);
	};
	auto sum_of_exponential_functions = []( const arma::vec& fit_params, const arma::vec& fit_params2, const arma::vec& time ) -> arma::vec
	{
		double A = fit_params[ 0 ];
		double offset1 = fit_params[ 1 ];
		double tau1 = fit_params[ 2 ];
		double B = fit_params2[ 0 ];
		double offset2 = fit_params2[ 1 ];
		double tau2 = fit_params2[ 2 ];
		arma::vec x1 = -(time - offset1) / tau1;
		arma::vec x2 = -(time - offset2) / tau2;
		return A * arma::exp( x1 ) + B * arma::exp( x2 );
	};


	for( const auto & [name, graph] : graphs_for_fit )
	{
		arma::vec x = arma::conv_to<arma::vec>::from( graph.x_data.toStdVector() );
		auto [early_fit, late_fit] = Fit_Lifetime( graph );
		auto [amplitude, x_offset, lifetime] = early_fit;
		auto [amplitude2, x_offset2, lifetime2] = late_fit;
		graph.early_fit = early_fit;
		graph.late_fit = late_fit;

		//Graph<X_Units::TEMPERATURE_K, Y_Units::TIME_US>( toQVec( x( selection_region ) ), toQVec( exponential_function( { amplitude, x_offset, lifetime }, x )( selection_region ) ), "Vs_Temperature", "Lifetime vs Temperature", {} );
		QPen copy_pen = graph.graph_pointer->pen();
		copy_pen.setStyle( Qt::DashLine );
		{
			arma::vec x_region = arma::linspace( graph.lower_x_fit, graph.upper_x_fit2, 201 );
			QString label = "Lifetime = " + QString::number( graph.early_fit.lifetime * 1E6, 'f', 4 ) + " " + QString( QChar( 0x03BC ) ) + "s";
			label += " Lifetime2 = " + QString::number( graph.late_fit.lifetime * 1E6, 'f', 4 ) + " " + QString( QChar( 0x03BC ) ) + "s";
			if( graph.early_fit_graph == nullptr ) graph.early_fit_graph = this->addGraph();
			graph.early_fit_graph->setPen( copy_pen );
			graph.early_fit_graph->setName( label );
			auto [x_fit, y_fit] = axes.Prepare_Fit_Data( x_region, sum_of_exponential_functions( { amplitude, x_offset, lifetime }, { amplitude2, x_offset2, lifetime2 }, x_region ) );
			graph.early_fit_graph->setData( x_fit, y_fit );
		}
		if constexpr( false ) // 1st piece-wise fit
		{
			arma::vec x_region = arma::linspace( graph.lower_x_fit, graph.upper_x_fit, 201 );
			QString label = "Lifetime = " + QString::number( graph.early_fit.lifetime * 1E6, 'f', 4 ) + " " + QString( QChar( 0x03BC ) ) + "s";
			if( graph.early_fit_graph == nullptr ) graph.early_fit_graph = this->addGraph();
			graph.early_fit_graph->setPen( copy_pen );
			graph.early_fit_graph->setName( label );
			auto [x_fit, y_fit] = axes.Prepare_Fit_Data( x_region, exponential_function( { amplitude, x_offset, lifetime }, x_region ) );
			graph.early_fit_graph->setData( x_fit, y_fit );
		}
		if constexpr( false ) // 2nd piece-wise fit
		{
			arma::vec x_region2 = arma::linspace( graph.upper_x_fit, graph.upper_x_fit2, 201 );
			QString label2 = "Lifetime = " + QString::number( graph.late_fit.lifetime * 1E6, 'f', 4 ) + " " + QString( QChar( 0x03BC ) ) + "s";
			if( graph.late_fit_graph == nullptr ) graph.late_fit_graph = this->addGraph();
			graph.late_fit_graph->setPen( copy_pen );
			graph.late_fit_graph->setName( label2 );
			auto [x_fit2, y_fit2] = axes.Prepare_Fit_Data( x_region2, exponential_function( { amplitude2, x_offset2, lifetime2 }, x_region2 ) );
			graph.late_fit_graph->setData(  x_fit2, y_fit2 );
		}
	}
}

void Interactive_Graph::Hide_Fit_Graphs( Single_Graph & single_graph, bool should_hide )
{
	std::array< QCPGraph*, 2 > graphs = { single_graph.early_fit_graph, single_graph.late_fit_graph };
	for ( QCPGraph* graph : graphs )
	{
		if ( should_hide )
		{
			if ( graph == nullptr || !graph->visible() ) // Already invisible, nothing to do
				continue;
			graph->setVisible( false );
			graph->removeFromLegend();
		}
		else
		{
			if ( graph == nullptr || graph->visible() ) // Already invisible, nothing to do
				continue;
			graph->setVisible( true );
			graph->addToLegend();
		}
	}
	replot();
}


void Interactive_Graph::Change_Axes( int index )
{
	X_Units x_units = X_Units( index );
	Y_Units y_units = Y_Units( index );
	//std::array<double, 2> bounds = { xAxis->range().lower, xAxis->range().upper };
	//std::array<double, 2> original = { xAxis->range().lower, xAxis->range().upper };

	if( X_Units::LOG_Y == x_units )
	{
		this->yAxis->setScaleType( QCPAxis::stLogarithmic );
		this->yAxis->setTicker( logTicker );
		this->yAxis2->setTicker( logTicker );
		this->yAxis->setNumberFormat( "ebd" );
		this->yAxis2->setNumberFormat( "ebd" );
		this->yAxis->setNumberPrecision( 0 );
		this->yAxis2->setNumberPrecision( 0 );
		if( this->yAxis->range().lower < 0 )
			this->yAxis->setRangeLower( 1E-15 );
		if( this->yAxis->range().upper < 0 )
			this->yAxis->setRangeUpper( 1E-3 + this->yAxis->range().lower );
	}
	else
	{
		this->yAxis->setScaleType( QCPAxis::stLinear );
		this->yAxis->setTicker( linearTicker );
		this->yAxis2->setTicker( linearTicker );
		this->yAxis->setNumberFormat( "gbd" );
		this->yAxis2->setNumberFormat( "gbd" );
		this->yAxis->setNumberPrecision( 6 );
		this->yAxis2->setNumberPrecision( 6 );
	}


	using name_value = std::tuple<QString, Single_Graph&>;

	if( x_units == X_Units::TEMPERATURE_K )
	{
		for( auto& [name, graph] : remembered_graphs )
			if( graph.x_units == X_Units::TIME_US || graph.x_units == X_Units::FIT_TIME_US )
			{
				this->Hide_Graph( name, true );
				this->Hide_Fit_Graphs( graph, true );
			}

		std::vector<name_value> relevant_graphs;
		for( auto& [name, graph] : remembered_graphs )
			if( graph.x_units == X_Units::TIME_US )
				relevant_graphs.emplace_back( name, graph );
		QVector<double> temperatures = relevant_graphs % fn::transform([this]( const auto & g )
		{
			const auto &[ name, graph ] = g;
			auto y = graph.meta.find("temperature_in_k");
			return ( y == graph.meta.end() ) ? 0.0 : y->second.toDouble();
		}) % fn::to(QVector<double>{});
		QVector<double> lifetimes_us = relevant_graphs % fn::transform( [ this ]( const auto & x )
		{
			const auto &[ name, graph ] = x;
			auto [early_fit, late_fit] = Fit_Lifetime( graph );
			auto[ amplitude, x_offset, lifetime ] = early_fit;
			return lifetime * 1E6;
		} ) % fn::to( QVector<double>{} );

		Graph<X_Units::TEMPERATURE_K, Y_Units::TIME_US>( temperatures, lifetimes_us, "Vs_Temperature", "Lifetime vs Temperature", {} );
	}
	else if( x_units == X_Units::TIME_US )
	{
		for( auto &[ name, graph ] : remembered_graphs )
		{
			if( graph.x_units != X_Units::TIME_US )
				this->Hide_Graph( name, true );
			else
			{
				this->Hide_Graph( name, false );
				this->Hide_Fit_Graphs( graph, true );
			}
		}
		//this->Hide_Graph( "Vs_Temperature", true );
	}
	else if( x_units == X_Units::FIT_TIME_US || x_units == X_Units::LOG_Y )
	{
		for( auto & [name, graph] : remembered_graphs )
			if( graph.x_units == X_Units::TIME_US || graph.x_units == X_Units::FIT_TIME_US )
			{
				this->Hide_Graph( name, false );
				this->Hide_Fit_Graphs( graph, false );
			}
			else
				this->Hide_Graph( name, true );

		std::vector<name_value> relevant_graphs;
		//remembered_graphs
		//	% fn::where( []( const auto & x ) { const auto & [name, graph] = x; return graph.x_units == X_Units::TIME_US; } )
		//	% fn::for_each( [&relevant_graphs]( std::tuple<QString&, Single_Graph&>& x ){ auto& [name, graph] = x; relevant_graphs.emplace_back( name, graph ); } );
		for( auto & [name, graph] : remembered_graphs )
			if( graph.x_units == X_Units::TIME_US )
				relevant_graphs.emplace_back( name, graph );
		Redo_Fits( relevant_graphs );
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

const QString Axes::X_Unit_Names[ 4 ] = { "Time (" + QString( QChar( 0x03BC ) ) + "s)",
										  "Temperature (K)",
										  "Time (" + QString( QChar( 0x03BC ) ) + "s)",
										  "Time (" + QString( QChar( 0x03BC ) ) + "s)", };
											//"1/Temperature (K" + QString( QChar( 0x207B ) ) + QString( QChar( 0x00B9 ) ) + ")" };

const QString Axes::Y_Unit_Names[ 4 ] = { "Voltage (V)",
										  "Lifetime (" + QString( QChar( 0x03BC ) ) + "s)",
										  "Voltage (V)",
										  "Voltage (V)", };

const QString Axes::Change_To_X_Unit_Names[ 4 ] = { "Change to Time",
													"Change to Temperature",
													"Change to Fit",
													"Change to Log Fit", };

const QString Axes::Change_To_Y_Unit_Names[ 4 ] = { "Change to Voltage",
													"Change to Lifetime",
													"Change to Fit",
													"Change to Log Fit", };


}