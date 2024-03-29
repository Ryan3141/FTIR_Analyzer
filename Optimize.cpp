#include "Optimize.h"

#include <armadillo>
#include <functional>
#include "nelder_mead.h"

#include "rangeless_helper.hpp"

arma::vec Gradient_Approximate( const arma::vec & x,
								std::function<double( const arma::vec & )> func,
								double resolution )
{
	arma::vec gradient_vector = arma::zeros( x.size() );
	for( auto i = 0; i < x.size(); i++ )
	{
		arma::vec offset = arma::zeros<arma::vec>( x.size() );
		offset( i ) = resolution;
		//double debug1 = func( x + offset );
		//double debug2 = func( x - offset );

		//gradient_vector( i ) = (func( x + offset ) - func( x - offset )) / (2 * resolution);
		gradient_vector( i ) = (func( x + offset ) - func( x - offset ));// / (2 * resolution);
	}

	return gradient_vector;
}

arma::vec Gradient_Approximate( const arma::vec & x,
	std::function<double( const arma::vec & )> func,
	arma::vec resolution )
{
	arma::vec gradient_vector = arma::zeros( x.size() );
	for( auto i = 0; i < x.size(); i++ )
	{
		arma::vec offset = arma::zeros<arma::vec>( x.size() );
		offset( i ) = resolution( i );
		std::cout << "Dimension " << i << " = " << func( x + offset ) << " " << func( x - offset ) << "\n";
		gradient_vector( i ) = ( func( x + offset ) - func( x - offset ) );// / (2 * resolution);
	}
	std::cout << "Gradient =";
	for( double x : gradient_vector )
		std::cout << " " << x;
	std::cout << std::endl;
	return gradient_vector / ( 2 * resolution );
}

arma::mat Hessian_Approximate( const arma::vec & x,
							   std::function<double( const arma::vec & )> func,
							   double resolution )
{
	arma::mat hessian_matrix( x.size(), x.size() );
	const double denominator = 4 * resolution * resolution;
	for( int j = 0; j < x.size(); j++ )
	{
		arma::vec offset_j = arma::zeros<arma::vec>( x.size() );
		offset_j( j ) = resolution;
		for( int i = 0; i <= j; i++ )
		{
			arma::vec offset_i = arma::zeros<arma::vec>( x.size() );
			offset_i( i ) = resolution;
			//double debug1 = func( x + offset_i + offset_j );
			//double debug2 = func( x + offset_i - offset_j );
			//double debug3 = func( x - offset_i + offset_j );
			//double debug4 = func( x - offset_i - offset_j );
			double delta_f = func( x + offset_i + offset_j ) - func( x + offset_i - offset_j ) - func( x - offset_i + offset_j ) + func( x - offset_i - offset_j );
			if( abs( delta_f ) < 1E-9 )
				int i = 0;
			//double debug5 = delta_f / denominator;
			double result = delta_f;// / denominator;
			hessian_matrix( i, j ) = result;
			hessian_matrix( j, i ) = result;
		}
	}

	return hessian_matrix;
}

