#pragma once


#include <armadillo>
#include <functional>

arma::vec Gradient_Approximate( const arma::vec & x,
								std::function<double( const arma::vec & )> func,
								double resolution = 1E-9 );

arma::mat Hessian_Approximate( const arma::vec & x,
							   std::function<double( const arma::vec & )> func,
							   double resolution = 1E-9 );
arma::vec Minimize_Function_Starting_Point( std::function<double( const arma::vec & )> function_to_minimize,
											const arma::vec & starting_point,
											int max_iteration_count = 100,
											double attenuation_coefficient = 1.0,
											double resolution = 1E-9,
											double biggest_step_size = 0.25E-6,
											std::function<bool( arma::vec )> iteration_finished_callback = []( arma::vec ) { return false;  } );

double Newtons_Method( std::function<double( double )> func,
					   std::function<double( double )> derivative,
					   double starting_point = 1.0,
					   double resolution = 1E-9,
					   int max_iteration_count = 100 );

double Binary_Search( std::function<double( double )> func,
					  double left_most = -1.0,
					  double right_most = 1.0,
					  double resolution = 1E-9,
					  int max_iteration_count = 100 );

arma::vec Fit_Data_To_Function( std::function<arma::vec( const arma::vec &, const arma::vec & )> function,
	const arma::vec & x_data,
	const arma::vec & y_data,
	arma::vec lower_bounds,
	arma::vec upper_bounds,
	double resolution = 1E-5,
	int max_iteration_count = 1000,
	double learning_factor = 1E-4 );
