#include "CV_Theoretical.h"
#include "Optimize.h"

double sign( double number )
{
	return (std::signbit( number ) ? -1 : +1);
}
using namespace Units;
//Meters2 testing123;
//void test2()
//{
//	auto z = Seconds( 1.0 );
//	Seconds( 5.0 ) * Meters( 1.0 );
//	auto a = Seconds( 1.0 ) * Meters( 15.0 );
//	auto d = 1234_R;
//	MilliMeters2 b1 = 1.0_um * 1.0_m;// Rational<2, 1>();
//	MilliMeters2 b2 = (10.0_mm)::Power( 2_R );// Rational<2, 1>();
//	//auto c = 15.0 * Meters( 6.0 ) * Meters( 15.0 ) + (10.0_m)^Rational<2,1>();
//	return;
//}

namespace Rosetta // Code from: https://rosettacode.org/wiki/Numerical_integration/Gauss-Legendre_Quadrature
{
double M_PI = arma::datum::pi;
/*! Implementation of Gauss-Legendre quadrature
*  http://en.wikipedia.org/wiki/Gaussian_quadrature
*  http://rosettacode.org/wiki/Numerical_integration/Gauss-Legendre_Quadrature
*
*/
template <int N>
class GaussLegendreQuadrature
{
public:
	enum { eDEGREE = N };

	/*! Compute the integral of a functor
	*
	*   @param a    lower limit of integration
	*   @param b    upper limit of integration
	*   @param f    the function to integrate
	*   @param err  callback in case of problems
	*/
	template <typename Function>
	double integrate( double a, double b, Function f )
	{
		double p = (b - a) / 2;
		double q = (b + a) / 2;
		const LegendrePolynomial& legpoly = s_LegendrePolynomial;

		double sum = 0;
		for( int i = 1; i <= eDEGREE; ++i )
		{
			sum += legpoly.weight( i ) * f( p * legpoly.root( i ) + q );
		}

		return p * sum;
	}

	/*! Print out roots and weights for information
	*/
	void print_roots_and_weights( std::ostream& out ) const
	{
		const LegendrePolynomial& legpoly = s_LegendrePolynomial;
		out << "Roots:  ";
		for( int i = 0; i <= eDEGREE; ++i )
		{
			out << ' ' << legpoly.root( i );
		}
		out << '\n';
		out << "Weights:";
		for( int i = 0; i <= eDEGREE; ++i )
		{
			out << ' ' << legpoly.weight( i );
		}
		out << '\n';
	}
private:
	/*! Implementation of the Legendre polynomials that form
	*   the basis of this quadrature
	*/
	class LegendrePolynomial
	{
	public:
		LegendrePolynomial()
		{
			// Solve roots and weights
			for( int i = 0; i <= eDEGREE; ++i )
			{
				double dr = 1;

				// Find zero
				Evaluation eval( cos( M_PI * (i - 0.25) / (eDEGREE + 0.5) ) );
				do
				{
					dr = eval.v() / eval.d();
					eval.evaluate( eval.x() - dr );
				} while( fabs( dr ) > 2e-16 );

				this->_r[ i ] = eval.x();
				this->_w[ i ] = 2 / ((1 - eval.x() * eval.x()) * eval.d() * eval.d());
			}
		}

		double root( int i ) const { return this->_r[ i ]; }
		double weight( int i ) const { return this->_w[ i ]; }
	private:
		double _r[ eDEGREE + 1 ];
		double _w[ eDEGREE + 1 ];

		/*! Evaluate the value *and* derivative of the
		*   Legendre polynomial
		*/
		class Evaluation
		{
		public:
			explicit Evaluation( double x ) : _x( x ), _v( 1 ), _d( 0 )
			{
				this->evaluate( x );
			}

			void evaluate( double x )
			{
				this->_x = x;

				double vsub1 = x;
				double vsub2 = 1;
				double f = 1 / (x * x - 1);

				for( int i = 2; i <= eDEGREE; ++i )
				{
					this->_v = ((2 * i - 1) * x * vsub1 - (i - 1) * vsub2) / i;
					this->_d = i * f * (x * this->_v - vsub1);

					vsub2 = vsub1;
					vsub1 = this->_v;
				}
			}

			double v() const { return this->_v; }
			double d() const { return this->_d; }
			double x() const { return this->_x; }

		private:
			double _x;
			double _v;
			double _d;
		};
	};

	/*! Pre-compute the weights and abscissae of the Legendre polynomials
	*/
	static LegendrePolynomial s_LegendrePolynomial;
};

template <int N>
typename GaussLegendreQuadrature<N>::LegendrePolynomial GaussLegendreQuadrature<N>::s_LegendrePolynomial;
}

// This to avoid issues with exp being a templated function
double RosettaExp( double x )
{
	return exp( x );
}