arma::vec Minimize_Function_Starting_Point( std::function<double( const arma::vec & )> function_to_minimize,
											const arma::vec & starting_point,
											int max_iteration_count,
											double attenuation_coefficient,
											double resolution,
											double biggest_step_ratio,
											std::function<bool( arma::vec )> iteration_finished_callback )
{
	arma::vec current_guess = starting_point;
	arma::vec previous_direction = arma::zeros( current_guess.size() );
	for( int i = 0; i < max_iteration_count; i++ )
	{
		std::cout << "current_guess: " << function_to_minimize( current_guess ) << " =";
		for( double x : current_guess )
			std::cout << " " << x;
		std::cout << std::endl;

		arma::vec gradient = Gradient_Approximate( current_guess, function_to_minimize, resolution );
		//gradient = gradient( arma::span( 0, 1 ) );
		//std::cout << "gradient: " << gradient << std::endl;
		//arma::mat hessian = Hessian_Approximate( current_guess, function_to_minimize, resolution );
		////hessian = hessian( arma::span( 0, 1 ), arma::span( 0, 1 ) );
		////std::cout << "hessian: " << hessian << std::endl;
		//arma::mat inverse_hessian;
		//try
		//{
		//	inverse_hessian = arma::inv( hessian );
		//}
		//catch( ... )
		//{
		//	current_guess = 1.1 * current_guess;
		//	continue;
		//}
		//std::cout << "inverse_hessian: " << inverse_hessian << std::endl;

		//arma::vec move_vector = -attenuation_coefficient * (inverse_hessian * gradient) * (2 * resolution);
		//if( arma::norm( move_vector ) > 10 * resolution )
		//	move_vector = move_vector / arma::norm( move_vector ) * 10 * resolution;
		//for( int j = 0; j < current_guess.size(); j++ )
		//{
		//	if( abs( move_vector( j ) ) > 0.0005 * abs( current_guess( j ) ) )
		//		move_vector( j ) = 0.0005 * abs( current_guess( j ) ) * move_vector( j ) / abs( move_vector( j ) );
		//}
		arma::vec to_zero = -gradient / (2 * resolution) / function_to_minimize( current_guess );
		arma::vec move_vector = arma::min( arma::abs(to_zero), biggest_step_ratio * current_guess ) % arma::sign( to_zero );
		//double length = arma::norm( gradient ) / (2 * resolution);
		//arma::vec move_vector = -arma::normalise( gradient ) * std::min( length, 1000 * resolution );
		//arma::vec extend_thing = { 0, 0, 0 };
		//extend_thing( arma::span( 0, 1 ) ) = move_vector;
		//move_vector = extend_thing;
		//move_vector( 2 ) = 0;
		std::cout << "Move vector: ";
		for( double x : move_vector )
			std::cout << " " << x;
		std::cout << std::endl;
		current_guess = current_guess + move_vector;
		if( iteration_finished_callback( current_guess ) ) // Check quit early
			return current_guess;
		//std::cout << "current_guess: " << current_guess( 0 ) << " " << current_guess( 1 ) << std::endl;
		if( arma::dot( move_vector, move_vector ) < resolution * resolution )
			break; // Quit out if we are barely moving anymore
		if( arma::dot( move_vector, previous_direction ) < 0 )
			biggest_step_ratio *= 0.9;

		previous_direction = move_vector;
	}

	return current_guess;
}

//arma::vec Fit_Data_To_Function( std::function<arma::vec( const arma::vec &, const arma::vec & )> function,
//	const arma::vec & x_data,
//	const arma::vec & y_data,
//	arma::vec lower_bounds,
//	arma::vec upper_bounds,
//	double resolution,
//	int max_iteration_count,
//	double learning_factor )
//{
//	const arma::vec bounds_range = upper_bounds - lower_bounds;
//	arma::vec current_guess = ( upper_bounds + lower_bounds ) / 2;
//	const arma::vec resolution_vec = bounds_range * resolution;
//	//arma::vec previous_direction = arma::zeros( current_guess.size() );
//	auto error_function = [ &x_data, &y_data, function ]( arma::vec fit_params ) -> double {
//		arma::vec diff = y_data - function( fit_params, x_data );
//		return arma::dot( diff, diff );
//	};
//	//arma::vec learning_factor = bounds_range * 1E-6;
//	for( int i = 0; i < max_iteration_count; i++ )
//	{
//		std::cout << "current_guess =";
//		for( double x : current_guess )
//			std::cout << " " << x;
//		std::cout << std::endl;
//
//		arma::vec gradient = Gradient_Approximate( current_guess, error_function, resolution_vec );
//		//arma::vec to_zero = -gradient / ( 2 * resolution ) / function_to_minimize( current_guess );
//		//arma::vec move_vector = arma::min( arma::abs( to_zero ), biggest_step_ratio * current_guess ) % arma::sign( to_zero );
//		//double length = arma::norm( gradient ) / (2 * resolution);
//		//arma::vec move_vector = -arma::normalise( gradient ) % bounds_range * 0.1;
//		//arma::vec move_vector = -gradient % bounds_range * 0.1;
//		arma::vec move_vector = -gradient * learning_factor;
//		double move_vector_amplitude = move_vector[ 0 ];
//		double move_vector_offset = move_vector[ 1 ];
//		double move_vector_tau = move_vector[ 2 ];
//
//		double current_guess_amplitude = current_guess[ 0 ];
//		double current_guess_offset = current_guess[ 1 ];
//		double current_guess_tau = current_guess[ 2 ];
//
//		//learning_factor *= 0.5;
//		//arma::vec extend_thing = { 0, 0, 0 };
//		//extend_thing( arma::span( 0, 1 ) ) = move_vector;
//		//move_vector = extend_thing;
//		//move_vector( 2 ) = 0;
//		current_guess = arma::min( upper_bounds, arma::max( lower_bounds, current_guess + move_vector ) );
//		if( arma::dot( move_vector, move_vector ) < resolution * resolution )
//			break; // Quit out if we are barely moving anymore
//
//		//previous_direction = move_vector;
//	}
//
//	return current_guess;
//}

