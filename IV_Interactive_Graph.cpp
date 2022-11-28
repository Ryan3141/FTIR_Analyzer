#include "IV_Interactive_Graph.h"

#include <algorithm>
#include <armadillo>

#include "Units.h"

#include "rangeless_helper.hpp"

//static QString Unit_Names[ 3 ] = { "Wave Number (cm" + QString( QChar( 0x207B ) ) + QString( QChar( 0x00B9 ) ) + ")",
//							"Wavelength (" + QString( QChar( 0x03BC ) ) + "m)",
//							"Photon Energy (eV)" };

namespace IV
{

Interactive_Graph::Interactive_Graph( QWidget* parent ) :
	Graph_Base( parent ),
	linearTicker( new QCPAxisTicker ),
	logTicker( new QCPAxisTickerLog )
{
	this->yAxis->setTicker( linearTicker );
	this->yAxis2->setTicker( linearTicker );
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
}

void Interactive_Graph::Change_Y_Axis( int index )
{
	Y_Units y_units = Y_Units( index );
	this->axes.y_units = y_units;
	if( Y_Units::LOG_CURRENT_A == this->axes.y_units ||
		Y_Units::LOG_CURRENT_A_PER_AREA_CM == this->axes.y_units )
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
	const QString & axis_name = Axes::Y_Unit_Names[ static_cast<int>( y_units ) ];
	this->yAxis->setLabel( axis_name );
	this->RegraphAll();
}

const QString Axes::X_Unit_Names[ 1 ] = { "Voltage (V)" };
const QString Axes::Change_To_Y_Unit_Names[ 7 ] = { "Change to current",
								"Change to current per area",
								QString::fromWCharArray( L"Change to log\u2081\u2080( Current )" ),
								QString::fromWCharArray( L"Change to log\u2081\u2080( Current per area )" ),
								QString::fromWCharArray( L"Change to one-sided log\u2081\u2080( Current per area )" ),
								QString::fromWCharArray( L"Change to resistance" ),
								QString::fromWCharArray( L"Change to resistance per area" ) };
const QString Axes::Y_Unit_Names[ 7 ] = { "Current (A)",
								QString::fromWCharArray( L"Current (A/cm\u00B2)" ),
								QString::fromWCharArray( L"Current (|A|))" ),
								QString::fromWCharArray( L"Current (|A/cm\u00B2|)" ),
								QString::fromWCharArray( L"Current (|A/cm\u00B2|)" ),
								QString::fromWCharArray( L"Resistance (\u03A9)" ),
								QString::fromWCharArray( L"Resistance (\u03A9/cm\u00B2)" ) };
//QString::fromWCharArray( L"Current (log\u2081\u2080(|A|/cm\u00B2))" ),
}