#include "Ceres_Curve_Fitting.h"
// #include "CppAD_Curve_Fitting.h"
#include "Optimize.h"

#include "Lifetime_Interactive_Graph.h"

namespace Lifetime
{

void Axes::Set_X_Units( X_Units units )
{
	if( units == x_units )
		return;

	x_units = units;
}

void Axes::Set_Y_Units( Y_Units units )
{
	if( units == y_units )
		return;

	y_units = units;
}


arma::vec Lowpass_Filter( arma::vec data, double sampling_frequency, double lowpass_Hz )
{
	arma::cx_vec data_fft = arma::fft( data );
	arma::vec lowpass_filter = arma::vec( data_fft.size(), arma::fill::ones );
	int something = (lowpass_Hz / sampling_frequency) * data_fft.size() / 2;
	int something2 = data_fft.size() - something - 1;
	// qDebug() << "something: " << something << " something2: " << something2;
	lowpass_filter.subvec( something, something2 ).fill( 0.0 );

	arma::vec filtered = arma::real( arma::ifft( data_fft % lowpass_filter ) );

	return filtered;
}

std::tuple< arma::vec, arma::vec > Modify_XY_Data( arma::vec x, arma::vec y, double x_offset, double y_offset, std::optional<double> lowpass_Hz, std::optional<double> sampling_frequency )
{
	x += x_offset;
	y += y_offset;
	if( lowpass_Hz )
		y = Lowpass_Filter( y, *sampling_frequency, *lowpass_Hz );

	return { x, y };
}

Prepared_Data Axes::Prepare_Any_Data( arma::vec x, arma::vec y, X_Units datas_x_units, double x_offset, double y_offset, std::optional<double> lowpass_Hz, std::optional<double> sampling_frequency ) const
{
	if( x.empty() || y.empty() )
		return { {}, {} };
	//if( x_units == X_Units::TEMPERATURE_K )
	//	return { toQVec( 1E6 * x ), graph_data.y_data };

	if( datas_x_units == X_Units::FIT_TIME_US ) // Original data was in fit units so don't modify it
	{
		return { toQVec( 1E6 * x ), toQVec( y ) };
	}
	else if( this->x_units == X_Units::FIT_TIME_US || this->x_units == X_Units::FIT_TIME_US2 )
	{
		if( datas_x_units == X_Units::TIME_US ) // Original data was in time units, so adjust it to be centered on the peak
		{
			auto [modified_x, modified_y] = Modify_XY_Data( x, y, x_offset, y_offset, lowpass_Hz, sampling_frequency );
			double x_loc_of_max = x( y.index_max() ); // Align all of the peaks
			x = 1E6 * (modified_x - x_loc_of_max);
			// if( this->x_units == X_Units::FIT_TIME_US2 )
				y = modified_y - y.min();
			// else
			// 	y_adjusted = y;
			return { toQVec( x ), toQVec( y ) };
		}
	}
	else if( this->x_units == X_Units::TEMPERATURE_K )
	{
		return { toQVec( x ), toQVec( y ) };
	}
	else if( this->x_units == X_Units::THOUSAND_OVER_TEMPERATURE_K )
	{
		return { toQVec( 1000 / x ), toQVec( y ) };
	}
	else if( this->x_units == X_Units::TIME_US && datas_x_units == X_Units::TIME_US )
	{
		auto [modified_x, modified_y] = Modify_XY_Data( x, y, x_offset, y_offset, lowpass_Hz, sampling_frequency );

		// Center the original data
		return { toQVec( 1E6 * (modified_x - (x.max() - x.min()) / 2) ), toQVec( modified_y ) };
	}

	auto [modified_x, modified_y] = Modify_XY_Data( x, y, x_offset, y_offset, lowpass_Hz, sampling_frequency );
	return { toQVec( 1E6 * modified_x ), toQVec( modified_y ) };
}

Prepared_Data Axes::Prepare_Fit_Data( const arma::vec & x, const arma::vec & y ) const
{
	return Prepare_Any_Data( x, y, X_Units::FIT_TIME_US );
}

Prepared_Data Axes::Prepare_XY_Data( const Single_Graph & graph_data ) const
{
	arma::vec x = fromQVec( graph_data.x_data );
	arma::vec y = fromQVec( graph_data.y_data );
	double sampling_frequency = 1 / (graph_data.x_data[1] - graph_data.x_data[0]);
	std::optional<double> lowpass_Hz = (graph_data.lowpass_MHz < 0.0) ? std::nullopt : std::optional<double>(graph_data.lowpass_MHz * 1E6);
	
	return Prepare_Any_Data( x, y, graph_data.x_units, graph_data.x_offset, graph_data.y_offset, lowpass_Hz, sampling_frequency );
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

			if( !mimeData->hasText() )
				return;
			auto[ x_data, y_data ] = Load_XY_CSV_Data( mimeData->text().toStdString(), ",\t" );
			static int clipboard_index = 0;
			this->Graph<X_Units::TIME_US, Y_Units::DONT_CHANGE>( toQVec( x_data ), toQVec( y_data ),
																	QString("Clipboard Data %1").arg( clipboard_index++ ), "Clipboard Data");
			this->replot();
		} );
	} );

	this->general_menu_functions.emplace_back( [ this ]( QMenu* menu, QPoint pos ) mutable
	{
		menu->addAction( "Zero Graphs Here Vertically", [ this, pos ]
		{
			for( auto & [name, graph] : this->remembered_graphs )
			{
				if( graph.x_data.size() == 0 || graph.y_data.size() == 0 )
					continue;
				auto [x_data, y_data] = this->axes.Prepare_XY_Data( graph );
				arma::uword index = arma::index_min(arma::abs( fromQVec(x_data) - pos.x() ));
				graph.y_offset -= y_data[ index ];
			}
			this->RegraphAll();
		} );
	} );
}