template< typename ArrayType >
arma::vec to_vec( const ArrayType & from )
{
	arma::vec to( from.size() );
	std::copy( from.begin(), from.end(), to.begin() );
	return to;
}

template< typename VecType, typename real, int n >
std::array<real, n> to_array( const VecType & from )
{
	assert( n == from.size() );
	std::array<real, n> to;
	std::copy( from.begin(), from.end(), to.begin() );
	return to;
}


arma::vec Fit_Data_To_Function( std::function<arma::vec( const arma::vec &, const arma::vec & )> function,
	const arma::vec & x_data,
	const arma::vec & y_data,
	arma::vec lower_bounds,
	arma::vec upper_bounds,
	double resolution,
	int max_iteration_count,
	double learning_factor )
{
	const arma::vec bounds_range = upper_bounds - lower_bounds;
	arma::vec current_guess = ( upper_bounds + lower_bounds ) / 2;
	const arma::vec resolution_vec = bounds_range * resolution;
	//arma::vec previous_direction = arma::zeros( current_guess.size() );
	auto error_function = [ &x_data, &y_data, function ]( const std::array<double, 6> & fit_params ) -> double {
		arma::vec diff = y_data - function( to_vec( fit_params ), x_data );
		std::cout << fit_params[0] << " " << fit_params[1] << " " << fit_params[2] << ": " << arma::dot( diff, diff ) << "\n";
		return arma::dot( diff, diff );
	};

	nelder_mead_result<double, 6> result = nelder_mead<double, 6>(
		error_function,
		to_array<arma::vec, double, 6>( current_guess ),
		1.0e-25, // the terminating limit for the variance of function values
		to_array<arma::vec, double, 6>( resolution_vec ), 1, 10000 );

	return to_vec( result.xmin );
	//return arma::min( upper_bounds, arma::max( lower_bounds, to_vec( result.xmin ) ) );
}

arma::mat Minimize_Function( std::function<double( const arma::vec & )> function_to_minimize,
							 const std::vector< std::tuple<double, double> > & bounds,
							 const int max_iteration_count )
{
	return arma::mat();
}

double Newtons_Method( std::function<double( double )> func, std::function<double( double )> derivative, double starting_point, double resolution, int max_iteration_count )
{
	double x_i = starting_point;
	double x_i_old;
	int iteration = 0;
	for( int iteration = 0; iteration < max_iteration_count; iteration++ )
	{
		x_i_old = x_i;
		x_i = x_i - func( x_i ) / derivative( x_i );
		if( abs( x_i - x_i_old ) < resolution )
		{
			return x_i;
		}
	}
	return x_i;
}

double Binary_Search( std::function<double( double )> func, double left_most, double right_most, double resolution, int max_iteration_count )
{
	// Binary search
	double left = left_most, right = right_most;
	while( right - left > resolution )
	{
		double center = (right + left) / 2;
		if( func( center ) > 0 )
			right = center;
		else
			left = center;
	}
	double center = (right + left) / 2;

	return center;
}
