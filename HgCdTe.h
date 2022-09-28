#pragma once

#include <armadillo>
#include <cppad/ipopt/solve.hpp>

struct arma_functions
{
	using vec = arma::vec;
	using cx_vec = arma::cx_vec;
	//	static decltype(arma::sqrt) sqrt = arma::sqrt;
//	static auto square = arma::square;
	template <typename... Args>
	static auto sqrt( Args&&... args ) -> decltype(arma::sqrt( std::forward<Args>( args )... )) {
		return arma::sqrt( std::forward<Args>( args )... );
	}
	template <typename... Args>
	static auto square( Args&&... args ) -> decltype(arma::square( std::forward<Args>( args )... )) {
		return arma::square( std::forward<Args>( args )... );
	}
	template <typename... Args>
	static auto exp( Args&&... args ) -> decltype(arma::exp( std::forward<Args>( args )... )) {
		return arma::exp( std::forward<Args>( args )... );
	}
	template <typename... Args>
	static auto min( Args&&... args ) -> decltype(arma::min( std::forward<Args>( args )... )) {
		return arma::min( std::forward<Args>( args )... );
	}
};

using ADvec = CPPAD_TESTVECTOR( CppAD::AD<double> );
struct ipopt_functions
{
	//using CppAD::AD;
	using vec    = ADvec;
	using cx_vec = CPPAD_TESTVECTOR( CppAD::AD< std::complex<double> > );

	template <typename... Args>
	static auto sqrt( Args&&... args ) -> decltype(CppAD::sqrt( std::forward<Args>( args )... )) {
		return CppAD::sqrt( std::forward<Args>( args )... );
	}
	vec sqrt( const vec& input )
	{
		vec result = vec( input.size() );
		for( int i = 0; i < input.size(); i++ )
			result[ i ] = CppAD::sqrt( input[ i ] );
		return result;
	}

	template <typename... Args>
	static auto square( Args&&... args ) -> decltype(CppAD::sqrt( std::forward<Args>( args )... )) {
		return CppAD::sqrt( std::forward<Args>( args )... );
	}
	vec square( const vec& input )
	{
		vec result = vec( input.size() );
		for( int i = 0; i < input.size(); i++ )
			result[ i ] = input[ i ] * input[ i ];
		return result;
	}
};
template< typename Vector_Like >
ADvec operator+( const ADvec& v1, const Vector_Like& v2 )
{
	ADvec result = ADvec( v1.size() );
	for( int i = 0; i < v1.size(); i++ )
		result[ i ] = v1[ i ] + v2[ i ];
	return result;
}
template< typename Vector_Like >
ADvec operator+( const Vector_Like& v1, const ADvec& v2 )
{
	ADvec result = ADvec( v1.size() );
	for( int i = 0; i < v1.size(); i++ )
		result[ i ] = v1[ i ] + v2[ i ];
	return result;
}
//template< typename Scalar_Like >
//ADvec operator+( const Scalar_Like& v1, const ADvec& v2 )
//{
//	ADvec result = ADvec( v2.size() );
//	for( int i = 0; i < v2.size(); i++ )
//		result[ i ] = v1 + v2[ i ];
//	return result;
//}
//template< typename Scalar_Like >
//ADvec operator+( const ADvec& v1, const Scalar_Like& v2 )
//{
//	ADvec result = ADvec( v1.size() );
//	for( int i = 0; i < v1.size(); i++ )
//		result[ i ] = v1[ i ] + v2;
//	return result;
//}

template< typename Vector_Like >
ADvec operator/( const ADvec& v1, const Vector_Like& v2 )
{
	ADvec result = ADvec( v1.size() );
	for( int i = 0; i < v1.size(); i++ )
		result[ i ] = v1[ i ] / v2[ i ];
	return result;
}
template< typename Vector_Like >
ADvec operator/( const Vector_Like& v1, const ADvec& v2 )
{
	ADvec result = ADvec( v1.size() );
	for( int i = 0; i < v1.size(); i++ )
		result[ i ] = v1[ i ] / v2[ i ];
	return result;
}
//template< typename Scalar_Like >
//ADvec operator/( const Scalar_Like& v1, const ADvec& v2 )
//{
//	ADvec result = ADvec( v2.size() );
//	for( int i = 0; i < v2.size(); i++ )
//		result[ i ] = v1 / v2[ i ];
//	return result;
//}
//template< typename Scalar_Like >
//ADvec operator/( const ADvec& v1, const Scalar_Like& v2 )
//{
//	ADvec result = ADvec( v1.size() );
//	for( int i = 0; i < v1.size(); i++ )
//		result[ i ] = v1[ i ] / v2;
//	return result;
//}