std::array<Fit_Results, 2> Fit_Lifetime_Piecewise( const arma::vec& initial_guess, const arma::vec& lower_limits, const arma::vec& upper_limits,
													const arma::vec & x, const arma::vec & y, double lower_x_fit, double upper_x_fit, double upper_x_fit2  )
{
	if( x.empty() || y.empty() )
		return { arma::datum::nan, arma::datum::nan, arma::datum::nan };

	//auto exponential_function = []( const arma::vec& fit_params, const arma::vec& time ) -> arma::vec
	//{
	//	double amplitude = fit_params[ 0 ];
	//	double offset = fit_params[ 1 ];
	//	double tau = fit_params[ 2 ];
	//	return amplitude * arma::exp( -(time - offset) / tau );
	//};

	auto exponential_function = []( const arma::vec & fit_params, const arma::vec & time ) -> arma::vec
	{
		double ln_amplitude = fit_params[0];
		double offset = fit_params[1];
		double tau = fit_params[2];
		return ln_amplitude + (-(time - offset) / tau);
	};
	arma::vec low_bound = lower_limits;
	low_bound[ 0 ] = std::log( low_bound[ 0 ] );
	arma::vec up_bound = upper_limits;
	up_bound[ 0 ] = std::log( up_bound[ 0 ] );

	arma::uvec selection_region = arma::find( x > lower_x_fit && x < upper_x_fit );
	arma::vec fit_params = Fit_Data_To_Function( exponential_function, x( selection_region ), arma::log( y( selection_region ) ), low_bound, up_bound );
	arma::uvec selection_region2 = arma::find( x > upper_x_fit && x < upper_x_fit2 );
	arma::vec fit_params2 = Fit_Data_To_Function( exponential_function, x( selection_region2 ), arma::log( y( selection_region2 ) ), low_bound, up_bound );

	//arma::vec fit_params = Fit_Data_To_Function( sum_of_exponential_functions, x( selection_region ), arma::log( y( selection_region ) ), lower_limits, upper_limits );

	return { Fit_Results{ std::exp(fit_params[ 0 ]), fit_params[ 1 ], fit_params[ 2 ] }, Fit_Results{ std::exp(fit_params2[ 0 ]), fit_params2[ 1 ], fit_params2[ 2 ] } };
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
		const arma::vec initial_guess = {  1.0,   0E-6,  1E-6, 1.0,   0E-6,  1E-6 };
		const arma::vec lower_limits = { 1E-12, -20E-6,  5E-9, 0.0, -20E-6, 50E-9 };
		const arma::vec upper_limits = {   +10, +20E-6, 100E-6, +10, +20E-6, 100E-6 };

		arma::vec x = fromQVec( graph.x_data );
		arma::vec y = fromQVec( graph.y_data );
		double sampling_frequency = 1 / (graph.x_data[1] - graph.x_data[0]);
		std::optional<double> lowpass_Hz = (graph.lowpass_MHz < 0.0) ? std::nullopt : std::optional<double>(graph.lowpass_MHz * 1E6);
		auto [modified_x, modified_y] = this->axes.Prepare_Any_Data( x, y, graph.x_units, graph.x_offset, graph.y_offset, lowpass_Hz, sampling_frequency );
		x = 1E-6 * fromQVec( modified_x );
		y = fromQVec( modified_y );
		//arma::vec fit_params = Fit_Data_To_Function( sum_of_exponential_functions, x( selection_region ), arma::log( y( selection_region ) ), lower_limits, upper_limits );
		auto [early_fit, late_fit] = [&] { switch( fit_technique ) {
			// case Fit_Technique::CPPAD: return CppAD_Fit_Lifetime< Single_Graph, Fit_Results >( initial_guess, lower_limits, upper_limits, graph );
			case Fit_Technique::CERES: return Ceres_Fit_Lifetime< Single_Graph, Fit_Results >( initial_guess, lower_limits, upper_limits, x, y, graph.lower_x_fit, graph.upper_x_fit2 );
			default:  return Fit_Lifetime_Piecewise( initial_guess, lower_limits, upper_limits, x, y, graph.lower_x_fit, graph.upper_x_fit, graph.upper_x_fit2 );
		} }();
		auto [amplitude, x_offset, lifetime] = early_fit;
		auto [amplitude2, x_offset2, lifetime2] = late_fit;
		graph.early_fit = early_fit;
		graph.late_fit = late_fit;

		//Graph<X_Units::TEMPERATURE_K, Y_Units::TIME_US>( toQVec( x( selection_region ) ), toQVec( exponential_function( { amplitude, x_offset, lifetime }, x )( selection_region ) ), "Vs_Temperature", "Lifetime vs Temperature", {} );
		QPen copy_pen = graph.graph_pointer->pen();
		copy_pen.setStyle( Qt::DashLine );
		if( fit_technique != Fit_Technique::PIECEWISE )
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
		else
		{
			{ // 1st piece-wise fit
				arma::vec x_region = arma::linspace( graph.lower_x_fit, graph.upper_x_fit, 201 );
				QString label = "Lifetime = " + QString::number( graph.early_fit.lifetime * 1E6, 'f', 4 ) + " " + QString( QChar( 0x03BC ) ) + "s";
				if( graph.early_fit_graph == nullptr ) graph.early_fit_graph = this->addGraph();
				graph.early_fit_graph->setPen( copy_pen );
				graph.early_fit_graph->setName( label );
				auto [x_fit, y_fit] = axes.Prepare_Fit_Data( x_region, exponential_function( { amplitude, x_offset, lifetime }, x_region ) );
				graph.early_fit_graph->setData( x_fit, y_fit );
			}
			{ // 2nd piece-wise fit
				arma::vec x_region2 = arma::linspace( graph.upper_x_fit, graph.upper_x_fit2, 201 );
				QString label2 = "Lifetime = " + QString::number( graph.late_fit.lifetime * 1E6, 'f', 4 ) + " " + QString( QChar( 0x03BC ) ) + "s";
				if( graph.late_fit_graph == nullptr ) graph.late_fit_graph = this->addGraph();
				graph.late_fit_graph->setPen( copy_pen );
				graph.late_fit_graph->setName( label2 );
				auto [x_fit2, y_fit2] = axes.Prepare_Fit_Data( x_region2, exponential_function( { amplitude2, x_offset2, lifetime2 }, x_region2 ) );
				graph.late_fit_graph->setData( x_fit2, y_fit2 );
			}
		}
	}
}

