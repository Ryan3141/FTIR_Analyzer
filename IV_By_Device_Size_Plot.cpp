#include "IV_By_Device_Size_Plot.h"

#include <algorithm>
#include <armadillo>

#include "Units.h"

namespace IV_By_Size
{

using Graph_Base = ::Interactive_Graph<IV_Scatter_Plot, Axes>;

Interactive_Graph::Interactive_Graph( QWidget* parent ) :
	Graph_Base( parent ),
	linearTicker( new QCPAxisTicker ),
	logTicker( new QCPAxisTickerLog )
{
	axes.graph_function = [ this ]() { this->RegraphAll(); }; // Override graph function
	this->yAxis->setTicker( linearTicker );
	this->yAxis2->setTicker( linearTicker );
	this->x_axis_menu_functions.emplace_back( [ this ]( QMenu* menu, QPoint pos ) mutable
	{
		for( size_t index = 0; index < sizeof( Axes::X_Unit_Names ) / sizeof( Axes::X_Unit_Names[ 0 ] ); index++ )
		{
			menu->addAction( Axes::Change_To_X_Unit_Names[ index ], [ index, this ]
			{
				Change_X_Axis( index );
			} );
		}
	} );

	this->y_axis_menu_functions.emplace_back( [ this ]( QMenu* menu, QPoint pos ) mutable
	{
		for( size_t index = 0; index < sizeof( Axes::Y_Unit_Names ) / sizeof( Axes::Y_Unit_Names[ 0 ] ); index++ )
		{
			menu->addAction( Axes::Change_To_Y_Unit_Names[ index ], [ index, this ]
			{
				Change_Y_Axis( index );
			} );
		}
	} );
}

void Interactive_Graph::Change_X_Axis( int index )
{
	X_Units x_units = X_Units( index );
	std::array<double, 2> bounds = { xAxis->range().lower, xAxis->range().upper };
	for( double & x : bounds )
		x = IV_By_Size::Convert_Units( this->axes.x_units, x_units, x );
	xAxis->setRange( std::min( bounds[ 0 ], bounds[ 1 ] ), std::max( bounds[ 0 ], bounds[ 1 ] ) );
	this->xAxis->setLabel( Axes::X_Unit_Names[ int( x_units ) ] );
	this->axes.x_units = x_units;
	this->RegraphAll();
}

void Interactive_Graph::Change_Y_Axis( int index )
{
	Y_Units y_units = Y_Units( index );
	this->axes.y_units = y_units;
	if( Y_Units::LOG_CURRENT_A == this->axes.y_units ||
		Y_Units::LOG_CURRENT_A_PER_AREA_CM == this->axes.y_units )
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
	const QString & axis_name = Axes::Y_Unit_Names[ static_cast<int>( y_units ) ];
	this->yAxis->setLabel( axis_name );
	this->RegraphAll();
}

//struct Structured_Metadata
//{
//	QStringList column_names;
//	std::vector<Metadata> data;
//};
//using XY_Data = std::tuple< QVector<double>, QVector<double> >;
//using ID_To_XY_Data = std::map<QString, XY_Data>;
//using IV_Scatter_Plot = std::tuple< Structured_Metadata, ID_To_XY_Data >;
//

Linear_Equation Linear_Regression( arma::vec x_data, arma::vec y_data )
{
	double x_average = arma::mean( x_data );
	double y_average = arma::mean( y_data );
	auto S_x = arma::sum( arma::pow( x_data - x_average, 2 ) );
	auto S_xy = arma::sum( ( x_data - x_average ) % ( y_data - y_average ) );
	double slope = S_xy / S_x;
	double y_intercept = y_average - slope * x_average;
	return { y_intercept, slope };
}

const Single_Graph & Interactive_Graph::Store_IV_Scatter_Plot( QString measurement_name, Structured_Metadata metadata, ID_To_XY_Data data )
{
	static int color_index = 0;
	QCPGraph* xy_scatter_graph = this->addGraph();
	QCPGraph* line_fit_pointer = this->addGraph();

	//if( graph_title.isEmpty() )
		xy_scatter_graph->setName( measurement_name );
	//else
	//	xy_scatter_graph->setName( graph_title );

	// Remember data before changing it at all
	remembered_graphs[ measurement_name ] = { X_Units::SIDE_LENGTH_UM, Y_Units::CURRENT_A, std::move( metadata ), std::move( data ), xy_scatter_graph, line_fit_pointer };
	graphs_in_order.push_back( &( remembered_graphs[ measurement_name ] ) );
	const QCPScatterStyle dot_styles[] = { QCPScatterStyle::ssPlus, QCPScatterStyle::ssCircle, QCPScatterStyle::ssStar, QCPScatterStyle::ssTriangleInverted, QCPScatterStyle::ssSquare, QCPScatterStyle::ssDiamond, QCPScatterStyle::ssCross, QCPScatterStyle::ssTriangle, QCPScatterStyle::ssCrossSquare, QCPScatterStyle::ssPlusSquare, QCPScatterStyle::ssCrossCircle, QCPScatterStyle::ssPlusCircle, QCPScatterStyle::ssPeace, QCPScatterStyle::ssDisc };
	constexpr int style_count = sizeof( dot_styles ) / sizeof( QCPScatterStyle );
	QPen graphPen;
	//QCPColorGradient gradient( QCPColorGradient::gpPolar );
	QCPColorGradient gradient( QCPColorGradient::gpSpectrum );
	gradient.setPeriodic( true );
	graphPen.setColor( gradient.color( color_index / 10.0, QCPRange( 0.0, 1.0 ) ) );
	{
		xy_scatter_graph->setLineStyle( QCPGraph::lsNone );
		xy_scatter_graph->setScatterStyle( dot_styles[color_index % style_count] );
		//QCPColorGradient gradient( QCPColorGradient::gpPolar );
		QCPColorGradient gradient( QCPColorGradient::gpSpectrum );
		gradient.setPeriodic( true );
		graphPen.setColor( gradient.color( color_index / 10.0, QCPRange( 0.0, 1.0 ) ) );
		color_index++;
		xy_scatter_graph->setPen( graphPen );
	}
	{
		line_fit_pointer->setLineStyle( QCPGraph::lsLine );
		line_fit_pointer->setScatterStyle( QCPScatterStyle::ssNone );
		//graphPen.setWidthF( 2 ); Changing width currently causes massive performance issues
		line_fit_pointer->setPen( graphPen );
	}

	return remembered_graphs[ measurement_name ];
}

