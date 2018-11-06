#include "FTIR_Analyzer.h"
#include <QtWidgets/QApplication>

#include <armadillo>

#include "Thin_Film_Interference.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	FTIR_Analyzer w;
	w.show();
	return a.exec();
}

#include "optim.hpp"
#include "ceres/ceres.h"

using ceres::AutoDiffCostFunction;
using ceres::CostFunction;
using ceres::Problem;
using ceres::Solver;
using ceres::Solve;

#include <dlib/optimization.h>
#include <dlib/global_optimization.h>

typedef dlib::matrix<double, 0, 1> column_vector;
double rosen( const column_vector& m )
/*
	This function computes what is known as Rosenbrock's function.  It is
	a function of two input variables and has a global minimum at (1,1).
	So when we use this function to test out the optimization algorithms
	we will see that the minimum found is indeed at the point (1,1).
*/
{
	const double x = m( 0 );
	const double y = m( 1 );

	// compute Rosenbrock's function and return the result
	return 100.0*pow( y - x * x, 2 ) + pow( 1 - x, 2 );
}

// This is a helper function used while optimizing the rosen() function.  
const column_vector rosen_derivative( const column_vector& m )
/*!
	ensures
		- returns the gradient vector for the rosen function
!*/
{
	const double x = m( 0 );
	const double y = m( 1 );

	// make us a column vector of length 2
	column_vector res( 2 );

	// now compute the gradient vector
	res( 0 ) = -400 * x*(y - x * x) - 2 * (1 - x); // derivative of rosen() with respect to x
	res( 1 ) = 200 * (y - x * x);              // derivative of rosen() with respect to y
	return res;
}

// This function computes the Hessian matrix for the rosen() fuction.  This is
// the matrix of second derivatives.
dlib::matrix<double> rosen_hessian( const column_vector& m )
{
	const double x = m( 0 );
	const double y = m( 1 );

	dlib::matrix<double> res( 2, 2 );

	// now compute the second derivatives 
	res( 0, 0 ) = 1200 * x*x - 400 * y + 2; // second derivative with respect to x
	res( 1, 0 ) = res( 0, 1 ) = -400 * x;   // derivative with respect to x and y
	res( 1, 1 ) = 200;                 // second derivative with respect to y
	return res;
}

// ----------------------------------------------------------------------------------------

class rosen_model
{
	/*!
		This object is a "function model" which can be used with the
		find_min_trust_region() routine.
	!*/

public:
	typedef column_vector column_vector;
	typedef dlib::matrix<double> general_matrix;

	double operator() (
		const column_vector& x
		) const
	{
		return rosen( x );
	}

	void get_derivative_and_hessian(
		const column_vector& x,
		column_vector& der,
		general_matrix& hess
	) const
	{
		der = rosen_derivative( x );
		hess = rosen_hessian( x );
	}
};

////
//// Ackley function
//
//double ackley_fn( const arma::vec& vals_inp, arma::vec* grad_out, void* opt_data )
//{
//	const double x = vals_inp( 0 );
//	const double y = vals_inp( 1 );
//	const double pi = arma::datum::pi;
//
//	//double obj_val = -20 * std::exp( -0.2*std::sqrt( 0.5*(x*x + y * y) ) ) - std::exp( 0.5*(std::cos( 2 * pi*x ) + std::cos( 2 * pi*y )) ) + 22.718282L;
//
//	return (x - 1) * (x - 1) + y * y;
//
//	//
//
//	//return obj_val;
//}
//
//template<class Area>
//auto cost_function( Area area, double desired )
//{
//	return [=]( auto const* shape, auto* residual )
//	{
//		using T = std::decay_t<decltype(*shape)>;
//		residual[ 0 ] = T( desired_area_ ) - area( shape );
//		return true;
//	};
//}
//auto triangle = []( auto* shape ) { return triangleArea( shape ); };
//auto tri_cost = cost_function( triangle, 3.14159 );
//ceres::CostFunction* cost_function_triangle = new ceres::AutoDiffCostFunction<decltype(tri_cost), 1, 2>(
//	new decltype(tri_cost)(tri_cost) );

//template <typename function_type>
//bool Minimize_Function( function_type func, const arma::vec & initial_guess, const arma::vec & lower_limits, const arma::vec & upper_limits )
//{
//	// First make gradient
//	for(  )
//	func
//}

