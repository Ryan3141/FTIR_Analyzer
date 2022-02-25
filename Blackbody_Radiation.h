#pragma once

#include <armadillo>
#include <boost/math/special_functions/lambert_w.hpp>

#include "Optimize.h"

const double pi = arma::datum::pi;
const double ee = arma::datum::ec;
const double k_B = arma::datum::k / ee; // 8.617333262145E-5; // eV / K
const double h = (arma::datum::h / ee); // Planck's constant in (eV / s)
const double c = arma::datum::c_0;
const double c3 = c * c * c;

inline arma::vec Blackbody_Radiation( arma::vec wavelengths, double temperature_in_k, double adjust_height = 0.0 )
{
	auto frequencies = c / wavelengths;
	const double A = h / (k_B * temperature_in_k);
	if( adjust_height == 0.0 )
		adjust_height = 2 * h / c3;
	else
	{
		//auto find_maximum = [A]( double f ) { return -(3 - f * A - 3 * std::exp( -A * f )); };
		//double frequency_of_maximum = Binary_Search( find_maximum, 0.0, 1E20, 1.0, 512 );
		//adjust_height = 1.0 / ( std::pow( frequency_of_maximum, 3 ) / (std::exp( A * frequency_of_maximum ) - 1) );

		// https://www.wolframalpha.com/input/?i=%28derivative+of+x%5E5+%2F+%28exp%28+A+*+x+%29+-+1%29%29+%3D+0
		double frequency_of_maximum = (boost::math::lambert_w0( -5 / std::pow( arma::datum::e, 5 ) ) + 5) / A;
		//double test1 = find_maximum( frequency_of_maximum );
		//double test2 = find_maximum( frequency_of_maximum+1 );
		//double test3 = find_maximum( frequency_of_maximum-1 );
		adjust_height = adjust_height / ( std::pow( frequency_of_maximum, 5 ) / (std::exp( A * frequency_of_maximum ) - 1) );
	}
	return adjust_height * arma::pow( frequencies, 5 ) / (arma::exp( A * frequencies ) - 1);
}

//template<int n>
//double Integrate_x_n_exp( double x1, double x2 )
//{
//	using std;
//	return n * Integrate_x_n_exp<n - 1>( x1, x2 ) + pow( x2, n ) * exp( x2 ) - pow( x1, n ) * exp( x1 );
//}
//
//template<>
//double Integrate_x_n_exp<0>( double x1, double x2 )
//{
//	using std;
//	return exp( x2 ) - exp( x1 );
//}
//
//void test()
//{
//	return Integrate_x_n_exp<4>( 0.1, 0.2 );
//}

//template< typename FloatLike >
//constexpr auto Blackbody_Indefinite_Integral( FloatLike wavelength, double temperature_in_k )
//{ // https://www.wolframalpha.com/input/?i=integrate+x%5E5+%2F+%28exp%28x*A%29+-+1%29
//
//}
//
//template< typename FloatLike >
//constexpr auto Polylog_Indefinite_Integral( FloatLike x, FloatLike A )
//{ // https://www.wolframalpha.com/input/?i=integrate+x%5E5+%2F+%28exp%28x*A%29+-+1%29
//	using std;
//	auto x2 = x * x;
//	auto x3 = x * x2;
//	auto x4 = x2 * x2;
//	auto x5 = x3 * x2;
//	auto A2 = A * A;
//	auto A3 = A * A2;
//	auto A4 = A2 * A2;
//	auto A5 = A3 * A2;
//	auto A6 = A3 * A3;
//
//	return -1 / A6 * ( -A5 * x5 * log( 1 - exp( -A * x ) ) +
//					   5 * A4 * x4
//					   );
//}