#include <iomanip>
void test()
{
	Rosetta::GaussLegendreQuadrature<5> gl5;

	std::cout << std::setprecision( 10 );

	gl5.print_roots_and_weights( std::cout );
	std::cerr << "Integrating Exp(X) over [-3, 3]: " << gl5.integrate( -3., 3., RosettaExp ) << '\n';
	std::cerr << "Actual value:                    " << RosettaExp( 3 ) - RosettaExp( -3 ) << '\n';

	auto f = []( double u )
	{
		double F_U = std::sqrt( (std::exp( -u ) + u - 1) + (std::exp( u ) - u - 1) );
		if( F_U == 0 )
			return 0.5;
		else
			return (1 - std::exp( -u )) * (std::exp( u ) - u - 1) / (F_U * F_U * F_U);
	};
	double integral2 = Rosetta::GaussLegendreQuadrature<2>().integrate( 0, -1.0, f );
	double integral3 = Rosetta::GaussLegendreQuadrature<3>().integrate( 0, -1.0, f );
	double integral4 = Rosetta::GaussLegendreQuadrature<4>().integrate( 0, -1.0, f );
	double integral5 = Rosetta::GaussLegendreQuadrature<5>().integrate( 0, -1.0, f );
	double integral7 = Rosetta::GaussLegendreQuadrature<7>().integrate( 0, -1.0, f );
	double integral8 = Rosetta::GaussLegendreQuadrature<8>().integrate( 0, -1.0, f );
	double integral9 = Rosetta::GaussLegendreQuadrature<9>().integrate( 0, -1.0, f );
	std::cerr << std::setprecision( 25 );
	std::cerr << 2 << ": " << integral2 << '\n';
	std::cerr << 3 << ": " << integral3 << '\n';
	std::cerr << 4 << ": " << integral4 << '\n';
	std::cerr << 5 << ": " << integral5 << '\n';
	std::cerr << 7 << ": " << integral7 << '\n';
	std::cerr << 8 << ": " << integral8 << '\n';
	std::cerr << 9 << ": " << integral9 << '\n';
	integral2 += 1;

}


