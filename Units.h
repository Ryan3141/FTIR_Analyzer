#pragma once

#include <vector>
#include <armadillo>

//enum class IV_Y_Units
//{
//	CURRENT_A = 0,
//	CURRENT_MA = 1,
//	CURRENT_UA = 2,
//	CURRENT_NA = 3
//};

namespace CV
{
enum class X_Units
{
	VOLTAGE_V = 0
};

enum class Y_Units
{
	CAPACITANCE_F = 0,
};
}

#include <ratio>

namespace Units
{
//const double epsilon_0 = arma::datum::eps_0; // 8.8541878128E-12; // F / m = C / (m * V)
////double k_B = 1.380649E-23; // J / K
//const double ee = arma::datum::ec; // 1.602176634E-19; // Coulombs
//const double k_B = arma::datum::k / ee; // 8.617333262145E-5; // eV / K
//const double pi = arma::datum::pi;
//const double hbar2 = (arma::datum::h_bar / ee) * (arma::datum::h_bar / ee); // Planck's constant squared in (eV / s)^2
//const double m_e = arma::datum::m_e;// *arma::datum::c_0 * arma::datum::c_0;

#if 0

template <std::intmax_t num, std::intmax_t den = 1>
using Rational = std::ratio<num, den>;

using Zero = Rational<0, 1>;
using One = Rational<1, 1>;
using Two = Rational<2, 1>;
using Three = Rational<3, 1>;


//template< typename Rhs_Rational >
//constexpr auto operator*( int lhs, const Rhs_Rational & rhs )
//{
//	return Rational< Rhs_Rational::num{ lhs * rhs.v };
//}

//template<typename m_p, typename m_e, typename N_A, typename k_B, typename a_0, typename mu_B, typename Z_0, typename G_0, typename eps_0, typename ee, typename F, typename alpha, typename alpha_inv, typename K_J, typename mu_0, typename phi_0, typename R, typename G, typename h, typename h_bar, typename m_p, typename R_inf, typename c_0, typename sigma, typename R_k, typename b>

template<typename T_PI, typename T_EE, typename T_K_B, typename T_EPS_0, typename T_H, typename T_M_E>
struct Physics_Constants
{
	template<typename Rhs_PI, typename Rhs_EE, typename Rhs_K_B, typename Rhs_EPS_0, typename Rhs_H, typename Rhs_M_E>
	constexpr auto operator*( const Physics_Constants< Rhs_PI, Rhs_EE, Rhs_K_B, Rhs_EPS_0, Rhs_H, Rhs_M_E > & rhs )
	{
		return Physics_Constants<
			std::ratio_add< T_PI, Rhs_PI    >,
			std::ratio_add< T_EE, Rhs_EE    >,
			std::ratio_add< T_K_B, Rhs_K_B   >,
			std::ratio_add< T_EPS_0, Rhs_EPS_0 >,
			std::ratio_add< T_H, Rhs_H     >,
			std::ratio_add< T_M_E, Rhs_M_E   > >{};
	}
	template<typename Rhs_PI, typename Rhs_EE, typename Rhs_K_B, typename Rhs_EPS_0, typename Rhs_H, typename Rhs_M_E>
	constexpr auto operator/( const Physics_Constants< Rhs_PI, Rhs_EE, Rhs_K_B, Rhs_EPS_0, Rhs_H, Rhs_M_E > & rhs )
	{
		return Physics_Constants<
			std::ratio_subtract< T_PI, Rhs_PI    >,
			std::ratio_subtract< T_EE, Rhs_EE    >,
			std::ratio_subtract< T_K_B, Rhs_K_B   >,
			std::ratio_subtract< T_EPS_0, Rhs_EPS_0 >,
			std::ratio_subtract< T_H, Rhs_H     >,
			std::ratio_subtract< T_M_E, Rhs_M_E   > >{};
	}
	template<typename Rational_Power>
	static constexpr auto Power()
	{
		return Physics_Constants<
			std::ratio_multiply< T_PI, Rational_Power >,
			std::ratio_multiply< T_EE, Rational_Power >,
			std::ratio_multiply< T_K_B, Rational_Power >,
			std::ratio_multiply< T_EPS_0, Rational_Power >,
			std::ratio_multiply< T_H, Rational_Power >,
			std::ratio_multiply< T_M_E, Rational_Power > >{};
	}

