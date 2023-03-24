#pragma once

#include <string>
#include <armadillo>
#include <cppad/ipopt/solve.hpp>


extern const std::string ipopt_options;

namespace {
	using CppAD::AD;

	template< typename Minimizing_Function >
	class FG_eval {
	public:
		Minimizing_Function func;
		FG_eval( Minimizing_Function func ) : func( func ) {}
		//using ADvector = std::vector<double>;
		typedef CPPAD_TESTVECTOR( AD<double> ) ADvector;
		void operator()( ADvector& fg, const ADvector& x )
		{
			fg = func( x );
		}
	};
}

template< typename Single_Graph, typename Fit_Results >
std::array<Fit_Results, 2> CppAD_Fit_Lifetime( const arma::vec& initial_guess, const arma::vec& lower_limits, const arma::vec& upper_limits, const Single_Graph& graph )
{
	if( graph.x_data.empty() || graph.y_data.empty() )
		return { Fit_Results{ arma::datum::nan, arma::datum::nan, arma::datum::nan }, Fit_Results{ arma::datum::nan, arma::datum::nan, arma::datum::nan } };
	arma::vec x = fromQVec( graph.x_data );
	arma::vec y = fromQVec( graph.y_data );

	double x_offset = x( y.index_max() ); // Align all of the peaks
	x = x - x_offset;
	y = y - y.min() + 1E-9;

	using ADvector = CPPAD_TESTVECTOR( AD<double> );
	auto sum_of_exponential_functions = []( const ADvector& fit_params, const arma::vec& time ) -> ADvector
	{
		AD<double> A = fit_params[ 0 ];
		AD<double> offset1 = fit_params[ 1 ];
		AD<double> tau1 = fit_params[ 2 ];
		AD<double> B = fit_params[ 3 ];
		AD<double> offset2 = fit_params[ 4 ];
		AD<double> tau2 = fit_params[ 5 ];
		ADvector result( time.size() );
		for( auto i = 0; i < time.size(); i++ )
			result[ i ] = A * CppAD::exp( -(time[ i ] - offset1) / tau1 ) + B * CppAD::exp( -(time[ i ] - offset2) / tau2 );
		return result;
		//return ln_B + x2 + arma::log( std::exp( ln_A - ln_B ) * arma::exp( x1 - x2 ) + 1 );
	};

	arma::uvec selection_region = arma::find( x > graph.lower_x_fit && x < graph.upper_x_fit2 );
	arma::vec x_fit_data = x( selection_region );
	arma::vec y_fit_data = y( selection_region );

	auto error_function = [&x_fit_data, &y_fit_data, sum_of_exponential_functions]( const ADvector& fit_params ) -> ADvector {
		ADvector exp_results = sum_of_exponential_functions( fit_params, x_fit_data );
		AD<double> sum_of_squares = 0;
		for( auto i = 0; i < exp_results.size(); i++ )
		{
			auto difference = y_fit_data[ i ] - exp_results[ i ];
			sum_of_squares += difference * difference;
		}
		ADvector results( 1 );
		results[ 0 ] = sum_of_squares;
		return results;
		//arma::vec diff = y_fit_data - sum_of_exponential_functions( fit_params, x_fit_data );
		//std::cout << fit_params[ 0 ] << " " << fit_params[ 1 ] << " " << fit_params[ 2 ] << ": " << arma::dot( diff, diff ) << "\n";
		//return { arma::dot( diff, diff ) };
	};

	//arma::vec fit_params = Fit_Data_To_Function( sum_of_exponential_functions, x( selection_region ), arma::log( y( selection_region ) ), lower_limits, upper_limits );
	CppAD::ipopt::solve_result<arma::vec> solution;
	FG_eval fg_eval{ error_function };
	CppAD::ipopt::solve( ipopt_options, initial_guess, lower_limits, upper_limits, {}, {}, fg_eval, solution );
	arma::vec fit_params = solution.x;

	if( fit_params[ 2 ] < fit_params[ 5 ] )
		return { Fit_Results{ (fit_params[ 0 ]), fit_params[ 1 ], fit_params[ 2 ] }, Fit_Results{ (fit_params[ 3 ]), fit_params[ 4 ], fit_params[ 5 ] } };
	else
		return { Fit_Results{ (fit_params[ 3 ]), fit_params[ 4 ], fit_params[ 5 ] }, Fit_Results{ (fit_params[ 0 ]), fit_params[ 1 ], fit_params[ 2 ] } };
}