template< typename Real = double, typename Func_Set = arma_functions >
typename Func_Set::cx_vec MCT_Index( const arma::vec& wavelengths, Optional_Material_Parameters_Type< Real > optional_parameters )
{
	Real x = optional_parameters.composition.value();
	Real temperature_k = optional_parameters.temperature.value();
	typename Func_Set::vec n = [&]() -> typename Func_Set::vec
	{
		Real x_2 = x * x;
		Real A1 = 13.173 - 9.852 * x + 2.909 * x_2 + 1E-3 * (300 - temperature_k);
		Real B1 = 0.83 - 0.246 * x - 0.0961 * x_2 + 8E-4 * (300 - temperature_k);
		Real C1 = 6.706 - 14.437 * x + 8.531 * x_2 + 7E-4 * (300 - temperature_k);
		Real D1 = 1.953E-4 - 0.00128 * x + 1.853E-4 * x_2;

		return Func_Set::sqrt( A1 + B1 / (1 - C1 * C1 / arma::square( wavelengths )) + D1 * arma::square( wavelengths ) );
	}();

	typename Func_Set::vec k = [&]() -> Func_Set::vec
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
			typename Func_Set::vec alpha = E;
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
						alpha = Func_Set::exp( log_alpha_0 + (E - E_0) / W );
					}
					return alpha * wavelengths * 1E6 / (4 * arma::datum::pi);
				} );
		}
		else if constexpr( true )
		{ // Before double checking sources
			typename Func_Set::vec E = 1.23984198406E-6 / wavelengths;

			Real T0 = 81.9;//61.9;  // Initial parameter is 81.9.Adjusted.
			Real W = T0 + temperature_k;
			Real E0 = -0.3424 + 1.838 * x + 0.148 * x * x;
			Real sigma = 3.267E4 * (1 + x);
			Real log_of_alpha0 = 53.61 * x - 18.88;


			//Real beta = 3.109E5 * std::sqrt( (1 + x) / W );  // Initial parameter is 2.109E5.Adjusted.
			//Real Eg = E0 + (6.29E-2 + 7.68E-4 * temperature_k) * ((1 - 2.14 * x) / (1 + x));
			Real Eg = -0.302 + 1.93 * x - 0.81 * x * x + 0.832 * x * x * x + 5.35E-4 * temperature_k * (1 - 2 * x);
			auto alpha_kane = [&]() -> Func_Set::vec
				//if( E >= Eg )
			{ // Kane region
				Real beta = -1.0 + 0.083 * temperature_k + (21.0 - 0.13 * temperature_k) * x;
				//Real beta = std::log( 2.109E5 * std::sqrt( (1 + x) / (81.9 + temperature_k) ) ); // Testing this version
				//Real log_of_alpha_g = log_of_alpha0 + sigma * (Eg - E0) / W; // When it switches from Urbach to Kane region at bandgap Eg
				Real alpha_g = -65 + 1.88 * temperature_k + (8694 - 10.31 * temperature_k) * x;
				return alpha_g * Func_Set::exp( arma::sqrt( beta * (E - Eg) ) );
				//Real log_of_alpha_g = std::log( alpha_g );
				//return arma::exp( log_of_alpha_g + arma::sqrt( beta * (E - Eg) ) );
			}();
			//else
			auto alpha_urbach = [&]() -> typename Func_Set::vec
			{ // Urbach tail
				if( optional_parameters.tauts_gap_eV.has_value() && optional_parameters.urbach_energy_eV.has_value() )
					return Func_Set::exp( log_of_alpha0 + (E - optional_parameters.tauts_gap_eV.value()) / optional_parameters.urbach_energy_eV.value() );
				else
					return Func_Set::exp( log_of_alpha0 + sigma * (E - E0) / W );
			}();
			//Func_Set::vec test = arma::min( Func_Set::vec{ arma::datum::nan, 1.0, 5 }, Func_Set::vec{ -1.0, arma::datum::nan, -29 } );
			//Real debug1 = test[ 0 ];
			//Real debug2 = test[ 1 ];
			//Real debug3 = test[ 2 ];
			// alpha_urbach must come first in case alpha_kane is nan
			alpha_kane.replace( arma::datum::nan, arma::datum::inf );
			return Func_Set::min( alpha_urbach, alpha_kane ) % wavelengths * 1E2 / (4 * arma::datum::pi); // Convert wavelengths from m to cm because alphas are per cm
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

	return Func_Set::cx_vec( n, k );
}