arma::vec Gradient_Approximate( const arma::vec & x,
								std::function<double(const arma::vec &)> func,
								double resolution = 1E-9 )
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
							   double resolution = 1E-9 )
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
			double debug1 = func( x + offset_i + offset_j );
			double debug2 = func( x + offset_i - offset_j );
			double debug3 = func( x - offset_i + offset_j );
			double debug4 = func( x - offset_i - offset_j );
			double delta_f = func( x + offset_i + offset_j ) - func( x + offset_i - offset_j ) - func( x - offset_i + offset_j ) + func( x - offset_i - offset_j );
			if( abs( delta_f ) < 1E-9 )
				int i = 0;
			double debug5 = delta_f / denominator;
			double result = delta_f;// / denominator;
			hessian_matrix( i, j ) = result;
			hessian_matrix( j, i ) = result;
		}
	}

	return hessian_matrix;
}

arma::vec Minimize_Function_Starting_Point( std::function<double( const arma::vec & )> function_to_minimize,
							 const arma::vec & starting_point,
							 int max_iteration_count = 100,
						     double attenuation_coefficient = 1.0,
							 double resolution = 1E-9 )
{
	arma::vec current_guess = starting_point;
	arma::vec previous_direction = arma::zeros( current_guess.size() );
	for( int i = 0; i < max_iteration_count; i++ )
	{
		std::cout << "current_guess: " << function_to_minimize( current_guess ) << " = " << current_guess( 0 ) << " " << current_guess( 1 ) << " " << current_guess( 2 ) << std::endl;

		arma::vec gradient = Gradient_Approximate( current_guess, function_to_minimize, resolution );
		//gradient = gradient( arma::span( 0, 1 ) );
		//std::cout << "gradient: " << gradient << std::endl;
		arma::mat hessian = Hessian_Approximate( current_guess, function_to_minimize, resolution );
		//hessian = hessian( arma::span( 0, 1 ), arma::span( 0, 1 ) );
		//std::cout << "hessian: " << hessian << std::endl;
		arma::mat inverse_hessian;
		try
		{
			inverse_hessian = arma::inv( hessian );
		}
		catch(...)
		{
			current_guess = 1.1 * current_guess;
			continue;
		}
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
		arma::vec move_vector = std::min( arma::norm(to_zero), 1000 * resolution ) * arma::normalise( to_zero );
		//double length = arma::norm( gradient ) / (2 * resolution);
		//arma::vec move_vector = -arma::normalise( gradient ) * std::min( length, 1000 * resolution );
		//arma::vec extend_thing = { 0, 0, 0 };
		//extend_thing( arma::span( 0, 1 ) ) = move_vector;
		//move_vector = extend_thing;
		//move_vector( 2 ) = 0;
		std::cout << "Move vector: " << move_vector << std::endl;
		current_guess = current_guess + move_vector;
		//std::cout << "current_guess: " << current_guess( 0 ) << " " << current_guess( 1 ) << std::endl;
		if( arma::dot( move_vector, previous_direction ) < 0 )
			resolution *= 0.9;

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

int main2()
{
	auto test_func = []( const arma::vec & x )
	{
		double x_part = x( 0 ) - 4;
		double y_part = x( 1 ) - 2;
		double z_part = x( 2 ) - 1;
		return x_part * x_part * x_part * x_part + y_part * y_part * y_part * y_part + z_part * z_part * z_part * z_part
			+ x_part * x_part * y_part * y_part;
	};

	//arma::vec solution_for_test_func = Minimize_Function_Starting_Point( test_func, arma::zeros<arma::vec>( 3 ), 100, 1.0, 1E-3 );

	if( 0 )
	{
		// Set the starting point to (4,8).  This is the point the optimization algorithm
	 // will start out from and it will move it closer and closer to the function's 
	 // minimum point.   So generally you want to try and compute a good guess that is
	 // somewhat near the actual optimum value.
		column_vector starting_point = { 4, 8 };

		// The first example below finds the minimum of the rosen() function and uses the
		// analytical derivative computed by rosen_derivative().  Since it is very easy to
		// make a mistake while coding a function like rosen_derivative() it is a good idea
		// to compare your derivative function against a numerical approximation and see if
		// the results are similar.  If they are very different then you probably made a 
		// mistake.  So the first thing we do is compare the results at a test point: 
		std::cout << "Difference between analytic derivative and numerical approximation of derivative: "
			<< dlib::length( dlib::derivative( rosen )(starting_point) - rosen_derivative( starting_point ) ) << endl;


		std::cout << "Find the minimum of the rosen function()" << endl;
		// Now we use the find_min() function to find the minimum point.  The first argument
		// to this routine is the search strategy we want to use.  The second argument is the 
		// stopping strategy.  Below I'm using the objective_delta_stop_strategy which just 
		// says that the search should stop when the change in the function being optimized 
		// is small enough.

		// The other arguments to find_min() are the function to be minimized, its derivative, 
		// then the starting point, and the last is an acceptable minimum value of the rosen() 
		// function.  That is, if the algorithm finds any inputs to rosen() that gives an output 
		// value <= -1 then it will stop immediately.  Usually you supply a number smaller than 
		// the actual global minimum.  So since the smallest output of the rosen function is 0 
		// we just put -1 here which effectively causes this last argument to be disregarded.

		dlib::find_min( dlib::bfgs_search_strategy(),  // Use BFGS search algorithm
						dlib::objective_delta_stop_strategy( 1e-7 ), // Stop when the change in rosen() is less than 1e-7
						rosen, rosen_derivative, starting_point, -1 );
		// Once the function ends the starting_point vector will contain the optimum point 
		// of (1,1).
		std::cout << "rosen solution:\n" << starting_point << endl;


		// Now let's try doing it again with a different starting point and the version
		// of find_min() that doesn't require you to supply a derivative function.  
		// This version will compute a numerical approximation of the derivative since 
		// we didn't supply one to it.
		starting_point = { -94, 5.2 };
		dlib::find_min_using_approximate_derivatives( dlib::bfgs_search_strategy(),
													  dlib::objective_delta_stop_strategy( 1e-7 ),
													  rosen, starting_point, -1 );
		// Again the correct minimum point is found and stored in starting_point
		std::cout << "rosen solution:\n" << starting_point << endl;

		auto complex_holder_table = []( double x0, double x1 )
		{
			// This function is a version of the well known Holder table test
			// function, which is a function containing a bunch of local optima.
			// Here we make it even more difficult by adding more local optima
			// and also a bunch of discontinuities. 

			// add discontinuities
			double sign = 1;
			for( double j = -4; j < 9; j += 0.5 )
			{
				if( j < x0 && x0 < j + 0.5 )
					x0 += sign * 0.25;
				sign *= -1;
			}
			// Holder table function tilted towards 10,10 and with additional
			// high frequency terms to add more local optima.
			return -(std::abs( sin( x0 )*cos( x1 )*exp( std::abs( 1 - std::sqrt( x0*x0 + x1 * x1 ) / dlib::pi ) ) ) - (x0 + x1) / 10 - sin( x0 * 10 )*cos( x1 * 10 ));
		};

		// To optimize this difficult function all we need to do is call
		// find_min_global()
		auto result = dlib::find_min_global( complex_holder_table,
									   { -10,-10 }, // lower bounds
									   { 10,10 }, // upper bounds
											 dlib::max_function_calls( 300 ) );

		std::cout.precision( 9 );
	}

	std::ofstream testing_file( "Why you no work.txt" );
	std::cout.precision( 17 );
	testing_file.precision( 17 );

	Thin_Film_Interference test;
	arma::vec wavelengths = arma::linspace( 5E-6, 20E-6, 1001 );
	//std::vector<double> Get_Expected_Transmission( double temperature_k, const std::vector<Material_Layer> & layers, const arma::vec & wavelengths );

	std::vector<Material_Layer> layers{ {Material::Si, 0.5, 1E-6}, {Material::CdTe, 0.5, 5E-6}, {Material::HgCdTe, 0.5, 1E-6} };
	arma::vec data_results = test.Get_Expected_Transmission( 300.0, layers, wavelengths );
	for( double x : data_results )
		testing_file << x << "\t";
	testing_file << "\n";
	//arma::vec data_results2 = test.Get_Expected_Transmission( 300.0, layers, wavelengths );
	arma::cx_vec data_results2 = arma::fft( data_results );
	testing_file << "Fourier transform\n";
	for( arma::cx_double x : data_results2 )
		testing_file << std::real( x ) << "\t";
	testing_file << "\n";
	for( arma::cx_double x : data_results2 )
		testing_file << std::imag( x ) << "\t";
	testing_file << "\n";

	//auto minimize_function = [&test, &layers, &wavelengths, &data_results]( const column_vector& input_to_optimize )
	auto minimize_function = [&test, layers, &wavelengths, &data_results]( const arma::vec& input_to_optimize, arma::vec* grad_out, void* opt_data )
	{
		std::vector<Material_Layer> copy_layers = layers;
		//layers[ 0 ].thickness = layer1;
		//layers[ 1 ].thickness = layer2;
		//layers[ 2 ].thickness = layer3;
		bool invalid_input = false;
		for( int i = 0; i < input_to_optimize.size(); i++ )
		{
			copy_layers[ i ].thickness = input_to_optimize( i );
			if( input_to_optimize( i ) < 0 )
				invalid_input = true;
		}
		if( invalid_input )
			return 9999999999.;
		arma::vec results = test.Get_Expected_Transmission( 300.0, copy_layers, wavelengths );
		arma::vec difference = results - data_results;
		double error = arma::dot( difference, difference );
		//std::cout << input_to_optimize << std::endl;
		//std::cout << layer1 << " " << layer2 << " " << layer3 << std::endl;
		//std::cout << error << std::endl;
		return error;
	};
	auto minimize_function2 = [&test, layers, &wavelengths, &data_results, &testing_file]( const arma::vec& input_to_optimize )
	{
		std::vector<Material_Layer> copy_layers = layers;
		//layers[ 0 ].thickness = layer1;
		//layers[ 1 ].thickness = layer2;
		//layers[ 2 ].thickness = layer3;
		bool invalid_input = false;
		for( int i = 0; i < input_to_optimize.size(); i++ )
		{
			copy_layers[ i ].thickness = input_to_optimize( i );
			if( input_to_optimize( i ) < 0 )
				invalid_input = true;
		}
		if( invalid_input )
			return 9999999999.;
		arma::vec results = test.Get_Expected_Transmission( 300.0, copy_layers, wavelengths );
		testing_file << "Other Data\n";
		for( double x : results )
			testing_file << x << "\t";
		testing_file << "\n";
		arma::vec difference = results - data_results;
		testing_file << "Difference\n";
		for( double x : difference )
			testing_file << x << "\t";
		testing_file << "\n";

		double sum_of_squares = 0;
		for( double x : difference )
			sum_of_squares += x * x;

		double error = arma::dot( difference, difference );
		//std::cout << input_to_optimize << std::endl;
		//std::cout << error << ": " << input_to_optimize( 0 ) << " " << input_to_optimize( 1 ) << " " << input_to_optimize( 2 ) << std::endl;
		//std::cout << error << ": " << input_to_optimize( 0 ) << " " << input_to_optimize( 1 ) << std::endl;
		//std::cout << error << std::endl;
		return error;
	};
	arma::vec testpoint3 = { 1E-6, 5E-6 };
	double result_again = minimize_function2( testpoint3 );
	for( double x : data_results )
		testing_file << x << "\t";
	testing_file << "\n";
	arma::vec data_results3 = test.Get_Expected_Transmission( 300.0, layers, wavelengths );
	for( double x : data_results3 )
		testing_file << x << "\t";
	testing_file << "\n";

	arma::vec testpoint1 = { 7.77993e-07, 5.19409e-06, 5.2127e-07 };
	double result1 = minimize_function2( testpoint1 );
	testing_file << "Higher: " << result1 << " - " << testpoint1( 0 ) << ", " << testpoint1( 1 ) << ", " << testpoint1( 2 ) << "\n";
	arma::vec testpoint2 = { 9.88584e-07 - 1E-9, 4.97057e-06, 9.7873e-07 };
	double result2 = minimize_function2( testpoint2 );
	testing_file << "Lower: " << result2 << " - " << testpoint2( 0 ) << ", " << testpoint2( 1 ) << ", " << testpoint2( 2 ) << "\n";
	double result3 = minimize_function2( testpoint3 );
	testing_file << "Higher: " << result3 << " - " << testpoint3( 0 ) << ", " << testpoint3( 1 ) << "\n";
	testing_file.flush();
	//arma::vec starting_point = { 7.77993e-07, 5.19409e-06 };// , 5.2127e-07 };
	//arma::vec starting_point = { 1.1E-6, 5.1E-06, 1.1E-6 };// , 5.2127e-07 };
	arma::vec starting_point = { 1.1E-6, 5.5E-06, 1.0E-6 };// , 5.2127e-07 };
	arma::vec solution = Minimize_Function_Starting_Point( minimize_function2, starting_point, 1000, 1.0, 1E-10 );

	arma::vec testpoint4 = { 9.8853623499091619e-07, 5.0059360929261567e-06, 9.9480259067058758e-07 };
	testing_file << "Use this data:\n";
	arma::vec offset_5 = { 0, 1E-9, 0 };
	double result4 = minimize_function2( testpoint4 );
	double result5 = minimize_function2( testpoint4 + offset_5 );
	double result6 = minimize_function2( testpoint4 - offset_5 );
	testing_file.flush();
	std::cout << "Another Point: " << result4 << " : " << result5 << " : " << result6 << " : " << testpoint4( 0 ) << ", " << testpoint4( 1 ) << ", " << testpoint4( 2 ) << "\n";
	arma::vec gradient_yup = Gradient_Approximate( testpoint4, minimize_function2, 1E-9 );
	std::cout << "Another Gradient: " << gradient_yup( 0 ) << ", " << gradient_yup( 1 ) << ", " << gradient_yup( 2 ) << "\n";

#if 0
	std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
	//auto result = dlib::find_min_global( minimize_function,
	//									 //{ 0.5E-6, 2.5E-6, 0.5E-6 }, // lower bounds
	//									 //{ 2E-6, 10E-6, 2E-6 }, // upper bounds
	//									 { 100E-9, 100E-9, 100E-9 }, // lower bounds
	//									 { 20E-6, 20E-6, 20E-6 }, // upper bounds
	//									 dlib::max_function_calls( 3000 ) );
	//column_vector starting_point = { 100E-9, 100E-9, 100E-9 };
	column_vector starting_point = { 1.1E-6, 5.1E-6, 1.1E-6 };
	dlib::find_min_using_approximate_derivatives( dlib::bfgs_search_strategy(),
												  dlib::objective_delta_stop_strategy( 1e-7 ),
												  minimize_function, starting_point, -1 );
	std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds = end - start;
	std::cout << "elapsed time: " << elapsed_seconds.count() << "s\n";
	std::cout << "result:\n" << starting_point << "\n";
#endif

	if( 0 )
	{
		arma::vec out_range_x = arma::linspace( 0.5E-6, 1.1E-6, 3 );
		arma::vec out_range_y = arma::linspace( 4.5E-6, 5.1E-6, 3 );
		arma::vec out_range_z = arma::linspace( 0.5E-6, 1.1E-6, 3 );
		for( double z : out_range_z )
		{
			for( double y : out_range_y )
			{
				for( double x : out_range_x )
				{
					arma::vec values_to_try{ x, y, z };
					arma::vec result = Minimize_Function_Starting_Point( minimize_function2, values_to_try );
					std::cout << values_to_try( 0 ) << ", " << values_to_try( 1 ) << ", " << values_to_try( 2 )
						<< " : " << result( 0 ) << ", " << result( 1 ) << ", " << result( 2 ) << "\n";

				}
			}
		}
	}

	if( 0 )
	{
		arma::vec startpoint = { 1E-6, 5e-06, 1.0E-6 };
		//arma::vec startpoint = { 9.8853623499091619e-07, 5.0059360929261567e-06, 9.9480259067058758e-07 };
		arma::vec out_range_x = arma::linspace( startpoint( 0 ) - 1000e-9, startpoint( 0 ) + 1000e-9, 201 );
		arma::vec out_range_y = arma::linspace( startpoint( 1 ) - 1000e-9, startpoint( 1 ) + 1000e-9, 201 );
		arma::vec out_range_z = arma::linspace( startpoint( 1 ) - 100e-9, startpoint( 1 ) + 100e-9, 21 );
		//arma::vec out_range_z = { startpoint( 2 ) };
		//arma::vec out_range_x = arma::linspace( 0.5E-6, 1.5E-6, 101 );
		//arma::vec out_range_y = arma::linspace( 0.5E-6, 1.5E-6, 101 );
		////arma::vec out_range_y = { 1.1E-6 };
		////arma::vec out_range_z = arma::linspace( 4.5E-6, 5.5E-6, 101 );
		//arma::vec out_range_z = { 5.1E-6 };
		//arma::vec out_range_y = arma::linspace( 4.5E-6, 5.5E-6, 101 );
		std::ofstream test_out( "Test.csv" );
		for( double x : out_range_x )
		{
			test_out << x << "\t";
		}
		test_out << std::endl;
		for( double x : out_range_y )
		{
			test_out << x << "\t";
		}
		test_out << std::endl;

		//int i = 0;
		for( double z : out_range_z )
		{
			//std::ofstream test_out( "Test" + std::to_string(i++) +  ".csv" );

			for( double y : out_range_y )
			{
				for( double x : out_range_x )
				{
					//arma::vec values_to_try{ x, y, 4.49048e-07 };
					//arma::vec values_to_try{ x, 5.328E-6, y };
					arma::vec values_to_try{ x, y, z };
					//column_vector values_to_try{ x, y, 1.2168e-06 };
					test_out << minimize_function( values_to_try, nullptr, nullptr ) << "\t";
					//test_out << minimize_function( values_to_try ) << "\t";
				}
				test_out << std::endl;
			}
		}
	}

//#if 0
//	{
//		auto minimize_function2 = [&test, &layers, &wavelengths, &data_results]( const arma::vec& input_to_optimize, arma::vec* grad_out, void* opt_data )
//		{
//			for( int i = 0; i < 3; i++ )
//				layers[ i ].thickness = input_to_optimize( i );
//			arma::vec results = test.Get_Expected_Transmission( 300.0, layers, wavelengths );
//			arma::vec difference = results - data_results;
//			double error = arma::dot( difference, difference );
//			std::cout << input_to_optimize << std::endl;
//			std::cout << error << std::endl;
//			return error;
//		};
//
//		double m = 0.0;
//		double c = 0.0;
//
//		Problem problem;
//		for( auto layer : layers )
//		{
//			double currently = layer.thickness;
//			problem.AddResidualBlock(
//				new AutoDiffCostFunction<decltype(minimize_function), 1, 1, 1>(
//					new decltype(minimize_function)(minimize_function) ),
//				nullptr,
//				&m, &c );
//			//problem.AddResidualBlock(
//			//	new AutoDiffCostFunction<ExponentialResidual, 1, 1, 1>(
//			//		new ExponentialResidual( data[ 2 * i ], data[ 2 * i + 1 ] ) ),
//			//	NULL,
//			//	&m, &c );
//		}
//
//		Solver::Options options;
//		options.max_num_iterations = 25;
//		options.linear_solver_type = ceres::DENSE_QR;
//		options.minimizer_progress_to_stdout = true;
//
//		Solver::Summary summary;
//		Solve( options, &problem, &summary );
//		std::cout << summary.BriefReport() << "\n";
//		std::cout << "Initial m: " << 0.0 << " c: " << 0.0 << "\n";
//		std::cout << "Final   m: " << m << " c: " << c << "\n";
//	}
//#endif
	// From optim library
	if( 0 )
	{
		// initial values:
		arma::vec x = arma::zeros( 3 ) + 1E-9;
		//arma::vec x;
		optim::algo_settings_t settings;
		settings.vals_bound = true;
		//settings.de_n_gen = 100;
		//settings.iter_max = 100;
		settings.nm_adaptive = true;
		settings.lower_bounds.resize( 3 );
		settings.upper_bounds.resize( 3 );
		for( int i = 0; i < 3; i++ )
		{
			double currently = layers[ i ].thickness * 1E6;
			x( i ) = currently * 1.3;
			//x( i ) = currently;
			settings.lower_bounds( i ) = currently * 0.6;
			settings.upper_bounds( i ) = currently * 1.5;
		}
		settings.pso_initial_lb = settings.lower_bounds;
		settings.pso_initial_ub = settings.upper_bounds;
		settings.pso_center_particle = false;
		settings.pso_n_pop = 1000;
		settings.pso_n_gen = 10;
		//settings.pso_inertia_method = 2;
		settings.de_initial_lb = settings.lower_bounds;
		settings.de_initial_ub = settings.upper_bounds;
		//
		//double do_i_wor_at_all = minimize_function( x, nullptr, nullptr );

		std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();

		//for( double i : arma::linspace(100E-9,25E-6, 11) )
		{
			//arma::vec x{ i, i, i };
			arma::vec x{ 1.1, 5.1, 1 };
			//bool success = optim::broyden_df( x, minimize_function, nullptr, settings );
			bool success = optim::nm( x, minimize_function, nullptr, settings );
			std::cout << settings.opt_value << ": " << x( 0 ) << " " << x( 1 ) << " " << x( 2 ) << "\n";
		}

		std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
		std::chrono::duration<double> elapsed_seconds = end - start;

		//if( success )
		{
			std::cout << "de: Ackley test completed successfully.\n"
				<< "elapsed time: " << elapsed_seconds.count() << "s\n";
		}
		//else
		//{
		//	std::cout << "de: Ackley test completed unsuccessfully." << std::endl;
		//}

		//arma::cout << "\nde: solution to Ackley test:\n" << x << arma::endl;
	}
	while( 1 );
	return 0;
}