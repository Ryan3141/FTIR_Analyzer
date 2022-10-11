#pragma once

#include <armadillo>
#include <cppad/ipopt/solve.hpp>

template< typename Real = double >
typename arma::cx_vec MCT_Index( const arma::vec& wavelengths, Optional_Material_Parameters optional_parameters )
{
	Real x = optional_parameters.composition.value();
	Real temperature_k = optional_parameters.temperature.value();
	typename arma::vec n = [&]() -> typename arma::vec
	{
		Real x_2 = x * x;
		Real A1 = 13.173 - 9.852 * x + 2.909 * x_2 + 1E-3 * (300 - temperature_k);
		Real B1 = 0.83 - 0.246 * x - 0.0961 * x_2 + 8E-4 * (300 - temperature_k);
		Real C1 = 6.706 - 14.437 * x + 8.531 * x_2 + 7E-4 * (300 - temperature_k);
		Real D1 = 1.953E-4 - 0.00128 * x + 1.853E-4 * x_2;

		return arma::sqrt( A1 + B1 / (1 - C1 * C1 / arma::square( wavelengths )) + D1 * arma::square( wavelengths ) );
	}();

	typename arma::vec k = [&]() -> arma::vec
	{
		//std::ofstream save_file( "Look at crossover.txt" );
		//for( Real x : arma::linspace( 0.0, 1.0, 1001 ) )
		if constexpr( false )
		{ // Equation from Chu et al.
			arma::vec E = 1.23984198406E-6 / wavelengths;
			Real T = temperature_k;
			//Real Eg = -0.302 + 1.93 * x - 0.81 * x * x + 0.832 * x * x * x + 5.35E-4 * T * (1 - 2 * x); # Mentioned in Rogalski
			Real Eg = -0.295 + 1.87 * x - 0.28 * x * x + (6 - 14 * x + 3 * x * x * x) * 1E-4 * T + 0.35 * x * x * x * x;
			Real alpha_g = -65 + 1.88 * T + (8694 - 10.31 * T) * x;
			typename arma::vec alpha = E;
			alpha.transform( [&]( Real E )
				{
					if( E > Eg )
					{ // Kane region
						Real beta = -1.0 + 0.083 * T + (21.0 - 0.13 * T) * x;
						alpha = alpha_g * std::exp( std::sqrt( beta * (E - Eg) ) );
					}
					else
					{ // Urbach region
						Real E_0 = -0.355 + 1.77 * x;
						Real alpha_g = -65 + 1.88 * T + (8694 - 10.31 * T) * x;
						Real log_alpha_0 = -18.5 + 45.68 * x;
						Real W = (Eg - E_0) / (std::log( alpha_g ) - log_alpha_0);
						Real test = (E - E_0) / W;
						alpha = arma::exp( log_alpha_0 + (E - E_0) / W );
					}
					return alpha * wavelengths * 1E6 / (4 * arma::datum::pi);
				} );
		}
		else if constexpr( true )
		{ // Before double checking sources
			typename arma::vec E = 1.23984198406E-6 / wavelengths;

			Real T0 = 81.9;//61.9;  // Initial parameter is 81.9.Adjusted.
			Real W = T0 + temperature_k;
			Real E0 = -0.3424 + 1.838 * x + 0.148 * x * x;
			Real sigma = 3.267E4 * (1 + x);
			Real log_of_alpha0 = 53.61 * x - 18.88;


			//Real beta = 3.109E5 * std::sqrt( (1 + x) / W );  // Initial parameter is 2.109E5.Adjusted.
			//Real Eg = E0 + (6.29E-2 + 7.68E-4 * temperature_k) * ((1 - 2.14 * x) / (1 + x));
			Real Eg = -0.302 + 1.93 * x - 0.81 * x * x + 0.832 * x * x * x + 5.35E-4 * temperature_k * (1 - 2 * x);
			auto alpha_kane = [&]() -> arma::vec
				//if( E >= Eg )
			{ // Kane region
				Real beta = -1.0 + 0.083 * temperature_k + (21.0 - 0.13 * temperature_k) * x;
				//Real beta = std::log( 2.109E5 * std::sqrt( (1 + x) / (81.9 + temperature_k) ) ); // Testing this version
				//Real log_of_alpha_g = log_of_alpha0 + sigma * (Eg - E0) / W; // When it switches from Urbach to Kane region at bandgap Eg
				Real alpha_g = -65 + 1.88 * temperature_k + (8694 - 10.31 * temperature_k) * x;
				return alpha_g * arma::exp( arma::sqrt( beta * (E - Eg) ) );
				//Real log_of_alpha_g = std::log( alpha_g );
				//return arma::exp( log_of_alpha_g + arma::sqrt( beta * (E - Eg) ) );
			}();
			//else
			auto alpha_urbach = [&]() -> typename arma::vec
			{ // Urbach tail
				if( optional_parameters.tauts_gap_eV.has_value() && optional_parameters.urbach_energy_eV.has_value() )
					return arma::exp( log_of_alpha0 + (E - optional_parameters.tauts_gap_eV.value()) / optional_parameters.urbach_energy_eV.value() );
				else
					return arma::exp( log_of_alpha0 + sigma * (E - E0) / W );
			}();
			//arma::vec test = arma::min( arma::vec{ arma::datum::nan, 1.0, 5 }, arma::vec{ -1.0, arma::datum::nan, -29 } );
			//Real debug1 = test[ 0 ];
			//Real debug2 = test[ 1 ];
			//Real debug3 = test[ 2 ];
			// alpha_urbach must come first in case alpha_kane is nan
			alpha_kane.replace( arma::datum::nan, arma::datum::inf );
			return arma::min( alpha_urbach, alpha_kane ) % wavelengths * 1E2 / (4 * arma::datum::pi); // Convert wavelengths from m to cm because alphas are per cm
		}
		//else if constexpr( false )
		//{
		//	Real T0 = 61.9;  // Initial parameter is 81.9.Adjusted.
		//	Real W = T0 + temperature_k;
		//	Real E0 = -0.3424 + 1.838 * x + 0.148 * x * x * x * x;
		//	Real sigma = 3.267E4 * (1 + x);
		//	Real alpha0 = std::exp( 53.61 * x - 18.88 );
		//	Real beta = 3.109E5 * std::sqrt( (1 + x) / W );  // Initial parameter is 2.109E5.Adjusted.
		//	Real Eg = E0 + (6.29E-2 + 7.68E-4 * temperature_k) * ((1 - 2.14 * x) / (1 + x));

		//	Real E = 4.13566743 * 3 / 10 / wavelength;
		//	Real ab1 = alpha0 * std::exp( sigma * (E - E0) / W );
		//	Real ab2 = 0;
		//	if( E >= Eg )
		//		ab2 = beta * std::sqrt( E - Eg );

		//	//if( ab1 < crossover_a && ab2 < crossover_a )
		//	//	return ab1 / 4 / datum::pi * wavelength / 10000;
		//	//else
		//	//{
		//	//	if( ab2 != 0 )
		//	//		return ab2 / 4 / datum::pi * wavelength / 10000;
		//	//	else
		//	//		return ab1 / 4 / datum::pi * wavelength / 10000;
		//	//}
		//	k = ab2 / 4 / datum::pi * wavelength / 10000;
		//}
	}();

	for( auto& y : k )
		if( !std::isfinite( y ) || y > 1.0E9 )
			y = 1.0E9;

	return arma::cx_vec( n, k );
}

