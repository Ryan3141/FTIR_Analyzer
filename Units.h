#pragma once


#include <armadillo>

enum class Unit_Type
{
	WAVE_NUMBER = 0,
	WAVELENGTH_MICRONS = 1,
	ENERGY_EV = 2
};

class Wavelength_Array;
class Wavenumber_Array;
class Electron_Volts_Array;
class Transmission_Array;
class Transmission_Percentage_Array;
class Raw_Transmission_Array;

class Wavelength_Array : public arma::vec
{
	explicit Wavelength_Array( const arma::vec & copy_me );
	Wavelength_Array( const Wavenumber_Array & copy_me );
	Wavelength_Array( const Electron_Volts_Array & copy_me );
};

class Wavenumber_Array : public arma::vec
{
	explicit Wavenumber_Array( const arma::vec & copy_me );
	Wavenumber_Array( const Wavelength_Array & copy_me );
	Wavenumber_Array( const Electron_Volts_Array & copy_me );
};

class Electron_Volts_Array : public arma::vec
{
	explicit Electron_Volts_Array( const arma::vec & copy_me );
	Electron_Volts_Array( const Wavelength_Array & copy_me );
	Electron_Volts_Array( const Wavenumber_Array & copy_me );
};

class Transmission_Array : public arma::vec
{
	explicit Transmission_Array( const Raw_Transmission_Array & background, const Raw_Transmission_Array & signal );
	Transmission_Array( const Transmission_Percentage_Array & copy_me );
};

class Transmission_Percentage_Array : public arma::vec
{
	explicit Transmission_Percentage_Array( const Raw_Transmission_Array & background, const Raw_Transmission_Array & signal );
	Transmission_Percentage_Array( const Transmission_Array & copy_me );
};

class Raw_Transmission_Array : public arma::vec
{
	explicit Raw_Transmission_Array( const arma::vec & copy_me );
};

template< typename FloatType >
inline FloatType Convert_Units( Unit_Type original, Unit_Type converted, FloatType input )
{
	if( original == converted )
		return input;
	switch( original )
	{
		case Unit_Type::WAVE_NUMBER:
		switch( converted )
		{
			case Unit_Type::WAVELENGTH_MICRONS:
			return 10000.0 / input;
			break;
			case Unit_Type::ENERGY_EV:
			return 1.23984198406e-4 * input;
			break;
		};
		break;
		case Unit_Type::WAVELENGTH_MICRONS:
		switch( converted )
		{
			case Unit_Type::WAVE_NUMBER:
			return 10000.0 / input;
			break;
			case Unit_Type::ENERGY_EV:
			return 1.23984198406 / input;
			break;
		};
		break;
		case Unit_Type::ENERGY_EV:
		switch( converted )
		{
			case Unit_Type::WAVE_NUMBER:
			return input / 1.23984198406e-4;
			break;
			case Unit_Type::WAVELENGTH_MICRONS:
			return 1.23984198406 / input;
			break;
		};
		break;
	};

	throw "Error with datatype in " __FUNCTION__;
	//return -INFINITY;
}
