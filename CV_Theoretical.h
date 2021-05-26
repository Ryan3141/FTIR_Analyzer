#pragma once

#include <armadillo>
#include <tuple>
#include <string>

#include "Units.h"

using namespace Units;

//const double epsilon_0 = arma::datum::eps_0; // 8.8541878128E-12; // F / m = C / (m * V)
////double k_B = 1.380649E-23; // J / K
//const double ee = arma::datum::ec; // 1.602176634E-19; // Coulombs
//const double k_B = arma::datum::k / ee; // 8.617333262145E-5; // eV / K
//const double pi = arma::datum::pi;
//const double hbar2 = (arma::datum::h_bar / ee) * (arma::datum::h_bar / ee); // Planck's constant squared in (eV / s)^2
//const double m_e = arma::datum::m_e * arma::datum::c_0 * arma::datum::c_0;

namespace CV_Measurements
{

struct Material_Constants
{
	double eps_s; // Relative dielectric constant
	Electron_Volts affinity; // electron affinity in eV
	Electron_Volts bandgap; // in eV
	double n_i; // Intrinsic carrier concentration in 1/cm^3
	double m_e_rel; // electron relative effective mass (m = m_e_rel * m_e)
	double Ec_minus_Ei; // Conduction band to intrinsic carrier energy in eV
};

struct Insulator : Material_Constants
{
	Insulator( const Material_Constants & material, double thickness, double interface_charge );
	double thickness; // Thickness in meters
	double interface_charge; // In charge per cm^2
	double capacitance; // Per area, aka units of cm^-2
};

struct Semiconductor : Material_Constants
{
	Semiconductor( const Material_Constants & material, double doping, double temperature_in_k );
	double doping; // in 1/cm^3
	Electron_Volts fermi_potential;
	Electron_Volts work_function;
};

struct Metal
{
	std::string symbol;
	Electron_Volts work_function;
};

// Voltage, Overall Capacitance, Oxide Capacitance
using CV_Data = std::tuple<arma::vec, arma::vec>;

CV_Data Get_MOS_Capacitance( const Semiconductor & semiconductor, const Insulator & insulator,
								const Metal & contact, double temperature_in_k,
								double lower_bound, double upper_bound, double frequency );



//constexpr double Passivant_Capacitance( double passivant_thickness, double relative_permittivity, double area )
//{
//	double epsilon_0 = 8.8541878128E-12; // F / m
//
//	return relative_permittivity * epsilon_0 * area / passivant_thickness;
//}
constexpr double HgCdTe_Dielectric_Constant( double cdte_fraction, double temperature_in_k )
{ // From: https://doi.org/10.1063/1.5000116
	double x = cdte_fraction;
	return 20.5 - 15.6 * x + 5.7 * x * x;
}

constexpr Electron_Volts HgCdTe_Bandgap_eV( double cdte_fraction, double temperature_in_k )
{ // From: https://doi.org/10.1063/1.5000116
	double x = cdte_fraction;
	double T = temperature_in_k;
	double T_2 = T * T;
	double T_3 = T_2 * T;
	double x_2 = x * x;
	double x_3 = x_2 * x;

	double bandgap = -0.302 + 1.93 * x - 0.813 * x_2 + 0.832 * x_3
		+ 5.35E-4 * (1 - 2 * x) * (-1882.0 + T_3) / (255.2 + T_2);
	return Electron_Volts( bandgap );
}

inline double Intrinsic_Energy_eV( double intrinsic_carrier_concentration, double electron_effective_relative_mass, double temperature_in_K )
{
	const double T = temperature_in_K;
	const double n_i = intrinsic_carrier_concentration * 1E6; // Convert from 1/cm^3 to 1/m^3
	double m = m_e * electron_effective_relative_mass / ee; // Units of eV / (m/s)^2
	//const auto debug1 = hbar.Power(2_R);
	//const auto debug2 = m * (2_R * pi * hbar.Power( 2_R ) );
	//const auto debug3 = m * k_B * T / (2_R * pi * hbar.Power( 2_R ) );
	//const auto A = 2_R * (m * k_B * T / (2_R * pi * hbar.Power( 2_R ))) ^ Rational<3, 2>();
	const auto A = 2 * std::pow(m * k_B * T / (2 * pi * hbar2), 3.0 / 2);
	double Ec_minus_Ei = -k_B * T * std::log( n_i / A );
	return Ec_minus_Ei;
}

//constexpr double HgCdTe_Bandgap_eV( double cdte_fraction, double temperature_in_k )
//{ // From book
//	double x = cdte_fraction;
//	double T = temperature_in_k;
//	double x_2 = x * x;
//	double x_3 = x_2 * x;
//
//	double bandgap = -0.302 + 1.93 * x - 0.81 * x_2 + 0.832 * x_3 + 5.35E-4 * (1 - 2 * x) * T;
//	return bandgap;
//}

constexpr Electron_Volts HgCdTe_Electron_Affinity_eV( double cdte_fraction, double temperature_in_k )
{ // From: https://doi.org/10.1063/1.5000116
	return 4.23_eV - 0.813 * (HgCdTe_Bandgap_eV( cdte_fraction, temperature_in_k ) - 0.083_eV);
}

inline double HgCdTe_Intrinsic_Carrier_Concentration( double cdte_fraction, double temperature_in_k )
{ // From: https://doi.org/10.1063/1.5000116
	double E_g = HgCdTe_Bandgap_eV( cdte_fraction, temperature_in_k );
	double x = cdte_fraction;
	double x² = x * x;
	double T = temperature_in_k;
	double T² = T * T;

	double n_i = (5.24256 - 3.5729 * x - 4.74019E-4 * T + 1.25942E-2 * x * T - 5.77046 * x² - 4.24123E-6 * T²)
		* 1E14 * std::pow( E_g, 0.75 ) * std::pow( T, 1.5 ) * std::exp( -E_g / (2 * k_B * T) );
	return std::max( 1.0, n_i );
}

//inline double HgCdTe_Intrinsic_Carrier_Concentration( double cdte_fraction, double temperature_in_k )
//{ // From book
//	double E_g = HgCdTe_Bandgap_eV( cdte_fraction, temperature_in_k );
//	double x = cdte_fraction;
//	double T = temperature_in_k;
//
//	double n_i = (5.585 - 3.82 * x + 0.001753 * T - 0.001364 * x * T)
//		* 1E14 * std::pow( E_g, 3.0 / 4 ) * std::pow( T, 3.0 / 2 ) * std::exp( -E_g / (2 * k_B * T) );
//	return n_i;
//}

inline double HgCdTe_Electron_Relative_Effective_Mass( Electron_Volts energy_gap )
{
	double E_g = energy_gap;
	double F = -0.8;
	double delta = 1.0; // eV
	double E_p = 19; // eV
	double m_e_rel = 1.0 / (1.0 + 2 * F + E_p / 3 *  (2 / E_g + 1 / (E_g + delta) ));
	return m_e_rel;
}

inline Material_Constants HgCdTe( double x, double temperature_in_K )
{
	Electron_Volts E_g = HgCdTe_Bandgap_eV( x, temperature_in_K );
	double n_i = HgCdTe_Intrinsic_Carrier_Concentration( x, temperature_in_K );
	double m_e_rel = HgCdTe_Electron_Relative_Effective_Mass( E_g );
	return { /*.eps_s =*/ HgCdTe_Dielectric_Constant( x, temperature_in_K ),
		/*.affinity =*/ HgCdTe_Electron_Affinity_eV( x, temperature_in_K ),
		/*.bandgap =*/ E_g,
		/*.n_i =*/ n_i,
		/*.m_e_rel =*/ m_e_rel,
		/*.Ec_minus_Ei =*/ Intrinsic_Energy_eV( n_i, m_e_rel, temperature_in_K ) };
}

inline Material_Constants CdTe( double temperature_in_K ) { return HgCdTe( 1.0, temperature_in_K ); };
constexpr Material_Constants ZnS{ /*.eps_s =*/ 8.3 , /*.affinity =*/ 3.9_eV, /*.bandgap =*/ 3.68_eV }; // From: http://www.chalcogen.ro/189_Ramli.pdf
constexpr Material_Constants ZnO{ /*.eps_s =*/ 8.49, /*.affinity =*/ 4.5_eV, /*.bandgap =*/ 3.37_eV }; // From: http://www.chalcogen.ro/189_Ramli.pdf
constexpr Material_Constants Al2O3{ /*.eps_s =*/ 7.6, /*.affinity =*/ 0.0_eV, /*.bandgap =*/ 0.0_eV };

const Metal Aluminum  { "Al", 4.08_eV };
const Metal Chromium  { "Cr", 4.60_eV };
const Metal Gold      { "Au", 5.10_eV };
const Metal Indium    { "In", 4.12_eV };
const Metal Molybdenum{ "Mo", 4.49_eV };
const Metal Nickel    { "Ni", 5.24_eV };
const Metal Platinum  { "Pd", 5.40_eV };
const Metal Titanium  { "Ti", 4.10_eV };
}