	//const auto & column_names = metadata.column_names;

	//int side_length_i = column_names.indexOf( "device_side_length_in_um" );
	//const std::vector<Metadata> metadata_sorted_by_device_sizes = metadata.data
	//	% fn::sort_by( [ side_length_i ]( const auto & row_of_metadata ) { return row_of_metadata[ side_length_i ]; } );

	//stored_graphs.emplace_back( { metadata.column_names, metadata_sorted_by_device_sizes }, data );

void Axes::Graph_XY_Data( QString measurement_name, const Single_Graph & graph )
{
	//scatter_data->setVisible( true );
	//scatter_data->addToLegend();

	auto[ x_data, y_data ] = this->Prepare_XY_Data( graph );
	graph.graph_pointer->setData( x_data, y_data );

	auto[ fit_name, x_line_fit_data, y_line_fit_data ] = this->Prepare_Linear_Fit_Data( x_data, y_data );
	graph.line_fit_pointer->setName( fit_name );
	graph.line_fit_pointer->setData( x_line_fit_data, y_line_fit_data );
}

const Single_Graph & Interactive_Graph::Graph( Structured_Metadata metadata, ID_To_XY_Data data, QString plot_title )
{
	auto[ measurement_name, x_data, y_data ] = Axes::Build_XY_Data( metadata, data, axes.set_voltage );
	auto dataset = remembered_graphs.find( measurement_name );
	if( dataset == remembered_graphs.end() )
		Store_IV_Scatter_Plot( measurement_name, metadata, data );

	const auto & this_measurement = remembered_graphs.at( measurement_name );
	if( plot_title.isEmpty() )
		plot_title = measurement_name;
	axes.Graph_XY_Data( plot_title, this_measurement );

	return this_measurement;
}

void Interactive_Graph::Show_Reference_Graph( QString measurement_name, QVector<double> x_data, QVector<double> y_data )
{
	if( reference_graph_pointer == nullptr )
	{
		reference_graph_pointer = this->addGraph();
		reference_graph_pointer->setName( measurement_name );
		reference_graph_pointer->setLineStyle( QCPGraph::lsLine );
		QPen graphPen;
		graphPen.setColor( QRgb( 0xFF0000 ) );
		graphPen.setStyle( Qt::SolidLine );
		this->graph()->setPen( graphPen );
	}

	reference_graph_pointer->setData( x_data, y_data );

	reference_graph_pointer->setVisible( true );
	reference_graph_pointer->addToLegend();
}

void Interactive_Graph::Hide_Reference_Graph( QString measurement_name )
{
	if( reference_graph_pointer == nullptr || !reference_graph_pointer->visible() ) // Already invisible, nothing to do
		return;
	reference_graph_pointer->setVisible( false );
	reference_graph_pointer->removeFromLegend();
}

void Interactive_Graph::Hide_Fit_Graphs( bool should_hide )
{
	for( auto full_graph : this->graphs_in_order )
	{
		QCPGraph* xy_scatter_graph = full_graph->graph_pointer;
		QCPGraph* line_fit_graph = full_graph->line_fit_pointer;
		if( should_hide )
		{
			if( !line_fit_graph->visible() ) // Already invisible, nothing to do
				return;
			line_fit_graph->setVisible( false );
			line_fit_graph->removeFromLegend();
		}
		else
		{
			xy_scatter_graph->removeFromLegend();
			xy_scatter_graph->addToLegend();
			line_fit_graph->setVisible( true );
			line_fit_graph->addToLegend();
		}
	}
	this->replot();
}

Axes::Prepared_Data_And_Name Axes::Build_XY_Data( const Structured_Metadata & metadata, const ID_To_XY_Data & data, double voltage_to_use )
{
	const auto & column_names = metadata.column_names;

	int sample_name_i = column_names.indexOf( "sample_name" );
	int temperature_in_k_i = column_names.indexOf( "temperature_in_k" );

	int measurement_id_i = column_names.indexOf( "measurement_id" );
	int device_side_length_in_um_i = column_names.indexOf( "device_side_length_in_um" );
	QVector< double > side_lengths_um;
	QVector< double > currents_per_side_a;
	for( const auto & one_metadata : metadata.data )
	{
		QString measurement_id = one_metadata[ measurement_id_i ].toString();
		if( data.find( measurement_id ) == data.end() )
			continue;

		const auto &[ voltages_v, currents_a ] = data.at( measurement_id );
		double side_length_um = one_metadata[ device_side_length_in_um_i ].toDouble();

		int i = std::distance( voltages_v.begin(), std::lower_bound( voltages_v.begin(), voltages_v.end(), voltage_to_use ) );
		i = std::min( voltages_v.size() - 1, i );
		side_lengths_um.push_back( side_length_um ); // Area / perimeter = side * side / (4 * side)
		currents_per_side_a.push_back( currents_a[ i ] );
	}

	QString name = QString( "%1 %2 K" ).arg( metadata.data[ 0 ][ sample_name_i ].toString(), metadata.data[ 0 ][ temperature_in_k_i ].toString() );
	return { name, side_lengths_um, currents_per_side_a };
	//this->replot();
}

const QString Axes::X_Unit_Names[ 3 ] = {
								QString::fromWCharArray( L"Side Length (\u03BCm)" ),
								QString::fromWCharArray( L"Area / Perimeter (\u03BCm)" ),
								QString::fromWCharArray( L"Perimeter / Area (\u03BCm\u207B\u00B9)" ) };
const QString Axes::Y_Unit_Names[ 4 ] = { "Current (A)",
								QString::fromWCharArray( L"Current (A/cm\u00B2)" ),
								QString::fromWCharArray( L"Current (log\u2081\u2080(|A|))" ),
								QString::fromWCharArray( L"Current (log\u2081\u2080(|A|/cm\u00B2))" ) };

const QString Axes::Change_To_X_Unit_Names[ 3 ] = { "Change to Side Length",
								"Change to Area / Perimeter",
								"Change to Perimeter / Area" };

const QString Axes::Change_To_Y_Unit_Names[ 4 ] = { "Change to current",
								"Change to current per area",
								QString::fromWCharArray( L"Change to log\u2081\u2080( Current )" ),
								QString::fromWCharArray( L"Change to log\u2081\u2080( Current per area )" ) };

}