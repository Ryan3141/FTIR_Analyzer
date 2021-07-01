#pragma once

#include <tuple>
#include <QVector>
#include <functional>

#include "Interactive_Graph.h"


namespace FTIR
{

enum class X_Units
{
	WAVE_NUMBER = 0,
	WAVELENGTH_MICRONS = 1,
	ENERGY_EV = 2,
	WAVELENGTH_METERS = 3
};

enum class Y_Units
{
	RAW_SENSOR = 0,
	TRANSMISSION = 1,
	ABSORPTION = 2,
	ABSORPTION_DERIVATIVE = 3,
	DONT_CHANGE = 4
};

struct Axes_Scales
{
	using XY_Data = std::tuple< QVector<double>, QVector<double> >;

	const static X_Units default_x_units = FTIR::X_Units::WAVELENGTH_MICRONS;
	const static Y_Units default_y_units = FTIR::Y_Units::RAW_SENSOR;
	X_Units x_units = default_x_units;
	Y_Units y_units = default_y_units;
	std::function<void()> graph_function;

	FTIR::Axes_Scales( std::function<void()> regraph_function ) : graph_function( regraph_function )
	{
	}

	void Set_X_Units( FTIR::X_Units units );
	void Set_Y_Units( FTIR::Y_Units units );

	void Set_As_Background( XY_Data xy );

	std::tuple< QVector<double>, QVector<double> > Prepare_XY_Data( const Single_Graph<FTIR::X_Units, FTIR::Y_Units> & graph_data ) const;

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

class Interactive_Graph :
	public ::Interactive_Graph<FTIR::X_Units, FTIR::Y_Units, FTIR::Axes_Scales>
{
public:
	Interactive_Graph( QWidget* parent = nullptr );
};

template< typename FloatType >
constexpr FloatType Convert_Units( FTIR::X_Units original, FTIR::X_Units converted, FloatType input )
{
	if( original == converted )
		return input;
	switch( original )
	{
	case FTIR::X_Units::WAVE_NUMBER:
		switch( converted )
		{
			case FTIR::X_Units::WAVELENGTH_METERS:
				return 1.0E-2 / input;
			break;
			case FTIR::X_Units::WAVELENGTH_MICRONS:
				return 10000.0 / input;
			break;
			case FTIR::X_Units::ENERGY_EV:
				return 1.23984198406e-4 * input;
			break;
		};
	break;
	case FTIR::X_Units::WAVELENGTH_MICRONS:
		switch( converted )
		{
			case FTIR::X_Units::WAVELENGTH_METERS:
				return 1.0E-6 * input;
			break;
			case FTIR::X_Units::WAVE_NUMBER:
				return 10000.0 / input;
			break;
			case FTIR::X_Units::ENERGY_EV:
				return 1.23984198406 / input;
			break;
		};
	break;
	case FTIR::X_Units::WAVELENGTH_METERS:
		switch( converted )
		{
			case FTIR::X_Units::WAVELENGTH_MICRONS:
				return 1.0E6 * input;
			break;
			case FTIR::X_Units::WAVE_NUMBER:
				return 1.0E-2 / input;
			break;
			case FTIR::X_Units::ENERGY_EV:
				return 1.23984198406E-6 / input;
			break;
		};
	break;
	case FTIR::X_Units::ENERGY_EV:
		switch( converted )
		{
			case FTIR::X_Units::WAVELENGTH_METERS:
				return 1.23984198406E-6 / input;
			break;
			case FTIR::X_Units::WAVE_NUMBER:
				return input / 1.23984198406e-4;
			break;
			case FTIR::X_Units::WAVELENGTH_MICRONS:
				return 1.23984198406 / input;
			break;
		};
	break;
	};

	throw "Error with datatype in " __FUNCTION__;
	//return -INFINITY;
}

}