#pragma once

#include <armadillo>
#include <boost/math/special_functions/lambert_w.hpp>

#include "Optimize.h"

const double pi = arma::datum::pi;
const double ee = arma::datum::ec;
const double k_B = arma::datum::k / ee; // 8.617333262145E-5; // eV / K
const double h = (arma::datum::h / ee); // Planck's constant in (eV / s)
const double c = arma::datum::c_0;
const double c2 = c * c;

inline arma::vec Blackbody_Radiation( arma::vec wavelengths, double temperature_in_k, double adjust_height = 0.0 )
{
	auto frequencies = c / wavelengths;
	const double A = h / (k_B * temperature_in_k);
	if( adjust_height == 0 )
		adjust_height = 2 * pi / c2 * h;
	else
	{
		auto find_maximum = [A]( double f ) { return -(3 - f * A - 3 * std::exp( -A * f )); };
		//double frequency_of_maximum = Binary_Search( find_maximum, 0.0, 1E20, 1.0, 512 );
		//adjust_height = 1.0 / ( std::pow( frequency_of_maximum, 3 ) / (std::exp( A * frequency_of_maximum ) - 1) );

		double frequency_of_maximum = (boost::math::lambert_w0( -3 / std::pow( arma::datum::e, 3 ) ) + 3) / A;
		double test1 = find_maximum( frequency_of_maximum );
		double test2 = find_maximum( frequency_of_maximum+1 );
		double test3 = find_maximum( frequency_of_maximum-1 );
		adjust_height = adjust_height / ( std::pow( frequency_of_maximum, 3 ) / (std::exp( A * frequency_of_maximum ) - 1) );
	}
	return adjust_height * arma::pow( frequencies, 3 ) / (arma::exp( A * frequencies ) - 1);
}