	template<typename FloatType>
	static constexpr FloatType To_Value()
	{
		const FloatType pi = std::pow( FloatType( 3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679 ), FloatType( T_PI::num ) / T_PI::den );
		const FloatType ee = std::pow( FloatType( 1.6021766208e-19 ), FloatType( T_EE::num ) / T_EE::den );
		const FloatType k_B = std::pow( FloatType( 1.38064852e-23 ), FloatType( T_K_B::num ) / T_K_B::den );
		const FloatType eps_0 = std::pow( FloatType( 8.85418781762039e-12 ), FloatType( T_EPS_0::num ) / T_EPS_0::den );
		const FloatType h = std::pow( FloatType( 6.626070040e-34 ), FloatType( T_H::num ) / T_H::den );
		const FloatType m_e = std::pow( FloatType( 9.10938356e-31 ), FloatType( T_M_E::num ) / T_M_E::den );

		return pi * ee * k_B * eps_0 * h * m_e;
	}
};

using electron_charge_t = Physics_Constants<Zero, One, Zero, Zero, Zero, Zero>;
using No_Constants = Physics_Constants<Zero, Zero, Zero, Zero, Zero, Zero>;

template< typename Length_Exponent, typename Time_Exponent, typename Energy_Exponent, typename Charge_Exponent, typename Temperature_Exponent >
struct Unit_Type
{
	//static constexpr auto Length_Exponent      = length_exponent     ();
	//static constexpr auto Time_Exponent        = time_exponent       ();
	//static constexpr auto Energy_Exponent      = energy_exponent     ();
	//static constexpr auto Charge_Exponent      = charge_exponent     ();
	//static constexpr auto Temperature_Exponent = temperature_exponent();

