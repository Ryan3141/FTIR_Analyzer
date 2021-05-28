#include <complex>

#include "Units.h"
#include "III_V_Materials.h"

struct range
{
	const int start_val;
	const int end_val;
	const int step;
	range( int start, int end, int step = 1 ) :
		start_val( start ), end_val( end ), step( step )
	{}
	range( int end ) :
		start_val( 0 ), end_val( end ), step( 1 )
	{}

	struct iter
	{
		int current;
		iter operator++()
		{
			current++;
			return *this;
		}
		int operator*() const
		{
			return current;
		}
		bool operator!=( const iter rhs ) const
		{
			return this->current != rhs.current;
		}
	};
	iter begin()
	{
		return iter{ start_val };
	}
	iter end()
	{
		return iter{ end_val };
	}
};

namespace III_V_Data
{

struct AlAs
{
	// model parameters
	static const double E0;
	static const double Δ0;
	static const double E1;
	static const double Δ1;
	static const double εinf;
	static const double A;
	static const double Γ0;
	static const double α0;
	static const double B1;
	static const double B1s;
	static const double B1x;
	static const double B2x;
	static const double Γ1;
	static const double α1;
	static const double f0pr;
	static const double Γ0pr;
	static const double E0pr;
	static const double f2x;
	static const double Γ2x;
	static const double E2x;
	static const double f2Σ;
	static const double Γ2Σ;
	static const double E2Σ;
	static const double α2;
};
struct GaAs
{
	// model parameters
	static const double E0;
	static const double Δ0;
	static const double E1;
	static const double Δ1;
	static const double εinf;
	static const double A;
	static const double Γ0;
	static const double α0;
	static const double B1;
	static const double B1s;
	static const double B1x;
	static const double B2x;
	static const double Γ1;
	static const double α1;
	static const double f0pr;
	static const double Γ0pr;
	static const double E0pr;
	static const double f2x;
	static const double Γ2x;
	static const double E2x;
	static const double f2Σ;
	static const double Γ2Σ;
	static const double E2Σ;
	static const double α2;
};

const double AlAs::E0   = 2.993; // eV
const double AlAs::Δ0   = 3.201 - E0; // eV
const double AlAs::E1   = 3.888; // eV
const double AlAs::Δ1   = 4.087 - E1; // eV
const double AlAs::εinf = 0.002;
const double AlAs::A    = 18.0 ; // eV**1.5
const double AlAs::Γ0   = 0.046; // eV
const double AlAs::α0   = 1.11 ;
const double AlAs::B1   = 4.15 ;
const double AlAs::B1s  = 1.54 ;
const double AlAs::B1x  = 1.39 ;
const double AlAs::B2x  = 0.56 ;
const double AlAs::Γ1   = 0.15 ; // eV
const double AlAs::α1   = 0.01 ;
const double AlAs::f0pr = 3.89 ; // eV( not eV**2!!!)
const double AlAs::Γ0pr = 0.55 ; // eV
const double AlAs::E0pr = 4.49 ; // eV
const double AlAs::f2x  = 6.05 ; // eV( not eV**2!!!)
const double AlAs::Γ2x  = 0.68 ; // eV
const double AlAs::E2x  = 4.74 ; // eV
const double AlAs::f2Σ  = 2.62 ; // eV( not eV**2!!!)
const double AlAs::Γ2Σ  = 0.26 ; // eV
const double AlAs::E2Σ  = 4.88 ; // eV
const double AlAs::α2   = 0.060;


const double GaAs::E0   = 1.410; // eV
const double GaAs::Δ0   = 1.746 - E0; // eV
const double GaAs::E1   = 2.926; // eV
const double GaAs::Δ1   = 3.170 - E1; // eV
const double GaAs::εinf = 0.77;
const double GaAs::A    = 3.97; // eV**1.5
const double GaAs::Γ0   = 0.039; // eV
const double GaAs::α0   = 1.65;
const double GaAs::B1   = 4.15;
const double GaAs::B1s  = 1.54;
const double GaAs::B1x  = 1.39;
const double GaAs::B2x  = 0.56;
const double GaAs::Γ1   = 0.15; // eV
const double GaAs::α1   = 0.01;
const double GaAs::f0pr = 3.89; // eV( not eV**2!!!)
const double GaAs::Γ0pr = 0.55; // eV
const double GaAs::E0pr = 4.49; // eV
const double GaAs::f2x  = 6.05; // eV( not eV**2!!!)
const double GaAs::Γ2x  = 0.68; // eV
const double GaAs::E2x  = 4.74; // eV
const double GaAs::f2Σ  = 2.62; // eV( not eV**2!!!)
const double GaAs::Γ2Σ  = 0.26; // eV
const double GaAs::E2Σ  = 4.88; // eV
const double GaAs::α2   = 0.060;


using cx_double = std::complex<double>;
using namespace std::complex_literals;

template<typename Element>
arma::cx_vec Epsilon_I( const arma::vec & ħω )
{
	using E = Element;
	arma::vec Γ = E::Γ0 * arma::exp( -E::α0 * arma::pow( (ħω - E::E0) / E::Γ0, 2 ) );
	arma::cx_vec χ0 = (ħω + 1i * Γ) / E::E0 + 1e-100i; // #1e-100j{ to avoid ambiguity in( 1 - χ )**0.5;
	arma::cx_vec χ0s = (ħω + 1i * Γ) / (E::E0 + E::Δ0) + 1e-100i; // 1e-100j: --"--												   ;
	arma::cx_vec fχ0 = arma::pow(χ0, -2) % (2.0 - arma::sqrt(1.0 + χ0) - arma::sqrt(1.0 - χ0));
	arma::cx_vec fχ0s = arma::pow(χ0s, -2) % (2.0 - arma::sqrt(1.0 + χ0s) - arma::sqrt(1.0 - χ0s));
	return E::A * std::pow( E::E0, -1.5) * (fχ0 + 0.5 * std::pow( E::E0 / (E::E0 + E::Δ0), 1.5) * fχ0s);
}

template<typename Element>
arma::cx_vec Epsilon_II( const arma::vec & ħω )
{
	using E = Element;
	arma::vec Γ = E::Γ1 * arma::exp( -E::α1 * arma::pow((ħω - E::E1) / E::Γ1, 2) );
	arma::cx_vec χ1 = (ħω + 1i * Γ) / E::E1;
	arma::cx_vec χ1s = (ħω + 1i * Γ) / (E::E1 + E::Δ1);
	return -E::B1 * arma::pow(χ1, -2) % arma::log( 1.0 - arma::pow(χ1, 2) ) - E::B1s * arma::pow(χ1s, -2) % arma::log( 1.0 - arma::square(χ1s) );
}

template<typename Element>
arma::cx_vec Epsilon_III( const arma::vec & ħω )
{
	using E = Element;
	arma::vec Γ = E::Γ1 * arma::exp( -E::α1 * arma::pow((ħω - E::E1) / E::Γ1, 2) );
	arma::cx_vec y( arma::size( ħω ), arma::fill::zeros );
	//for( int n : range( 1, 1000 ) )
	for( int n : range( 1, 10 ) )
		y += 1.0 / std::pow(2 * n - 1, 3) * (E::B1x / (E::E1 - ħω - 1i * Γ) + E::B2x / (E::E1 + E::Δ1 - ħω - 1i * Γ));
	return y;
}
template<typename Element>
arma::cx_vec Epsilon_IV( const arma::vec & ħω )
{
	using E = Element;
	arma::vec Γ = E::Γ0pr * arma::exp( -E::α2 * arma::pow((ħω - E::E0pr) / E::Γ0pr, 2) );
	arma::cx_vec ε0pr = std::pow( E::f0pr, 2) / (std::pow( E::E0pr, 2) - arma::pow(ħω, 2) - 1i * ħω % Γ);
	Γ = E::Γ2x * arma::exp( -E::α2 * arma::square((ħω - E::E2x) / E::Γ2x) );
	arma::cx_vec ε2x = std::pow( E::f2x, 2) / (std::pow( E::E2x, 2) - arma::square(ħω) - 1i * ħω % Γ);
	Γ = E::Γ2Σ * arma::exp( -E::α2 * arma::square( (ħω - E::E2Σ) / E::Γ2Σ ) );
	arma::cx_vec ε2Σ = std::pow( E::f2Σ, 2) / (std::pow( E::E2Σ, 2) - arma::square(ħω) - 1i * ħω % Γ);
	return E::εinf + ε0pr + ε2x + ε2Σ;
}

template< typename Element >
arma::cx_vec Get_Refraction_Index( const arma::vec & wavelengths, double temperature_k )
{
	arma::vec ħω = 1.23984198406E-6 / wavelengths;
	arma::cx_vec εA = Epsilon_I  <Element>( ħω );
	arma::cx_vec εB = Epsilon_II <Element>( ħω );
	arma::cx_vec εC = Epsilon_III<Element>( ħω );
	arma::cx_vec εD = Epsilon_IV <Element>( ħω );
	arma::cx_vec ε = εA + εB + εC + εD + Element::εinf;
	arma::cx_vec sqrt_ε = arma::sqrt( ε );
	return sqrt_ε; // = {n, k}
	//arma::cx_vec n = sqrt_ε.real;
	//arma::cx_vec k = sqrt_ε.imag;
	//arma::cx_vec α = 4 * π*k / μm * 1e4 #1 / cm;
}


arma::cx_vec Get_AlAs_Refraction_Index( const arma::vec & wavelengths, double temperature_k )
{
	return Get_Refraction_Index<GaAs>( wavelengths, temperature_k );
}

arma::cx_vec Get_GaAs_Refraction_Index( const arma::vec & wavelengths, double temperature_k )
{
	return Get_Refraction_Index<AlAs>( wavelengths, temperature_k );
}


namespace InAs
{
const double π = arma::datum::pi;

// # model parameters
const double E0 = 0.36     ; // #eV
const double Δ0 = 0.76 - E0; // #eV
const double E1 = 2.50     ; // #eV
const double Δ1 = 2.78 - E1; // #eV
const double E2 = 4.45     ; // #eV
const double Eg = 1.07     ; // #eV
const double A = 0.61      ; // #eV^1.5
const double B1 = 6.59     ; // 
const double B11 = 13.76   ; // #eV^-0.5
const double Γ = 0.21      ; // #eV
const double C = 1.78	   ;
const double γ = 0.108	   ;
const double D = 20.8	   ;
const double εinf = 2.8	   ;

arma::vec H( const arma::vec & x ) // #Heviside function
{
	arma::vec h( arma::size( x ), arma::fill::zeros );// , { 1, 0 }; // Air
	h.elem( arma::find( x >= 0 ) ).ones();
	return h;
}
arma::cx_vec Epsilon_A( const arma::vec & ħω ) // #E0
{
	auto χ0 = ħω / E0;
	auto χso = ħω / (E0 + Δ0);
	arma::vec H0 = H( 1 - χ0 );
	arma::vec Hso = H( 1 - χso );
	arma::vec fχ0 = arma::pow( χ0, -2 ) % (2 - arma::sqrt( 1 + χ0 ) - arma::sqrt( (1 - χ0) % H0 ));
	arma::vec fχso = arma::pow( χso, -2 ) % (2 - arma::sqrt( 1 + χso ) - arma::sqrt( (1 - χso) % Hso ));
	H0 = H( χ0 - 1 );
	Hso = H( χso - 1 );
	auto ε2 = A / arma::square( ħω ) % ( arma::sqrt( (ħω - E0) % H0 ) + 0.5*arma::sqrt( (ħω - E0 - Δ0) % Hso ) );
	auto ε1 = A * std::pow( E0, -1.5 ) * (fχ0 + 0.5 * std::pow( E0 / (E0 + Δ0), 1.5 )*fχso);
	return ε1 + 1i * ε2;
}
arma::cx_vec Epsilon_B( const arma::vec & ħω ) // #E1
{
	// # ignoring E1 + Δ1 contribution - no data on B2 & B21 in the paper
	// # result seems to reproduce graphical data from the paper
	auto χ1 = ħω / E1;
	auto H1 = H( 1 - χ1 );
	arma::vec ε2 = π * arma::pow(χ1,-2) % (B1 - B11 * arma::sqrt((E1 - ħω) % H1));
	ε2 %= H( ε2 ); // #undocumented trick : ignore negative ε2;
	auto χ1_again = (ħω + 1i * Γ) / E1;
	auto ε1 = -B1 * arma::pow( χ1_again, -2 ) % arma::log( 1 - arma::square( χ1_again ) );
	return arma::real( ε1 ) + 1i * arma::real( ε2 );
}
arma::cx_vec Epsilon_C( const arma::vec & ħω ) // #E2
{
	auto χ2 = ħω / E2;
	auto ε2 = C * χ2*γ / ( arma::square( (1 - arma::square( χ2 )) ) + arma::square(χ2*γ));
	auto ε1 = C * arma::square( 1 - χ2 ) / (arma::square( 1 - arma::square( χ2 ) ) + arma::square( χ2*γ ));
	return ε1 + 1i * ε2;
}
arma::cx_vec Epsilon_D( const arma::vec & ħω ) // #Eg
{
	// # ignoring ħωq - no data in the paper
	// # result seems to reproduce graphical data from the paper
	auto Ech = E1;
	auto χg = Eg / ħω;
	auto χch = ħω / Ech;
	arma::vec Hg = H( 1 - χg );
	arma::vec Hch = H( 1 - χch );
	auto ε2 = D / arma::square( ħω ) % arma::square( (ħω - Eg) ) % Hg % Hch;
	return 1i * ε2;
}

} // End namespace InAs

arma::cx_vec Get_InAs_Refraction_Index( const arma::vec & wavelengths, double temperature_k )
{
	arma::vec ħω = 1.23984198406E-6 / wavelengths;
	arma::cx_vec εA = InAs::Epsilon_A( ħω );
	arma::cx_vec εB = InAs::Epsilon_B( ħω );
	arma::cx_vec εC = InAs::Epsilon_C( ħω );
	arma::cx_vec εD = InAs::Epsilon_D( ħω );
	arma::cx_vec ε = εA + εB + εC + εD + InAs::εinf;
	arma::cx_vec sqrt_ε = arma::sqrt( ε );
	return sqrt_ε; // = {n, k}
	//arma::cx_vec n = sqrt_ε.real;
	//arma::cx_vec k = sqrt_ε.imag;
	//arma::cx_vec α = 4 * π*k / μm * 1e4 #1 / cm;
}

}