void Interactive_Graph::Hide_Fit_Graphs( Single_Graph & single_graph, bool should_hide )
{
	std::array< QCPGraph*, 2 > graphs = { single_graph.early_fit_graph, single_graph.late_fit_graph };
	for ( QCPGraph* graph : graphs )
	{
		if( should_hide )
		{
			if( graph == nullptr || !graph->visible() ) // Already invisible, nothing to do
				continue;
			graph->setVisible( false );
			graph->removeFromLegend();
		}
		else
		{
			if( graph == nullptr || graph->visible() ) // Already invisible, nothing to do
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

	remembered_ranges_x[ int( this->axes.x_units ) ] = xAxis->range();
	remembered_ranges_y[ int( this->axes.y_units ) ] = yAxis->range();

	if( X_Units::FIT_TIME_US2 == x_units || X_Units::TEMPERATURE_K == x_units || X_Units::THOUSAND_OVER_TEMPERATURE_K == x_units )
	{
		for( auto axis : { this->yAxis, this->yAxis2 } )
		{
			axis->setScaleType( QCPAxis::stLogarithmic );
			axis->setTicker( logTicker );
			axis->setNumberFormat( "ebd" );
			axis->setNumberPrecision( 0 );
			if( axis->range().lower < 0 )
				axis->setRangeLower( 1E-15 );
			if( axis->range().upper < 0 )
				axis->setRangeUpper( 1E-3 + axis->range().lower );
		}
	}
	else
	{
		for( auto axis : { this->yAxis, this->yAxis2 } )
		{
			axis->setScaleType( QCPAxis::stLinear );
			axis->setTicker( linearTicker );
			axis->setNumberFormat( "gbd" );
			axis->setNumberPrecision( 6 );
		}
	}


	using name_value = std::tuple<QString, Single_Graph&>;

	if( x_units == X_Units::TEMPERATURE_K || x_units == X_Units::THOUSAND_OVER_TEMPERATURE_K )
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
			//auto [early_fit, late_fit] = Fit_Lifetime( graph );
			auto[ amplitude, x_offset, lifetime ] = graph.early_fit;
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
				this->Hide_Fit_Graphs( graph, false );
			}
		}
		//this->Hide_Graph( "Vs_Temperature", true );
	}
	else if( x_units == X_Units::FIT_TIME_US || x_units == X_Units::FIT_TIME_US2 )
	{
		for( auto & [name, graph] : remembered_graphs )
			if( graph.x_units == X_Units::TIME_US || graph.x_units == X_Units::FIT_TIME_US )
			{
				this->Hide_Graph( name, false );
				this->Hide_Fit_Graphs( graph, false );
			}
			else
				this->Hide_Graph( name, true );

		//std::vector<name_value> relevant_graphs;
		////remembered_graphs
		////	% fn::where( []( const auto & x ) { const auto & [name, graph] = x; return graph.x_units == X_Units::TIME_US; } )
		////	% fn::for_each( [&relevant_graphs]( std::tuple<QString&, Single_Graph&>& x ){ auto& [name, graph] = x; relevant_graphs.emplace_back( name, graph ); } );
		//for( auto & [name, graph] : remembered_graphs )
		//	if( graph.x_units == X_Units::TIME_US )
		//		relevant_graphs.emplace_back( name, graph );
		//Redo_Fits( relevant_graphs );
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
	xAxis->setRange( remembered_ranges_x[ int( this->axes.x_units ) ] );
	yAxis->setRange( remembered_ranges_y[ int( this->axes.y_units ) ] );
	this->axes.graph_function();
	emit X_Units_Changed();
	emit Y_Units_Changed();
}