	template< typename Rhs_Length_Exponent, typename Rhs_Time_Exponent, typename Rhs_Energy_Exponent, typename Rhs_Charge_Exponent, typename Rhs_Temperature_Exponent >
	constexpr auto operator+( const Unit_Type< Rhs_Length_Exponent, Rhs_Time_Exponent, Rhs_Energy_Exponent, Rhs_Charge_Exponent, Rhs_Temperature_Exponent > & rhs )
	{
		static_assert( std::is_same< Length_Exponent, Rhs_Length_Exponent      >::value, "The units are not compatible in the length component" );
		static_assert( std::is_same< Time_Exponent, Rhs_Time_Exponent        >::value, "The units are not compatible in the time component" );
		static_assert( std::is_same< Energy_Exponent, Rhs_Energy_Exponent      >::value, "The units are not compatible in the energy component" );
		static_assert( std::is_same< Charge_Exponent, Rhs_Charge_Exponent      >::value, "The units are not compatible in the charge component" );
		static_assert( std::is_same< Temperature_Exponent, Rhs_Temperature_Exponent >::value, "The units are not compatible in the temperature component" );
		return Unit_Type<Length_Exponent, Time_Exponent, Energy_Exponent, Charge_Exponent, Temperature_Exponent>{};
	}
	template< typename Rhs_Length_Exponent, typename Rhs_Time_Exponent, typename Rhs_Energy_Exponent, typename Rhs_Charge_Exponent, typename Rhs_Temperature_Exponent >
	constexpr auto operator-( const Unit_Type< Rhs_Length_Exponent, Rhs_Time_Exponent, Rhs_Energy_Exponent, Rhs_Charge_Exponent, Rhs_Temperature_Exponent > & rhs )
	{
		static_assert( std::is_same< Length_Exponent, Rhs_Length_Exponent      >::value, "The units are not compatible in the length component" );
		static_assert( std::is_same< Time_Exponent, Rhs_Time_Exponent        >::value, "The units are not compatible in the time component" );
		static_assert( std::is_same< Energy_Exponent, Rhs_Energy_Exponent      >::value, "The units are not compatible in the energy component" );
		static_assert( std::is_same< Charge_Exponent, Rhs_Charge_Exponent      >::value, "The units are not compatible in the charge component" );
		static_assert( std::is_same< Temperature_Exponent, Rhs_Temperature_Exponent >::value, "The units are not compatible in the temperature component" );
		return Unit_Type<Length_Exponent, Time_Exponent, Energy_Exponent, Charge_Exponent, Temperature_Exponent>{};
	}
	template< typename Rhs_Length_Exponent, typename Rhs_Time_Exponent, typename Rhs_Energy_Exponent, typename Rhs_Charge_Exponent, typename Rhs_Temperature_Exponent >
	constexpr auto operator*( const Unit_Type< Rhs_Length_Exponent, Rhs_Time_Exponent, Rhs_Energy_Exponent, Rhs_Charge_Exponent, Rhs_Temperature_Exponent > & rhs )
	{
		return Unit_Type<
			std::ratio_add< Length_Exponent, Rhs_Length_Exponent      >,
			std::ratio_add< Time_Exponent, Rhs_Time_Exponent        >,
			std::ratio_add< Energy_Exponent, Rhs_Energy_Exponent      >,
			std::ratio_add< Charge_Exponent, Rhs_Charge_Exponent      >,
			std::ratio_add< Temperature_Exponent, Rhs_Temperature_Exponent > >{};
	}
	template< typename Rhs_Length_Exponent, typename Rhs_Time_Exponent, typename Rhs_Energy_Exponent, typename Rhs_Charge_Exponent, typename Rhs_Temperature_Exponent >
	constexpr auto operator/( const Unit_Type< Rhs_Length_Exponent, Rhs_Time_Exponent, Rhs_Energy_Exponent, Rhs_Charge_Exponent, Rhs_Temperature_Exponent > & rhs )
	{
		return Unit_Type<
			std::ratio_subtract< Length_Exponent, Rhs_Length_Exponent      >,
			std::ratio_subtract< Time_Exponent, Rhs_Time_Exponent        >,
			std::ratio_subtract< Energy_Exponent, Rhs_Energy_Exponent      >,
			std::ratio_subtract< Charge_Exponent, Rhs_Charge_Exponent      >,
			std::ratio_subtract< Temperature_Exponent, Rhs_Temperature_Exponent > >{};
	}
	template<typename Rational_Power>
	static constexpr auto Power()
	{
		return Unit_Type<
			std::ratio_multiply< Length_Exponent, Rational_Power >,
			std::ratio_multiply< Time_Exponent, Rational_Power >,
			std::ratio_multiply< Energy_Exponent, Rational_Power >,
			std::ratio_multiply< Charge_Exponent, Rational_Power >,
			std::ratio_multiply< Temperature_Exponent, Rational_Power > >{};
	}
};

using Empty_Unit_Type = Unit_Type<Zero, Zero, Zero, Zero, Zero>;
using Length_t = Unit_Type< One, Zero, Zero, Zero, Zero>;
using Time_t = Unit_Type<Zero, One, Zero, Zero, Zero>;
using Energy_t = Unit_Type<Zero, Zero, One, Zero, Zero>;
using Charge_t = Unit_Type<Zero, Zero, Zero, One, Zero>;
using Temperature_t = Unit_Type<Zero, Zero, Zero, Zero, One>;

//constexpr int64_t Power( int64_t base, int exp, int64_t result = 1 )
//{
//	return exp < 1 ? result : Power( base*base, exp / 2, (exp % 2) ? result * base : result );
//}

template< typename Lhs_Rational, typename Result_t >
constexpr auto Power( Lhs_Rational base, int exp, Result_t result )
{
	return exp < 1 ? result : Power( base*base, exp / 2, ( exp % 2 ) ? result * base : result );
}

template< typename FloatType, typename Physics_Constants, typename Scale_From_SI, typename Unit_Type > // typename Length_Exponent, typename Time_Exponent, typename Energy_Exponent, typename Charge_Exponent, typename Temperature_Exponent >
struct Combined_Units
{
	FloatType v;

	template< typename Rhs_Physics_Constants, typename Rhs_Scale_From_SI, typename Rhs_Unit_Type > // typename Rhs_Length_Exponent, typename Rhs_Time_Exponent, typename Rhs_Energy_Exponent, typename Rhs_Charge_Exponent, typename Rhs_Temperature_Exponent >
	constexpr auto operator*( const Combined_Units< FloatType, Rhs_Physics_Constants, Rhs_Scale_From_SI, Rhs_Unit_Type > & rhs ) const
	{
		return Combined_Units< FloatType,
			decltype( Physics_Constants() * Rhs_Physics_Constants() ),
			decltype( Scale_From_SI() * Rhs_Scale_From_SI() ),
			decltype( Unit_Type() * Rhs_Unit_Type() ) >{ this->v * rhs.v };
	}

