#include "IV_By_Device_Size_Plot.h"

#include <algorithm>
#include <armadillo>

#include "Units.h"

namespace Report
{

using Graph_Base = ::Interactive_Graph<X_Units, Y_Units, Axes>;

Interactive_Graph::Interactive_Graph( QWidget* parent ) :
	Graph_Base( parent ),
	linearTicker( new QCPAxisTicker ),
	logTicker( new QCPAxisTickerLog )
{
	this->yAxis->setTicker( linearTicker );
	this->yAxis2->setTicker( linearTicker );
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
						this->yAxis->setNumberFormat( "eed" );
						this->yAxis2->setNumberFormat( "eed" );
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
						this->yAxis->setNumberFormat( "ggd" );
						this->yAxis2->setNumberFormat( "ggd" );
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

Axes::Axes( std::function<void()> regraph_function ) : graph_function( regraph_function )
{
}

const QString Axes::X_Unit_Names[ 1 ] = { QString::fromWCharArray( L"Area / Side Length (\u03BCm)" ) };
const QString Axes::Y_Unit_Names[ 4 ] = { "Current (A)",
								QString::fromWCharArray( L"Current (A/cm\u00B2)" ),
								QString::fromWCharArray( L"Current (log\u2081\u2080(|A|))" ),
								QString::fromWCharArray( L"Current (log\u2081\u2080(|A|/cm\u00B2))" ) };

const QString Axes::Change_To_Y_Unit_Names[ 4 ] = { "Change to current",
								"Change to current per area",
								QString::fromWCharArray( L"Change to log\u2081\u2080( Current )" ),
								QString::fromWCharArray( L"Change to log\u2081\u2080( Current per area )" ) };

}