namespace CV_Measurements
{

inline arma::vec Frequency_Semiconductor_Capacitance( const arma::vec & bias_voltages, const Semiconductor & semiconductor, const Insulator & insulator, const Metal & contact, double temperature_in_k, double frequency_in_hz )
{
	double eps = semiconductor.eps_s * epsilon_0;
	double kT_over_e = k_B * temperature_in_k; // in Volts, note divide by e is automatic since k_B is in eV
	double L_D = std::sqrt( eps * kT_over_e / (2 * ee * (semiconductor.n_i * 1E6)) ); // Debye Length

	double flat_band_voltage = [&semiconductor, &insulator, &contact]
	{
		double work_function_difference = contact.work_function - semiconductor.work_function; // Units are both eV and V since no conversion is needed
		double oxide_voltage = -insulator.interface_charge * ee / insulator.capacitance; // In volts
		return oxide_voltage + work_function_difference;
	}();
	//arma::vec surface_potential = bias_voltages - flat_band_voltage
	//	- kT_over_e * semiconductor.eps_s / insulator.eps_s * insulator.thickness / L_D * F;
	//arma::vec U_S = surface_potential / kT_over_e; // Normalized surface potential

	double U_F = semiconductor.fermi_potential / kT_over_e; // Normalized fermi potential
	arma::vec U_S( bias_voltages.size() );
	int i = 0;
	for( double bias : bias_voltages )
	{
		auto test_Func = [=]( double U_S )
		{
			double F = std::sqrt( std::exp( U_F ) * (std::exp( -U_S ) + U_S - 1) + std::exp( -U_F ) * (std::exp( U_S ) - U_S - 1) );
			double bias_drop_across_oxide = kT_over_e * semiconductor.eps_s / insulator.eps_s * insulator.thickness / L_D * F;
			if( std::signbit( U_S ) )
				bias_drop_across_oxide = -bias_drop_across_oxide;
			double surface_potential = U_S * kT_over_e;
			double needs_to_be_zero = bias - flat_band_voltage - surface_potential - bias_drop_across_oxide;
			return -needs_to_be_zero;
		};

		//auto test_dFuncdU_S = [=]( double U_S )
		//{
		//	double dbias_drop_across_oxide = kT_over_e * semiconductor.eps_s / insulator.eps_s * insulator.thickness / L_D * dF;
		//	double dsurface_potential = kT_over_e;
		//	return - dsurface_potential - dbias_drop_across_oxide;
		//};

		//double test = Newtons_Method( test_Func, test_dFuncdU_S, /*starting_point =*/ 1.0, /*resolution =*/ 1E-9, /*max_iteration_count =*/ 100 );
		double test = Binary_Search( test_Func, /*left_most =*/ -5.0 / kT_over_e, /*right_most =*/ 5.0 / kT_over_e, /*resolution =*/ 1E-9, /*max_iteration_count =*/ 100 );
		U_S[ i++ ] = test;
	}

	arma::vec F = arma::sqrt( std::exp( U_F ) * (arma::exp( -U_S ) + U_S - 1) + std::exp( -U_F ) * (arma::exp( U_S ) - U_S - 1) );

	arma::vec C_S = [&]
	{
		if( frequency_in_hz == 0 )
		{
			arma::vec C_S_lf = eps / (2 * L_D) * (std::exp( U_F ) * (1 - arma::exp( -U_S )) + std::exp( -U_F ) * (arma::exp( U_S ) - 1)) / F;
			auto test1 = (std::exp( U_F ) * (1 - arma::exp( -U_S )))[ 100 ];
			auto test2 = (std::exp( -U_F ) * (arma::exp( U_S ) - 1))[ 100 ];
			auto test3 = F[ 100 ];
			auto test4 = C_S_lf[ 100 ];
			C_S_lf = arma::sign( U_S ) % C_S_lf;
			return C_S_lf;
		}
		else
		{
			arma::vec delta( bias_voltages.size() );
			int i = 0;
			for( double U_S_i : U_S )
			{
				//{ // Old version with trapezoidal integration
				//	arma::vec U = arma::linspace( 0, U_S_i, 101 );
				//	arma::vec F_U = arma::sqrt( std::exp( U_F ) * (arma::exp( -U ) + U - 1) + std::exp( -U_F ) * (arma::exp( U ) - U - 1) );
				//	F_U[ 0 ] = 0.0;
				//	arma::vec func = std::exp( U_F ) * (1 - arma::exp( -U )) % (arma::exp( U ) - U - 1) / (2 * F_U % F_U % F_U);
				//	func[ 0 ] = 0.0;
				//	double integral = arma::vec( arma::trapz( func ) / (U[ 1 ] - U[ 0 ]) )[ 0 ];
				//}

				auto f = [=]( double u )
				{
					double F_U = std::sqrt( std::exp( U_F ) * (std::exp( -u ) + u - 1) + std::exp( -U_F ) * (std::exp( u ) - u - 1) );
					if( u == 0 )
						return 1 / ( 4 * std::pow( std::cosh( U_F ), 1.5 ) ); // lim as u -> 0
					else
						return std::exp( U_F ) * (1 - std::exp( -u )) * (std::exp( u ) - u - 1) / (2 * F_U * F_U * F_U);
				};
				double integral = Rosetta::GaussLegendreQuadrature<15>().integrate( 0, U_S_i, f );
				double test123 = f( 0.0 );
				double numerator = (std::exp( U_S_i ) - U_S_i - 1) / F[ i ];
				delta[ i++ ] = numerator / integral;
			}

			arma::vec C_S_hf = eps / (2 * L_D) * ( std::exp( U_F ) * (1 - arma::exp( -U_S )) + std::exp( -U_F ) * (arma::exp( U_S ) - 1) / (1 + delta) ) / F;
			C_S_hf = arma::sign( U_S ) % C_S_hf;
			return C_S_hf;
		}
	}();

	return C_S;
}

CV_Data Get_MOS_Capacitance( const Semiconductor & semiconductor, const Insulator & insulator, const Metal & contact, double temperature_in_k, double lower_bound, double upper_bound, double frequency )
{
	double kT_over_e = k_B * temperature_in_k; // in Volts, note divide by e is automatic since k_B is in eV
	arma::vec bias_voltages = arma::linspace( lower_bound, upper_bound, 1000 );
	double C_O = insulator.capacitance; // Oxide Capacitance per cm^2
	arma::vec C_S = 1E-4 * Frequency_Semiconductor_Capacitance( bias_voltages, semiconductor, insulator, contact, temperature_in_k, frequency );

	arma::vec capacitances = 1.0 / (1.0 / C_S + 1.0 / C_O);
	//return { bias_voltages, C_S_lf };
	//return { bias_voltages, capacitances / C_O };
	//if( frequency == 0 )
		return { std::move( bias_voltages ), std::move( capacitances ) };
	//else
		//return { std::move( bias_voltages ), std::move( C_S ) };
}

Insulator::Insulator( const Material_Constants & material, double thickness, double interface_charge ) :
	Material_Constants( material ), thickness( thickness ), interface_charge( interface_charge )
{
	capacitance = material.eps_s * epsilon_0 / thickness * 1E-4; // Capacitance per cm^2
}

Semiconductor::Semiconductor( const Material_Constants & material, double doping, double temperature_in_k ) :
	Material_Constants( material ), doping( doping )
{
	double kT_over_e = k_B * temperature_in_k; // in Volts, note divide by e is automatic since k_B is in eV
	double log_arguement = std::max( 1.0, std::abs( doping ) / this->n_i ); // If the doping isn't greater than the intrinsic carrier concentration, there is no energy offset
	this->fermi_potential = kT_over_e * std::log( log_arguement ) * sign( doping );
	this->work_function = this->affinity + this->Ec_minus_Ei + this->fermi_potential;

}

};