	template< typename Rhs_Physics_Constants, typename Rhs_Scale_From_SI, typename Rhs_Unit_Type > // typename Rhs_Length_Exponent, typename Rhs_Time_Exponent, typename Rhs_Energy_Exponent, typename Rhs_Charge_Exponent, typename Rhs_Temperature_Exponent >
	constexpr auto operator/( const Combined_Units< FloatType, Rhs_Physics_Constants, Rhs_Scale_From_SI, Rhs_Unit_Type > & rhs ) const
	{
		return Combined_Units< FloatType,
			decltype( Physics_Constants() / Rhs_Physics_Constants() ),
			decltype( Scale_From_SI() / Rhs_Scale_From_SI() ),
			decltype( Unit_Type() / Rhs_Unit_Type() ) >{ this->v / rhs.v };
	}

	template< typename Rhs_Physics_Constants, typename Rhs_Scale_From_SI, typename Rhs_Unit_Type >
	constexpr auto operator+( const Combined_Units< FloatType, Rhs_Physics_Constants, Rhs_Scale_From_SI, Rhs_Unit_Type > & rhs ) const
	{
		static_assert( std::is_same< Physics_Constants, Rhs_Physics_Constants >::value, "The physics constants are not compatible" );
		static_assert( std::is_same< Scale_From_SI, Rhs_Scale_From_SI     >::value, "The units are not compatible in the non SI unit exponent component" );
		return Combined_Units< FloatType, Physics_Constants, Scale_From_SI, decltype( Unit_Type() + Rhs_Unit_Type() ) >( this->v + rhs.v );
	}

	template< typename Rhs_Physics_Constants, typename Rhs_Scale_From_SI, typename Rhs_Unit_Type >
	constexpr auto operator-( const Combined_Units< FloatType, Rhs_Physics_Constants, Rhs_Scale_From_SI, Rhs_Unit_Type > & rhs ) const
	{
		static_assert( std::is_same< Physics_Constants, Rhs_Physics_Constants >::value, "The physics constants are not compatible" );
		static_assert( std::is_same< Scale_From_SI, Rhs_Scale_From_SI     >::value, "The units are not compatible in the non SI unit exponent component" );
		return Combined_Units< FloatType, Physics_Constants, Scale_From_SI, decltype( Unit_Type() - Rhs_Unit_Type() ) >( this->v - rhs.v );
	}

	template <typename Rhs_Rational>
	constexpr auto Power( const Rhs_Rational & rhs ) const
	{
		return Combined_Units< FloatType,
			decltype( Physics_Constants::Power<Rhs_Rational>() ),
			decltype( Scale_From_SI::Power<Rhs_Rational>() ),
			decltype( Unit_Type::Power<Rhs_Rational>() ) >{ std::pow( this->v, double( Rhs_Rational::num ) / Rhs_Rational::den ) };
	}

	constexpr operator FloatType() const
	{
		if constexpr(
			std::is_same<Unit_Type, Empty_Unit_Type>::value &&
			std::is_same<Scale_From_SI, Zero>::value &&
			std::is_same<Physics_Constants, No_Constants>::value )
			return this->v;
		else if constexpr(
			std::is_same<Unit_Type, Empty_Unit_Type>::value )
			return this->v / ( Physics_Constants::Value() * Scale_From_SI::Value() * FloatType( Scale_From_SI::num ) / Scale_From_SI::den );
		else
			static_assert( false, "To get the value as a double, use the .v component" );
	}

	constexpr explicit Combined_Units() : v( 1.0 )
	{
	}
	constexpr explicit Combined_Units( int input ) : v( input )
	{
	}
	constexpr explicit Combined_Units( float input ) : v( input )
	{
	}
	constexpr explicit Combined_Units( double input ) : v( input )
	{
	}
	constexpr explicit Combined_Units( long double input ) : v( input )
	{
	}
	//template <class T>
	//constexpr Combined_Units( T input ) { static_assert(false, "Cannot convert to unit requested: " __FUNCTION__); }; // Avoid implicit conversions from anything but floating point types

