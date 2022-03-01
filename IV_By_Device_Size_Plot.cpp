#include "IV_By_Device_Size_Plot.h"

#include <algorithm>
#include <armadillo>

#include "Units.h"

namespace Report
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
	this->x_axis_menu_functions.push_back( [ this ]( Graph_Base* graph, QMenu* menu, QPoint pos )
	{
		auto Fix_X_Range = [ graph, this ]( X_Units new_type )
		{
			std::array<double, 2> bounds = { xAxis->range().lower, xAxis->range().upper };
			for( double & x : bounds )
				x = Report::Convert_Units( this->axes.x_units, new_type, x );
			xAxis->setRange( std::min( bounds[ 0 ], bounds[ 1 ] ), std::max( bounds[ 0 ], bounds[ 1 ] ) );
			graph->xAxis->setLabel( Axes::X_Unit_Names[ int( new_type ) ] );
			this->axes.x_units = new_type;
			this->RegraphAll();
		};
		menu->addAction( "Change to Side Length", [ this, Fix_X_Range ]
		{
			Fix_X_Range( X_Units::SIDE_LENGTH_UM );
		} );
		menu->addAction( "Change to Area / Perimeter", [ this, Fix_X_Range ]
		{
			Fix_X_Range( X_Units::AREA_OVER_PERIMETER_UM );
		} );
		menu->addAction( "Change to Perimeter / Area", [ this, Fix_X_Range ]
		{
			Fix_X_Range( X_Units::PERIMETER_OVER_AREA_UM );
		} );
	} );

	this->y_axis_menu_functions.emplace_back( [ this ]( Graph_Base* graph, QMenu* menu, QPoint pos )
	{
		if( graph->yAxis->selectTest( pos, false ) >= 0 ) // general context menu on graphs requested
		{
			//CURRENT_A = 0,
			//	CURRENT_A_PER_AREA_CM = 1,
			//	LOG_CURRENT_A = 2,
			//	LOG_CURRENT_A_PER_AREA_CM = 3,
			//	ONE_SIDE_LOG_CURRENT_A_PER_AREA_CM = 4

			for( size_t index = 0; index < sizeof( Axes::Y_Unit_Names ) / sizeof( Axes::Y_Unit_Names[ 0 ] ); index++ )
			{
				QString axis_name = Axes::Y_Unit_Names[ index ];
				menu->addAction( Axes::Change_To_Y_Unit_Names[ index ], [ graph, index, axis_name, this ]
				{
					graph->axes.y_units = Y_Units( index );
					if( Y_Units::LOG_CURRENT_A == graph->axes.y_units ||
						Y_Units::LOG_CURRENT_A_PER_AREA_CM == graph->axes.y_units )
					{
						graph->yAxis->setScaleType( QCPAxis::stLogarithmic );
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
						this->yAxis->setTicker( linearTicker );
						this->yAxis2->setTicker( linearTicker );
						this->yAxis->setNumberFormat( "gbd" );
						this->yAxis2->setNumberFormat( "gbd" );
						this->yAxis->setNumberPrecision( 6 );
						this->yAxis2->setNumberPrecision( 6 );
						graph->yAxis->setScaleType( QCPAxis::stLinear );
					}
					graph->yAxis->setLabel( axis_name );
					graph->RegraphAll();
				} );
			}
		}
	} );
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

const IV_Scatter_Plot & Interactive_Graph::Store_IV_Scatter_Plot( QString measurement_name, Structured_Metadata metadata, ID_To_XY_Data data )
{
	static int color_index = 0;
	QCPGraph* current_graph = this->addGraph();

	//if( graph_title.isEmpty() )
		current_graph->setName( measurement_name );
	//else
	//	current_graph->setName( graph_title );

	// Remember data before changing it at all
	remembered_graphs[ measurement_name ] = { std::move( metadata ), std::move( data ), current_graph };
	this->graph()->setLineStyle( QCPGraph::lsNone );
	this->graph()->setScatterStyle( QCPScatterStyle::ssCircle );
	QPen graphPen;
	//QCPColorGradient gradient( QCPColorGradient::gpPolar );
	QCPColorGradient gradient( QCPColorGradient::gpSpectrum );
	gradient.setPeriodic( true );
	graphPen.setColor( gradient.color( color_index / 10.0, QCPRange( 0.0, 1.0 ) ) );
	//graphPen.setWidthF( 2 ); Changing width currently causes massive performance issues
	color_index++;
	this->graph()->setPen( graphPen );

	return remembered_graphs[ measurement_name ];
}

	//const auto & column_names = metadata.column_names;

	//int side_length_i = column_names.indexOf( "device_side_length_in_um" );
	//const std::vector<Metadata> metadata_sorted_by_device_sizes = metadata.data
	//	% fn::sort_by( [ side_length_i ]( const auto & row_of_metadata ) { return row_of_metadata[ side_length_i ]; } );

	//stored_graphs.emplace_back( { metadata.column_names, metadata_sorted_by_device_sizes }, data );

const IV_Scatter_Plot & Interactive_Graph::Graph( Structured_Metadata metadata, ID_To_XY_Data data )
{
	auto[ measurement_name, x_data, y_data ] = Axes::Build_XY_Data( metadata, data, axes.set_voltage );
	auto dataset = remembered_graphs.find( measurement_name );
	if( dataset == remembered_graphs.end() )
		Store_IV_Scatter_Plot( measurement_name, metadata, data );

	const auto & this_measurement = remembered_graphs.at( measurement_name );
	QCPGraph* this_graph = this_measurement.graph_pointer;
	auto[ adjusted_x_data, adjusted_y_data ] = axes.Prepare_XY_Data( this_measurement, x_data, y_data );
	this_graph->setData( adjusted_x_data, adjusted_y_data );

	this_graph->setVisible( true );
	this_graph->addToLegend();

	return this_measurement;
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

const QString Axes::Change_To_Y_Unit_Names[ 4 ] = { "Change to current",
								"Change to current per area",
								QString::fromWCharArray( L"Change to log\u2081\u2080( Current )" ),
								QString::fromWCharArray( L"Change to log\u2081\u2080( Current per area )" ) };

}