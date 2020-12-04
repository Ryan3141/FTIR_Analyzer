#include "Optimize.h"

#include <armadillo>
#include <functional>

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
											double biggest_step_size,
											std::function<void( arma::vec )> iteration_finished_callback )
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
		arma::vec move_vector = std::min( arma::norm( to_zero ), biggest_step_size ) * arma::normalise( to_zero );
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
		iteration_finished_callback( current_guess );
		//std::cout << "current_guess: " << current_guess( 0 ) << " " << current_guess( 1 ) << std::endl;
		if( arma::dot( move_vector, move_vector ) < resolution * resolution )
			break; // Quit out if we are barely moving anymore
		if( arma::dot( move_vector, previous_direction ) < 0 )
			biggest_step_size *= 0.9;

		previous_direction = move_vector;
	}

	return current_guess;
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