	Combined_Units( const Combined_Units & ) = default;
	Combined_Units( Combined_Units && ) = default;
	Combined_Units& operator=( const Combined_Units & ) & = default;
	Combined_Units& operator=( Combined_Units && ) & = default;
	~Combined_Units() = default;
};


template< typename FloatType, typename Physics_Constants, typename Scale_From_SI, typename Unit_Type >
constexpr auto Make_Units( double v )
{
	return Combined_Units< FloatType, Physics_Constants, Scale_From_SI, Unit_Type >{ v };
}

template< typename FloatType, typename Physics_Constants, typename Scale_From_SI, typename Unit_Type >
constexpr auto operator*( FloatType lhs, const Combined_Units< FloatType, Physics_Constants, Scale_From_SI, Unit_Type > & rhs )
{
	return decltype( rhs ){ lhs * rhs.v };
}

template< typename Lhs_Ratio, typename FloatType, typename Physics_Constants, typename Scale_From_SI, typename Unit_Type >
constexpr auto operator*( Lhs_Ratio lhs, const Combined_Units< FloatType, Physics_Constants, Scale_From_SI, Unit_Type > & rhs )
{
	return Combined_Units< FloatType, Physics_Constants, std::ratio_multiply< Lhs_Ratio, Scale_From_SI >, Unit_Type >{ rhs.v };
}


//using h_bar = Combined_Units< double, Physics_Constants<Rational<-1, 1>, Zero, Zero, Zero, One, Zero>,
//	Zero, Zero, Rational<-1, 1>, One, Zero, Zero >;

// Build all of the nicely named predefined types like Meters, Seconds, etc and their literals so you can write 1_s for one second
#define BUILD_NAME( a, b, c, d ) a##b##c##d
#define CREATE_UNIT_TYPES( prefix, suffix, power, unit_scale, rep ) \
using prefix ## Meters   ## suffix = Combined_Units< double, No_Constants, Rational<(unit_scale), 1>, Unit_Type< Rational<(power), 1>, Zero, Zero, Zero, Zero > >; \
using prefix ## Seconds  ## suffix = Combined_Units< double, No_Constants, Rational<(unit_scale), 1>, Unit_Type< Zero, Rational<(power), 1>, Zero, Zero, Zero > >; \
using prefix ## Joules   ## suffix = Combined_Units< double, No_Constants, Rational<(unit_scale), 1>, Unit_Type< Zero, Zero, Rational<(power), 1>, Zero, Zero > >; \
using prefix ## Coulombs ## suffix = Combined_Units< double, No_Constants, Rational<(unit_scale), 1>, Unit_Type< Zero, Zero, Zero, Rational<(power), 1>, Zero > >; \
using prefix ## Kelvin   ## suffix = Combined_Units< double, No_Constants, Rational<(unit_scale), 1>, Unit_Type< Zero, Zero, Zero, Zero, Rational<(power), 1> > >; \
constexpr auto operator"" BUILD_NAME(_, rep, m, suffix)( long double input ) { return BUILD_NAME(,prefix, Meters  , suffix){ input }; } \
constexpr auto operator"" BUILD_NAME(_, rep, s, suffix)( long double input ) { return BUILD_NAME(,prefix, Seconds , suffix){ input }; } \
constexpr auto operator"" BUILD_NAME(_, rep, J, suffix)( long double input ) { return BUILD_NAME(,prefix, Joules  , suffix){ input }; } \
constexpr auto operator"" BUILD_NAME(_, rep, Coul, suffix)( long double input ) { return BUILD_NAME(,prefix, Coulombs, suffix){ input }; } /*_C2 is a reserved keyword*/ \
constexpr auto operator"" BUILD_NAME(_, rep, K, suffix)( long double input ) { return BUILD_NAME(,prefix, Kelvin  , suffix){ input }; }


#define ALL_UNIT_POWERS( prefix, representation, si_scale ) \
CREATE_UNIT_TYPES( prefix,  , 1,  (si_scale)     , representation ) \
CREATE_UNIT_TYPES( prefix, 2, 2, ((si_scale) * 2), representation ) \
CREATE_UNIT_TYPES( prefix, 3, 3, ((si_scale) * 3), representation )

ALL_UNIT_POWERS( , , 0 )
ALL_UNIT_POWERS( Milli, m, -3 )
ALL_UNIT_POWERS( Micro, u, -6 )
ALL_UNIT_POWERS( Nano, n, -9 )
ALL_UNIT_POWERS( Pico, p, -12 )
ALL_UNIT_POWERS( Femto, f, -15 )
ALL_UNIT_POWERS( Atto, a, -18 )
ALL_UNIT_POWERS( Zepto, z, -21 )
ALL_UNIT_POWERS( Yocto, y, -24 )
ALL_UNIT_POWERS( Kilo, k, +3 )
ALL_UNIT_POWERS( Mega, M, +6 )
ALL_UNIT_POWERS( Giga, G, +9 )
ALL_UNIT_POWERS( Tera, T, +12 )
ALL_UNIT_POWERS( Peta, P, +15 )
ALL_UNIT_POWERS( Exa, E, +18 )
ALL_UNIT_POWERS( Zetta, Z, +21 )
ALL_UNIT_POWERS( Yotta, Y, +24 )



using Electron_Volts = Combined_Units< double, electron_charge_t, One, Energy_t >;
constexpr Electron_Volts operator"" _eV( long double input )
{
	return Electron_Volts{ input };
	//return double( input );
}

//template<typename T_PI, typename T_EE, typename T_K_B, typename T_EPS_0, typename T_H, typename T_M_E>
constexpr auto epsilon_0 = Combined_Units<double, Physics_Constants<Zero, Zero, Zero, One, Zero, Zero>, One, Energy_t>();
constexpr auto k_B = Combined_Units<double, Physics_Constants<Zero, Zero, One, Zero, Zero, Zero>, One, decltype( Energy_t() / Temperature_t() )>(); //1.380649E-23; // J / K
constexpr auto ee = Combined_Units<double, Physics_Constants<Zero, Zero, Zero, One, Zero, Zero>, One, Charge_t>();
constexpr auto pi = Combined_Units<double, Physics_Constants< One, Zero, Zero, Zero, Zero, Zero>, One, Empty_Unit_Type>();
constexpr auto hbar = Combined_Units<double, Physics_Constants<Rational<-1>, Zero, Zero, Zero, One, Zero>, Rational<1, 2>, decltype( Energy_t() * Time_t() )>();
constexpr auto h = Combined_Units<double, Physics_Constants<Zero, Zero, Zero, Zero, One, Zero>, One, decltype( Energy_t() * Time_t() )>();
constexpr auto m_e = Combined_Units<double, Physics_Constants<Zero, Zero, Zero, One, Zero, Zero>, One, Energy_t>();
#else

using Electron_Volts = double;
constexpr Electron_Volts operator"" _eV( long double input )
{
	//return Electron_Volts{ input };
	return double( input );
}

const double ee = arma::datum::ec; // 1.602176634E-19; // Coulombs
const double k_B = arma::datum::k / ee; // 8.617333262145E-5; // eV / K
const double pi = arma::datum::pi;
const double hbar2 = ( arma::datum::h_bar / ee ) * ( arma::datum::h_bar / ee ); // Planck's constant squared in (eV / s)^2
const double m_e = arma::datum::m_e;// *arma::datum::c_0 * arma::datum::c_0;
const double epsilon_0 = arma::datum::eps_0; // 8.8541878128E-12; // F / m = C / (m * V)

#endif
// Allow for Rational to be constructed as a compile time literal
template<class none = void>
constexpr int Recursive_Parse_Numbers()
{
	return 0;
}

template<char right_number, char ...Rest>
constexpr int Recursive_Parse_Numbers()
{
	int r1 = right_number - '0';
	constexpr int r2 = Recursive_Parse_Numbers<Rest...>();
	for( int i = 0; i < sizeof...( Rest ); i++ )
		r1 *= 10;
	return r1 + r2;
}

template <std::intmax_t num, std::intmax_t den = 1>
using Rational = std::ratio<num, den>;

template<char ...Numbers>
auto operator "" _R()
{
	constexpr int r = Recursive_Parse_Numbers<Numbers...>();
	return Rational< r, 1 >();
}

}