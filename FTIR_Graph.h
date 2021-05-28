#pragma once

#include <tuple>
#include <QVector>
#include <functional>

#include "Interactive_Graph.h"


enum class FTIR_X_Units
{
	WAVE_NUMBER = 0,
	WAVELENGTH_MICRONS = 1,
	ENERGY_EV = 2,
	WAVELENGTH_METERS = 3
};

enum class FTIR_Y_Units
{
	RAW_SENSOR = 0,
	TRANSMISSION = 1,
	ABSORPTION = 2,
	ABSORPTION_DERIVATIVE = 3,
	DONT_CHANGE = 4
};


struct FTIR_Axes_Scales
{
	using XY_Data = std::tuple< QVector<double>, QVector<double> >;

	const static FTIR_X_Units default_x_units = FTIR_X_Units::WAVELENGTH_MICRONS;
	const static FTIR_Y_Units default_y_units = FTIR_Y_Units::RAW_SENSOR;
	FTIR_X_Units x_units = default_x_units;
	FTIR_Y_Units y_units = default_y_units;
	std::function<void()> graph_function;

	FTIR_Axes_Scales( std::function<void()> regraph_function ) : graph_function( regraph_function )
	{
	}

	void Set_X_Units( FTIR_X_Units units );
	void Set_Y_Units( FTIR_Y_Units units );

	void Set_As_Background( XY_Data xy );

	std::tuple< QVector<double>, QVector<double> > Prepare_XY_Data( const Single_Graph<FTIR_X_Units, FTIR_Y_Units> & graph_data ) const;

	QVector<double> background_x_data;
	QVector<double> background_y_data;

	const inline static QString X_Unit_Names[ 3 ] = {
		"Wave Number (cm" + QString( QChar( 0x207B ) ) + QString( QChar( 0x00B9 ) ) + ")",
		"Wavelength (" + QString( QChar( 0x03BC ) ) + "m)",
		"Photon Energy (eV)" };

	const inline static QString Y_Unit_Names[ 4 ] = {
		"Raw Sensor Data (arbitrary units)",
		"Transmission %",
		"Absorption %",
		"Absorption Derivative" };
};

class FTIR_Graph :
	public ::Interactive_Graph<FTIR_X_Units, FTIR_Y_Units, FTIR_Axes_Scales>
{
public:
	FTIR_Graph( QWidget* parent = nullptr );
};

template< typename FloatType >
constexpr FloatType Convert_Units( FTIR_X_Units original, FTIR_X_Units converted, FloatType input )
{
	if( original == converted )
		return input;
	switch( original )
	{
	case FTIR_X_Units::WAVE_NUMBER:
		switch( converted )
		{
			case FTIR_X_Units::WAVELENGTH_METERS:
				return 1.0E-2 / input;
			break;
			case FTIR_X_Units::WAVELENGTH_MICRONS:
				return 10000.0 / input;
			break;
			case FTIR_X_Units::ENERGY_EV:
				return 1.23984198406e-4 * input;
			break;
		};
	break;
	case FTIR_X_Units::WAVELENGTH_MICRONS:
		switch( converted )
		{
			case FTIR_X_Units::WAVELENGTH_METERS:
				return 1.0E-6 * input;
			break;
			case FTIR_X_Units::WAVE_NUMBER:
				return 10000.0 / input;
			break;
			case FTIR_X_Units::ENERGY_EV:
				return 1.23984198406 / input;
			break;
		};
	break;
	case FTIR_X_Units::WAVELENGTH_METERS:
		switch( converted )
		{
			case FTIR_X_Units::WAVELENGTH_MICRONS:
				return 1.0E6 * input;
			break;
			case FTIR_X_Units::WAVE_NUMBER:
				return 1.0E-2 / input;
			break;
			case FTIR_X_Units::ENERGY_EV:
				return 1.23984198406E-6 / input;
			break;
		};
	break;
	case FTIR_X_Units::ENERGY_EV:
		switch( converted )
		{
			case FTIR_X_Units::WAVELENGTH_METERS:
				return 1.23984198406E-6 / input;
			break;
			case FTIR_X_Units::WAVE_NUMBER:
				return input / 1.23984198406e-4;
			break;
			case FTIR_X_Units::WAVELENGTH_MICRONS:
				return 1.23984198406 / input;
			break;
		};
	break;
	};

	throw "Error with datatype in " __FUNCTION__;
	//return -INFINITY;
}