const QString Axes::X_Unit_Names[ 5 ] = { "Time (" + QString( QChar( 0x03BC ) ) + "s)",
										  "Temperature (K)",
										  "1000 / Temperature (K)",
										  "Time (" + QString( QChar( 0x03BC ) ) + "s)",
										  "Time (" + QString( QChar( 0x03BC ) ) + "s)", };
											//"1/Temperature (K" + QString( QChar( 0x207B ) ) + QString( QChar( 0x00B9 ) ) + ")" };

const QString Axes::Y_Unit_Names[ 5 ] = { "Voltage (V)",
										  "Lifetime (" + QString( QChar( 0x03BC ) ) + "s)",
										  "Lifetime (" + QString( QChar( 0x03BC ) ) + "s)",
										  "Voltage (V)",
										  "Voltage (V)", };

const QString Axes::Change_To_X_Unit_Names[ 5 ] = { "Change to Time",
													"Change to Temperature",
													"Change to 1000 / Temperature",
													"Change to Fit",
													"Change to Log Fit", };

const QString Axes::Change_To_Y_Unit_Names[ 5 ] = { "Change to Voltage",
													"Change to Lifetime vs T",
													"Change to Lifetime vs 1000 / T",
													"Change to Fit",
													"Change to Log Fit", };


}