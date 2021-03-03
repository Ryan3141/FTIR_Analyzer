#pragma once


#include <armadillo>

enum class X_Unit_Type
{
	WAVE_NUMBER = 0,
	WAVELENGTH_MICRONS = 1,
	ENERGY_EV = 2,
	WAVELENGTH_METERS = 3
};

enum class Y_Unit_Type
{
	RAW_SENSOR = 0,
	TRANSMISSION = 1,
	ABSORPTION = 2,
	ABSORPTION_DERIVATIVE = 3,
	DONT_CHANGE = 4
};

template< typename FloatType >
constexpr FloatType Convert_X_Units( X_Unit_Type original, X_Unit_Type converted, FloatType input )
{
	if( original == converted )
		return input;
	switch( original )
	{
	case X_Unit_Type::WAVE_NUMBER:
		switch( converted )
		{
			case X_Unit_Type::WAVELENGTH_METERS:
				return 1.0E-2 / input;
			break;
			case X_Unit_Type::WAVELENGTH_MICRONS:
				return 10000.0 / input;
			break;
			case X_Unit_Type::ENERGY_EV:
				return 1.23984198406e-4 * input;
			break;
		};
	break;
	case X_Unit_Type::WAVELENGTH_MICRONS:
		switch( converted )
		{
			case X_Unit_Type::WAVELENGTH_METERS:
				return 1.0E-6 * input;
			break;
			case X_Unit_Type::WAVE_NUMBER:
				return 10000.0 / input;
			break;
			case X_Unit_Type::ENERGY_EV:
				return 1.23984198406 / input;
			break;
		};
	break;
	case X_Unit_Type::WAVELENGTH_METERS:
		switch( converted )
		{
			case X_Unit_Type::WAVELENGTH_MICRONS:
				return 1.0E6 * input;
			break;
			case X_Unit_Type::WAVE_NUMBER:
				return 1.0E-2 / input;
			break;
			case X_Unit_Type::ENERGY_EV:
				return 1.23984198406E-6 / input;
			break;
		};
	break;
	case X_Unit_Type::ENERGY_EV:
		switch( converted )
		{
			case X_Unit_Type::WAVELENGTH_METERS:
				return 1.23984198406E-6 / input;
			break;
			case X_Unit_Type::WAVE_NUMBER:
				return input / 1.23984198406e-4;
			break;
			case X_Unit_Type::WAVELENGTH_MICRONS:
				return 1.23984198406 / input;
			break;
		};
	break;
	};

	throw "Error with datatype in " __FUNCTION__;
	//return -INFINITY;
}