using ADCVector = CPPAD_TESTVECTOR( CppAD::AD< std::complex<double> > );
using ADVector = CPPAD_TESTVECTOR( CppAD::AD< double > );
ADCVector MCT_Index_AD( const arma::vec& wavelengths, Optional_Material_Parameters_AD optional_parameters )
{
	using Real = CppAD::AD< double >;
	Real x = optional_parameters.composition.value();
	Real temperature_k = optional_parameters.temperature.value();
	ADVector n = [&]() -> ADVector
	{
		Real x_2 = x * x;
		Real A1 = 13.173 - 9.852 * x + 2.909 * x_2 + 1E-3 * (300 - temperature_k);
		Real B1 = 0.83 - 0.246 * x - 0.0961 * x_2 + 8E-4 * (300 - temperature_k);
		Real C1 = 6.706 - 14.437 * x + 8.531 * x_2 + 7E-4 * (300 - temperature_k);
		Real D1 = 1.953E-4 - 0.00128 * x + 1.853E-4 * x_2;
		ADVector output;
		for( int i = 0; i < output.size(); i++ )
			output[ i ] = CppAD::sqrt( A1 + B1 / (1 - C1 * C1 / ( wavelengths[ i ] * wavelengths[ i ] )) + D1 * wavelengths[ i ] * wavelengths[ i ] );
		return output;
	}();

	ADVector k = [&]() -> ADVector
	{
		//std::ofstream save_file( "Look at crossover.txt" );
		//for( Real x : arma::linspace( 0.0, 1.0, 1001 ) )
		if constexpr( false )
		{ // Equation from Chu et al.
			arma::vec E = 1.23984198406E-6 / wavelengths;
			Real T = temperature_k;
			//Real Eg = -0.302 + 1.93 * x - 0.81 * x * x + 0.832 * x * x * x + 5.35E-4 * T * (1 - 2 * x); # Mentioned in Rogalski
			Real Eg = -0.295 + 1.87 * x - 0.28 * x * x + (6 - 14 * x + 3 * x * x * x) * 1E-4 * T + 0.35 * x * x * x * x;
			Real alpha_g = -65 + 1.88 * T + (8694 - 10.31 * T) * x;
			ADVector alpha = ADVector( E.size() );
			for( int i = 0; i < alpha.size(); i++ )
			{
					if( E[ i ] > Eg )
					{ // Kane region
						Real beta = -1.0 + 0.083 * T + (21.0 - 0.13 * T) * x;
						alpha[ i ] = alpha_g * CppAD::exp(CppAD::sqrt(beta * (E[i] - Eg)));
					}
					else
					{ // Urbach region
						Real E_0 = -0.355 + 1.77 * x;
						Real alpha_g = -65 + 1.88 * T + (8694 - 10.31 * T) * x;
						Real log_alpha_0 = -18.5 + 45.68 * x;
						Real W = (Eg - E_0) / (CppAD::log( alpha_g ) - log_alpha_0);
						//Real test = (E - E_0) / W;
						alpha[ i ] = CppAD::exp(log_alpha_0 + (E[ i ] - E_0) / W);
					}
					alpha[ i ] = alpha[ i ] * wavelengths[ i ] * 1E6 / (4 * arma::datum::pi);
			}
		}
		else if constexpr( true )
		{ // Before double checking sources
			arma::vec E = 1.23984198406E-6 / wavelengths;

			Real T0 = 81.9;//61.9;  // Initial parameter is 81.9.Adjusted.
			Real W = T0 + temperature_k;
			Real E0 = -0.3424 + 1.838 * x + 0.148 * x * x;
			Real sigma = 3.267E4 * (1 + x);
			Real log_of_alpha0 = 53.61 * x - 18.88;


			//Real beta = 3.109E5 * std::sqrt( (1 + x) / W );  // Initial parameter is 2.109E5.Adjusted.
			//Real Eg = E0 + (6.29E-2 + 7.68E-4 * temperature_k) * ((1 - 2.14 * x) / (1 + x));
			Real Eg = -0.302 + 1.93 * x - 0.81 * x * x + 0.832 * x * x * x + 5.35E-4 * temperature_k * (1 - 2 * x);
			auto alpha_kane = [&]() -> ADVector
				//if( E >= Eg )
			{ // Kane region
				Real beta = -1.0 + 0.083 * temperature_k + (21.0 - 0.13 * temperature_k) * x;
				//Real beta = std::log( 2.109E5 * std::sqrt( (1 + x) / (81.9 + temperature_k) ) ); // Testing this version
				//Real log_of_alpha_g = log_of_alpha0 + sigma * (Eg - E0) / W; // When it switches from Urbach to Kane region at bandgap Eg
				Real alpha_g = -65 + 1.88 * temperature_k + (8694 - 10.31 * temperature_k) * x;
				ADVector output = ADVector( wavelengths.size() );
				for( int i = 0; i < output.size(); i++ )
					output[ i ] = alpha_g * CppAD::exp( CppAD::sqrt( beta * (E[ i ] - Eg) ) );
				return output;
				//Real log_of_alpha_g = std::log( alpha_g );
				//return arma::exp( log_of_alpha_g + arma::sqrt( beta * (E - Eg) ) );
			}();
			//else
			auto alpha_urbach = [&]() -> ADVector
			{ // Urbach tail
				ADVector output = ADVector( E.size() );
				for( int i = 0; i < output.size(); i++ )
					if( optional_parameters.tauts_gap_eV.has_value() && optional_parameters.urbach_energy_eV.has_value() )
						output[ i ] = CppAD::exp( log_of_alpha0 + (E[ i ] - optional_parameters.tauts_gap_eV.value()) / optional_parameters.urbach_energy_eV.value());
					else
						output[ i ] = CppAD::exp( log_of_alpha0 + sigma * (E[ i ] - E0) / W);
				return output;
			}();

			ADVector output = ADVector( E.size() );
			for( int i = 0; i < output.size(); i++ )
				output[ i ] = ( (alpha_urbach[ i ] < alpha_kane[ i ]) ? alpha_urbach[ i ] : alpha_kane[ i ] )
								* wavelengths[ i ] * 1E2 / (4 * arma::datum::pi); // Convert wavelengths from m to cm because alphas are per cm
			return output;
		}
	}();
	ADCVector output = ADCVector( n.size() );
	CppAD::AD< std::complex<double> > fucker = std::complex<double>{ 2, 1 };
	CppAD::exp( fucker );
	//for( int i = 0; i < output.size(); i++ )
	//	output[ i ] = CppAD::AD< std::complex<double> >(n[i], k[i]);
	return output;
}
