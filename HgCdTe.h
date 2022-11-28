#pragma once

#include <armadillo>
#include <cppad/ipopt/solve.hpp>
#include <fstream>

#include "Thin_Film_Interference.h"

namespace HgCdTe
{

//template< typename Real, typename Real2, std::enable_if_t< (std::is_floating_point_v<Real> || std::is_integral_v<Real>)
//														&& (std::is_floating_point_v<Real2> || std::is_integral_v<Real2>), bool> = true >
//struct Deduce_Types
//{
//	using Return = Real;
//};
//
//template< typename Real, typename Real2, std::enable_if_t<(std::is_convertible<Real, arma::vec>::value || std::is_convertible<Real2, arma::vec>::value), bool> = true>
//struct Deduce_Types
//{
//	using Return = arma::vec;
//};

//template< typename Real, typename Real2 >
//struct Deduce_Types
//{
//	using Return = Real;
//};
//
//template< typename Real >
//struct Deduce_Types< Real, arma::vec >
//{
//	using Return = arma::vec;
//};
//
//template< typename Real >
//struct Deduce_Types< arma::vec, Real >
//{
//	using Return = arma::vec;
//};


template< typename Real, std::enable_if_t<std::is_convertible<Real, arma::vec>::value, bool> = true >
arma::vec pow_( Real && base, double power )
{
	return arma::pow( std::forward<Real>( base ), power );
}
template<typename Real, std::enable_if_t<std::is_floating_point_v<Real> || std::is_integral_v<Real>, bool> = true>
Real pow_( Real base, double power )
{
	return std::pow( base, power );
}
template<int power>
constexpr double pow_( double base )
{
	return base * pow_< power - 1 >( base );
}

template<>
constexpr double pow_<0>( double base )
{
	return 1.0;
}


template<typename Real, std::enable_if_t<std::is_floating_point_v<Real> || std::is_integral_v<Real>, bool> = true>
Real exp_( Real power )
{
	return std::exp( power );
}
template< typename Real, std::enable_if_t<std::is_convertible<Real, arma::vec>::value, bool> = true >
arma::vec exp_( Real && power )
{
	return arma::exp( std::forward<Real>( power ) );
}

template< typename Real, typename Real2 >
arma::vec E_g_Hansen( const Real & x, const Real2 & T )
{
	return -0.302 + 1.930 * x + 5.35E-4 * T * (1 - 2 * x) - 0.810 * x * x + 0.832 * x * x * x;
}
//template< typename Real, typename Real2, std::enable_if_t<std::is_convertible<Real, arma::vec>::value, bool> = true >
//arma::vec E_g_Hansen( const Real& x, const Real2& T )
//{
//	return -0.302 + 1.930 * x + 5.35E-4 * T * (1 - 2 * x) - 0.810 * x % x + 0.832 * x % x % x;
//}

template< typename Real, typename Real2, std::enable_if_t< (std::is_floating_point_v<Real> || std::is_integral_v<Real>)
														&& (std::is_floating_point_v<Real2> || std::is_integral_v<Real2>), bool> = true >
arma::vec E_g_Laurenti( const Real& x, const Real2& T )
{
	return -0.303 * (1 - x) + 1.606 * x - 0.132 * x * (1 - x) + (6.3 * (1 - x) - 3.25 * x - 5.92 * x * (1 - x)) * 1E-4 * T * T / (11 * (1 - x) + 78.7 * x + T);
}

template< typename Real, typename Real2, std::enable_if_t<std::is_convertible<Real2, arma::vec>::value && !std::is_convertible<Real, arma::vec>::value, bool> = true >
arma::vec E_g_Laurenti( const Real& x, const Real2& T )
{
	return -0.303 * (1 - x) + 1.606 * x - 0.132 * x * (1 - x) + (6.3 * (1 - x) - 3.25 * x - 5.92 * x * (1 - x)) * 1E-4 * T % T / (11 * (1 - x) + 78.7 * x + T);
}

template< typename Real, typename Real2, std::enable_if_t<std::is_convertible<Real, arma::vec>::value && !std::is_convertible<Real2, arma::vec>::value, bool> = true >
arma::vec E_g_Laurenti( const Real& x, const Real2& T )
{
	return -0.303 * (1 - x) + 1.606 * x - 0.132 * x % (1 - x) + (6.3 * (1 - x) - 3.25 * x - 5.92 * x % (1 - x)) * 1E-4 * pow_( T, 2 ) / (11 * (1 - x) + 78.7 * x + T);
}

template< typename Real = double >
typename arma::cx_vec Refractive_Index( const arma::vec& wavelengths, Optional_Material_Parameters optional_parameters )
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

template< typename Real >
Real Dielectric_Relative_Constant( const Real & Cd_composition )
{
	// From : https ://doi.org/10.1063/1.5000116
	const auto & x = Cd_composition;
	return 20.5 - 15.6 * x + 5.7 * x * x;
}

template< typename Real >
Real Dielectric_Relative_Constant_Infinite( const Real & Cd_composition )
{
	// From : https ://doi.org/10.1063/1.5000116
	const auto & x = Cd_composition;
	// return 15.2 - 15.6 * x + 8.2 * x * x;
	return 15.2 - 13.7 * x + 5.4 * x * x;
}

template< typename Real >
Real Light_Hole_Relative_Effective_Mass( const Real& Cd_composition, const Real& temperature_in_K )
{
	return Electron_Relative_Effective_Mass( Cd_composition, temperature_in_K );
}

inline arma::vec Heavy_Hole_Relative_Effective_Mass( const arma::vec & Cd_composition, double temperature_in_K )
{
	arma::vec all_same = arma::vec( Cd_composition.size() );
	all_same.fill( 0.55 );
	return all_same;
}

inline arma::vec Heavy_Hole_Relative_Effective_Mass( double Cd_composition, const arma::vec & temperature_in_K )
{
	arma::vec all_same = arma::vec( temperature_in_K.size() );
	all_same.fill( 0.55 );
	return all_same;
}

inline double Heavy_Hole_Relative_Effective_Mass( double Cd_composition, double temperature_in_K )
{
	return 0.55;
}

template< typename Real, typename Real2 >
arma::vec Electron_Relative_Effective_Mass( const Real & Cd_composition, const Real2 & temperature_in_K ) // # in cm ^ -3 / s
{
	using RReal = arma::vec;
	// From : https ://iopscience.iop.org/article/10.1088/0268-1242/8/6S/005/meta
	RReal E_g = E_g_Hansen<Real, Real2>( Cd_composition, temperature_in_K );
	const double F = -0.8;
	const double delta = 1.0; // eV
	const double E_p = 19; // eV
	RReal m_e_rel = 1.0 / (1.0 + 2 * F + E_p / 3 * (2 / E_g + 1 / (E_g + delta)));
	return m_e_rel;
}

template< typename Real, typename Real2 >
arma::vec Intrinsic_Carrier_Concentration( const Real& Cd_composition, const Real2& temperature_in_K )
{
	// From : https ://doi.org/10.1063/1.5000116
	arma::vec E_g = E_g_Hansen<Real, Real2>( Cd_composition, temperature_in_K );
	double x = Cd_composition;
	double x2 = x * x;
	arma::vec T = temperature_in_K;
	arma::vec T2 = T % T;

	arma::vec n_i = ((5.24256 - 3.5729 * x - 4.74019E-4 * T + 1.25942E-2 * x * T - 5.77046 * x2 - 4.24123E-6 * T2)
		* 1E14) % pow_( E_g, 0.75 ) % pow_( T, 1.5 ) % exp_( -E_g / (2 * arma::datum::k_evk * T) );
	return n_i;
}

arma::vec Auger1_to_7_Lifetime_Ratio( double Cd_composition, const arma::vec & temperature_in_K ); // in cm^-3 / s

template< typename Real, typename Real2 >
arma::vec Auger_Intrinsic_Lifetimes( const Real& Cd_composition, const Real2& temperature_in_K ) // # in cm ^ -3 / s
{
	using RReal = arma::vec;
	constexpr auto k_B = arma::datum::k / arma::datum::eV;
	RReal m_e_rel = Electron_Relative_Effective_Mass( Cd_composition, temperature_in_K );
	RReal m_hh_rel = Heavy_Hole_Relative_Effective_Mass( Cd_composition, temperature_in_K );
	constexpr double m_e = arma::datum::m_e * pow_<2>( arma::datum::c_0 ) / arma::datum::eV;
	RReal μ = m_e_rel / m_hh_rel;
	RReal E_g = E_g_Hansen<Real, Real2>( Cd_composition, temperature_in_K );
	//RReal E_g = E_g_Laurenti<Real, Real2>( Cd_composition, temperature_in_K );
	// E_g = HgCdTe.Bandgap_eV( Cd_composition, temperature_in_K ) - HgCdTe.Ec_minus_Ei( Cd_composition, temperature_in_K )
	Real ε = Dielectric_Relative_Constant_Infinite( Cd_composition );
	const double F1F2 = 0.15; // Overlap integral of the Bloch functions typically between 0.1 and 0.3

	// D = (sc.h) * *3 * sc.epsilon_0 / (2 * *(3 / 2) * sc.pi * *0.5 * sc.elementary_charge * *4 * sc.m_e)
	const double C = pow_<3>( arma::datum::h ) * pow_<2>( arma::datum::eps_0 )
		/ (std::pow( 8 * arma::datum::pi, 0.5 ) * pow_<4>( arma::datum::ec ) * arma::datum::m_e);
	// C / 2 ~3.8E-18

	const auto& T = temperature_in_K;
	RReal τ_A1_i = (C / 2) * pow_<2>( ε ) * pow_( 1 + μ, 0.5 ) % (1 + 2 * μ)
		% pow_( E_g / (k_B * T), 1.5 ) * 1 / m_e_rel
		% exp_( (1 + 2 * μ) / (1 + μ) % E_g / (k_B * T) ) * pow_( F1F2, -2 );
	//Real τ_A7_i = τ_A1_i * Auger_7to1_Ratio( Cd_composition, temperature_in_K );

	//return { τ_A1_i, τ_A7_i };
	return τ_A1_i;
}

template< typename Real, typename Real2 >
arma::vec Radiative_Recombination_Rate( const Real& Cd_composition, const Real2& temperature_in_K ) // in m^3 / s
{
	using RReal = arma::vec;
	arma::vec T = temperature_in_K;
	constexpr auto k_B = arma::datum::k / arma::datum::eV;
	RReal m_e_rel = Electron_Relative_Effective_Mass( Cd_composition, temperature_in_K );
	RReal m_hh_rel = Heavy_Hole_Relative_Effective_Mass( Cd_composition, temperature_in_K );
	// m_e = sc.electron_mass
	RReal E_g = E_g_Hansen<Real, Real2>( Cd_composition, temperature_in_K );
	Real ε = Dielectric_Relative_Constant_Infinite( Cd_composition );

	arma::vec recombination = 5.8E-13 * pow_( ε, 0.5 ) * pow_( 1 / (m_e_rel + m_hh_rel), 1.5 )
		% (1 + 1 / m_e_rel + 1 / m_hh_rel)
		% pow_(300 / T, 1.5) % ( pow_(E_g, 2) + 3 * k_B * T % E_g + 3.75 * pow_(k_B * T, 2) );

	// C = 8 * np.pi / (sc.h**3 * sc.c**2) \
	// 	* 2**(2/3) / (3 * sc.epsilon_0**0.5) \
	// 	* sc.m_e * sc.elementary_charge**2 / sc.hbar**2

	// recombination2 = 1E6 * 8.685E28 * ε**0.5 * (1/m_e_rel + 1/m_hh_rel)**-1.5 \
	// 				* (1 + 1/m_e_rel + 1/m_hh_rel) * np.exp( -E_g / (k_B * T) ) \
	// 				* (k_B * T)**1.5 * (E_g**2 + 3 * k_B * T * E_g + 3.75 * (k_B * T)**2)

	return recombination;
}

template< typename Real, typename Real2, typename Real3 >
arma::vec Auger1_Lifetime( const Real& Cd_composition, const Real2& temperature_in_K, const Real3& N_d ) // # in cm ^ -3 / s
{
	arma::vec n_i = Intrinsic_Carrier_Concentration( Cd_composition, temperature_in_K ) * 1E6;
	arma::vec τ_A1_i = Auger_Intrinsic_Lifetimes( Cd_composition, temperature_in_K );
	arma::vec τ_A1 = 2 * pow_( n_i, 2 ) % τ_A1_i / ((N_d + pow_( n_i, 2 ) / N_d) * N_d);
	return τ_A1;
}

template< typename Real, typename Real2, typename Real3 >
arma::vec Auger7_Lifetime( const Real& Cd_composition, const Real2& temperature_in_K, const Real3& N_d ) // # in cm ^ -3 / s
{
	arma::vec n_i = Intrinsic_Carrier_Concentration( Cd_composition, temperature_in_K ) * 1E6;
	arma::vec τ_A7_i = Auger_Intrinsic_Lifetimes( Cd_composition, temperature_in_K ) % Auger1_to_7_Lifetime_Ratio( Cd_composition, temperature_in_K );
	arma::vec N_a = pow_( n_i, 2 ) / N_d;
	arma::vec τ_A7 = 2 * pow_( n_i, 2 ) % τ_A7_i / ((N_d + N_a) % N_a);
	return τ_A7;
}

template< typename Real, typename Real2, typename Real3 >
arma::vec Radiative_Lifetime( const Real& Cd_composition, const Real2& temperature_in_K, const Real3& N_d )
{
	arma::vec n_i = Intrinsic_Carrier_Concentration( Cd_composition, temperature_in_K ) * 1E6;
	arma::vec G_R = Radiative_Recombination_Rate( Cd_composition, temperature_in_K );
	arma::vec τ_R = 1E6 / ((N_d + pow_( n_i, 2 ) / N_d) % G_R);
	return τ_R;
}

template< bool is_n_not_p, typename Real >
arma::vec SRH_Lifetimes( double Cd_composition, const Real & temperature_in_K, double N_d,
	double N_t, double E_t_E_g_ratio, double τ_n0, double τ_p0 )
{
	arma::vec n_i = Intrinsic_Carrier_Concentration( Cd_composition, temperature_in_K ) * 1E6;
	double x = Cd_composition;
	const auto& T = temperature_in_K;
	double n_0 = N_d;
	arma::vec p_0 = arma::square( n_i ) / N_d;
	// N_t = 1E15 * 1E6
	const auto π = arma::datum::pi;
	const auto k_B = arma::datum::k;
	const auto m_e = arma::datum::m_e;
	const auto h = arma::datum::h;
	const auto eV = arma::datum::eV;
	arma::vec E_g = E_g_Hansen<double, Real>( Cd_composition, temperature_in_K );
	arma::vec E_t = E_t_E_g_ratio * E_g;
	arma::vec N_c = 2 * pow_( (2 * π * Electron_Relative_Effective_Mass( x, T ) * m_e * k_B % T / pow_( h, 2 )), 1.5 );
	arma::vec N_v = 2 * pow_( (2 * π * Heavy_Hole_Relative_Effective_Mass( x, T ) * m_e * k_B % T / pow_( h, 2 )), 1.5 );
	arma::vec n_1 = N_c % exp_( -E_t / (k_B / eV * T) );
	arma::vec p_1 = N_v % exp_( -(E_g - E_t) / (k_B / eV * T) );
	if constexpr( is_n_not_p )
	{
		arma::vec τ_SRH_n = (τ_p0 * (n_0 + n_1) + τ_n0 * (p_0 + p_1) + τ_n0 * N_t / (1 + p_0 / p_1))
			/ (n_0 + p_0 + N_t / (1 + p_0 / p_1) / (1 + p_1 / p_0));
		return τ_SRH_n;
	}
	else
	{
		arma::vec τ_SRH_p = (τ_n0 * (p_0 + p_1) + τ_p0 * (n_0 + n_1) + τ_p0 * N_t / (1 + n_0 / n_1))
			/ (n_0 + p_0 + N_t / (1 + n_0 / n_1) / (1 + n_1 / n_0));
		return τ_SRH_p;
	}
}

using ADCVector = CPPAD_TESTVECTOR( CppAD::AD< std::complex<double> > );
using ADVector = CPPAD_TESTVECTOR( CppAD::AD< double > );
inline ADCVector Refractive_Index_AD( const arma::vec& wavelengths, Optional_Material_Parameters_AD optional_parameters )
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
			output[ i ] = CppAD::sqrt( A1 + B1 / (1 - C1 * C1 / (wavelengths[ i ] * wavelengths[ i ])) + D1 * wavelengths[ i ] * wavelengths[ i ] );
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
					alpha[ i ] = alpha_g * CppAD::exp( CppAD::sqrt( beta * (E[ i ] - Eg) ) );
				}
				else
				{ // Urbach region
					Real E_0 = -0.355 + 1.77 * x;
					Real alpha_g = -65 + 1.88 * T + (8694 - 10.31 * T) * x;
					Real log_alpha_0 = -18.5 + 45.68 * x;
					Real W = (Eg - E_0) / (CppAD::log( alpha_g ) - log_alpha_0);
					//Real test = (E - E_0) / W;
					alpha[ i ] = CppAD::exp( log_alpha_0 + (E[ i ] - E_0) / W );
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
						output[ i ] = CppAD::exp( log_of_alpha0 + (E[ i ] - optional_parameters.tauts_gap_eV.value()) / optional_parameters.urbach_energy_eV.value() );
					else
						output[ i ] = CppAD::exp( log_of_alpha0 + sigma * (E[ i ] - E0) / W );
				return output;
			}();

			ADVector output = ADVector( E.size() );
			for( int i = 0; i < output.size(); i++ )
				output[ i ] = ((alpha_urbach[ i ] < alpha_kane[ i ]) ? alpha_urbach[ i ] : alpha_kane[ i ])
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

} // End namespace HgCdTe