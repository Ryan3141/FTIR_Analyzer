﻿#include "IV_Interactive_Graph.h"

#include <algorithm>
#include <armadillo>

#include "Units.h"

#include "fn.hpp"
namespace fn = rangeless::fn;
using fn::operators::operator%;   // arg % f % g % h; // h(g(f(std::forward<Arg>(arg))));

//static QString Unit_Names[ 3 ] = { "Wave Number (cm" + QString( QChar( 0x207B ) ) + QString( QChar( 0x00B9 ) ) + ")",
//							"Wavelength (" + QString( QChar( 0x03BC ) ) + "m)",
//							"Photon Energy (eV)" };

namespace IV
{

using Graph_Base = ::Interactive_Graph<X_Units, Y_Units, Axes>;

Interactive_Graph::Interactive_Graph( QWidget* parent ) :
	Graph_Base( parent ),
	linearTicker( new QCPAxisTicker ),
	logTicker( new QCPAxisTickerLog )
{
	this->yAxis->setTicker( linearTicker );
	this->yAxis2->setTicker( linearTicker );
	this->y_axis_menu_functions.emplace_back( [this]( Graph_Base* graph, QMenu* menu, QPoint pos )
	{
		if( graph->yAxis->selectTest( pos, false ) >= 0 ) // general context menu on graphs requested
		{
			//CURRENT_A = 0,
			//	CURRENT_A_PER_AREA_CM = 1,
			//	LOG_CURRENT_A = 2,
			//	LOG_CURRENT_A_PER_AREA_CM = 3,
			//	ONE_SIDE_LOG_CURRENT_A_PER_AREA_CM = 4

			//menu->addAction( "Add random graph", this, SLOT( addRandomGraph() ) );
			for( size_t index = 0; index < sizeof( Axes::Y_Unit_Names ) / sizeof( Axes::Y_Unit_Names[0] ); index++ )
			{
				QString axis_name = Axes::Y_Unit_Names[ index ];
				menu->addAction( "Change to " + axis_name, [graph, index, axis_name, this]
				{
					graph->axes.y_units = Y_Units( index );
					if( Y_Units::LOG_CURRENT_A == graph->axes.y_units ||
						Y_Units::LOG_CURRENT_A_PER_AREA_CM == graph->axes.y_units )
					{
						graph->yAxis->setScaleType( QCPAxis::stLogarithmic );
						this->yAxis->setTicker( logTicker );
						this->yAxis2->setTicker( logTicker );
					}
					else
					{
						this->yAxis->setTicker( linearTicker );
						this->yAxis2->setTicker( linearTicker );
						graph->yAxis->setScaleType( QCPAxis::stLinear );
					}
					graph->yAxis->setLabel( axis_name );
					graph->RegraphAll();
				} );
			}
		}
	} );
}

const QString Axes::X_Unit_Names[ 1 ] = { "Voltage (V)" };
const QString Axes::Y_Unit_Names[ 5 ] = { "Current (A)",
								QString::fromWCharArray( L"Current (A/cm\u00B2)" ),
								QString::fromWCharArray( L"Current (log\u2081\u2080(|A|))" ),
								QString::fromWCharArray( L"Current (log\u2081\u2080(|A|/cm\u00B2))" ),
								QString::fromWCharArray( L"Current (log\u2081\u2080(|A|/cm\u00B2))" ) };

}