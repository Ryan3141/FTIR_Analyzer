//#include "FTIR_Analyzer.h"
#include <QtWidgets/QApplication>

#include "IV_By_Size_Plotter.h"
#include "IV_Plotter.h"
#include "CV_Plotter.h"
#include "FTIR_Analyzer.h"
#include "Lifetime_Plotter.h"
#include "Report_Plots.h"
//#include "cppad/ipopt/solve.hpp"

#include "main.h"

void Add_Mouse_Position_Label( QCustomPlot* graph, QLabel* statusLabel )
{
	statusLabel->setText( "Status Label" );
	QObject::connect( graph, &QCustomPlot::mouseMove, [ statusLabel, graph ]( QMouseEvent *event )
	{
		double x = graph->xAxis->pixelToCoord( event->pos().x() );
		double y = graph->yAxis->pixelToCoord( event->pos().y() );

		statusLabel->setText( QString( "%1 , %2" ).arg( x ).arg( y ) );
	} );
}

void Add_Mouse_Position_And_Point_Label( QCustomPlot* graph, QLabel* statusLabel )
{
	statusLabel->setText( "Status Label" );
	QObject::connect( graph, &QCustomPlot::mouseMove, [ statusLabel, graph ]( QMouseEvent *event )
	{
		double x = graph->xAxis->pixelToCoord( event->pos().x() );
		double y = graph->yAxis->pixelToCoord( event->pos().y() );

		statusLabel->setText( QString( "%1 , %2" ).arg( x ).arg( y ) );
	} );
}

Main_Widget::Main_Widget( QWidget *parent ) : QMainWindow( parent )
{
	ui.setupUi( this );

	QLabel* statusLabel = new QLabel( this );
	ui.statusBar->addPermanentWidget( statusLabel );
	Prepare_New_Tab( statusLabel );
}

void Main_Widget::Prepare_New_Tab( QLabel* statusLabel )
{
	connect( this->ui.main_tabWidget, &QTabWidget::tabCloseRequested, this->ui.main_tabWidget, &QTabWidget::removeTab );
	auto prepare_new_tab = [ this, statusLabel ]( auto* new_tab, QString title )
	{
		int index = ui.main_tabWidget->count() - 1;
		ui.main_tabWidget->insertTab( index, new_tab, title );
		ui.main_tabWidget->setCurrentIndex( index );
		Add_Mouse_Position_Label( new_tab->ui.customPlot, statusLabel );
	};
	auto prepare_new_tab2 = [ this, statusLabel ]( auto* new_tab, QString title )
	{
		int index = ui.main_tabWidget->count() - 1;
		ui.main_tabWidget->insertTab( index, new_tab, title );
		ui.main_tabWidget->setCurrentIndex( index );
	};
	connect( ui.startFTIR_pushButton,     &QPushButton::clicked, [ this, prepare_new_tab ]( bool checked ) { prepare_new_tab( new FTIR::FTIR_Analyzer( this ), "&FTIR" ); } );
	connect( ui.startIV_pushButton,       &QPushButton::clicked, [ this, prepare_new_tab ]( bool checked ) { prepare_new_tab( new IV::Plotter( this ), "&IV" ); } );
	connect( ui.startLifetime_pushButton, &QPushButton::clicked, [ this, prepare_new_tab ]( bool checked ) { prepare_new_tab( new Lifetime::Plotter( this ), "&Lifetime" ); } );
	connect( ui.startReport_pushButton,   &QPushButton::clicked, [ this, prepare_new_tab2 ]( bool checked ) { prepare_new_tab2( new Report::Report_Plots( this ), "&Report" ); } );
	connect( ui.startIVBySize_pushButton, &QPushButton::clicked, [ this, prepare_new_tab ]( bool checked ) { prepare_new_tab( new IV_By_Size::Plotter( this ), "IV By &Size" ); } );
	connect( ui.startCV_pushButton,       &QPushButton::clicked, [ this, prepare_new_tab ]( bool checked ) { prepare_new_tab( new CV::Plotter( this ), "&CV" ); } );

	ui.main_tabWidget->tabBar()->tabButton( 0, QTabBar::RightSide )->resize( 0, 0 );
}

#include "Optimize.h"
bool get_started();
int main(int argc, char *argv[])
{
	if constexpr( false )
	{
		get_started();
	}
	if constexpr( false )
	{
		auto func = []( const arma::vec & fit_params, const arma::vec & x ) -> arma::vec
		{
			return fit_params - x;
			//return ( 1 - x[ 0 ] ) * ( 1 - x[ 1 ] ) * ( 1 - x[ 2 ] );
		};
		const arma::vec x_data = { 1,2,3 };
		const arma::vec y_data = { 0,0,0 };
		const arma::vec lower_bounds = { -10,-10,-10 };
		const arma::vec upper_bounds = { +10,+10,+10 };

		arma::vec best_fit_params = Fit_Data_To_Function( func, x_data, y_data, lower_bounds, upper_bounds,
			1E-5, 1000, 1E-2 );
		std::cout << "Test fit should be 1,2,3: " << best_fit_params[ 0 ] << " " << best_fit_params[ 1 ] << " " << best_fit_params[ 2 ] << "\n";
	}

	if constexpr( false )
	{
		QApplication a( argc, argv );

		SQL_Configuration config;
		//config.header_titles = QStringList{ "Sample Name", "Date", "Temperature (K)", "Dewar Temp (C)", "Time of Day", "Gain", "Bias (V)", "measurement_id" };
		//config.what_to_collect = QStringList{ "sample_name", "date(time)", "temperature_in_k", "dewar_temp_in_c", "time(time)", "gain", "bias_in_v", "measurement_id" }; // DATE_FORMAT(time, '%b %e %Y') DATE_FORMAT(time, '%H:%i:%s')
		config.header_titles = QStringList{ "Sample Name", "Temperature (K)", "Dewar Temp (C)", "Gain", "Bias (V)", "measurement_id" };
		config.what_to_collect = QStringList{ "sample_name", "temperature_in_k", "dewar_temp_in_c", "gain", "bias_in_v", "measurement_id" }; // DATE_FORMAT(time, '%b %e %Y') DATE_FORMAT(time, '%H:%i:%s')
		config.sql_table = "ftir_measurements";
		config.raw_data_columns = QStringList{ "measurement_id","wavenumber","intensity" };
		config.raw_data_table = "ftir_raw_data";
		config.sorting_strategy = "ORDER BY measurement_id, wavenumber ASC";
		config.columns_to_show = 7;

		QString config_filename = "configuration.ini";
		SQL_Manager test( nullptr, config_filename, "Test", "Local_SQL_Cache" );
		//ID_To_Metadata data;
		//data[ "99998" ] = { 1, 2, 3, 4, 5, 6, 7, 99998 };
		//data[ "99999" ] = { 1, 2, 3, 4, 5, 6, 7, 99999 };
		//data[ "99998" ] = { 1, 2, 3, 4, 5, 99998 };
		//data[ "99990" ] = { 1, 2, 3, 4, 5, 99990 };
		ID_To_XY_Data data;
		data[ "99998" ] = { {1}, {2} };
		data[ "99990" ] = { {2, 3, 4}, {3, 4, 5} };
		test.Start_Thread();
		//test.Write_SQL_Metadata( data, config.what_to_collect, config.sql_table );
		test.Write_SQL_XY_Data( data, config.raw_data_columns, config.raw_data_table );
		return a.exec();
	}

	QApplication a(argc, argv);
	Main_Widget w;
	//Ui::Main ui;
	//ui.setupUi( &w );
	//QLabel* statusLabel = new QLabel( &w );
	//ui.statusBar->addPermanentWidget( statusLabel );
	//Prepare_New_Tab( &w, ui, statusLabel );
	//Add_Mouse_Position_Label( ui.ftir_tab->ui.customPlot, statusLabel );
	//Add_Mouse_Position_Label( ui.iv_tab->ui.customPlot, statusLabel );
	//Add_Mouse_Position_Label( ui.ivBySize_tab->ui.customPlot, statusLabel );
	//Add_Mouse_Position_Label( ui.cv_tab->ui.customPlot, statusLabel );
	w.show();
	return a.exec();
}


//#include <armadillo>
//#include "Thin_Film_Interference.h"
//int main2()
//{
//	auto test_func = []( const arma::vec & x )
//	{
//		double x_part = x( 0 ) - 4;
//		double y_part = x( 1 ) - 2;
//		double z_part = x( 2 ) - 1;
//		return x_part * x_part * x_part * x_part + y_part * y_part * y_part * y_part + z_part * z_part * z_part * z_part
//			+ x_part * x_part * y_part * y_part;
//	};
//
//	//arma::vec solution_for_test_func = Minimize_Function_Starting_Point( test_func, arma::zeros<arma::vec>( 3 ), 100, 1.0, 1E-3 );
//
//
//	std::ofstream testing_file( "Why you no work.txt" );
//	std::cout.precision( 17 );
//	testing_file.precision( 17 );
//
//	Thin_Film_Interference test;
//	arma::vec wavelengths = arma::linspace( 5E-6, 20E-6, 1001 );
//	//std::vector<double> Get_Expected_Transmission( double temperature_k, const std::vector<Material_Layer> & layers, const arma::vec & wavelengths );
//
//	std::vector<Material_Layer> layers{ {Material::Si, 0.5, 1E-6}, {Material::CdTe, 0.5, 5E-6}, {Material::HgCdTe, 0.5, 1E-6} };
//	arma::vec data_results = test.Get_Expected_Transmission( 300.0, layers, wavelengths );
//	for( double x : data_results )
//		testing_file << x << "\t";
//	testing_file << "\n";
//	//arma::vec data_results2 = test.Get_Expected_Transmission( 300.0, layers, wavelengths );
//	arma::cx_vec data_results2 = arma::fft( data_results );
//	testing_file << "Fourier transform\n";
//	for( arma::cx_double x : data_results2 )
//		testing_file << std::real( x ) << "\t";
//	testing_file << "\n";
//	for( arma::cx_double x : data_results2 )
//		testing_file << std::imag( x ) << "\t";
//	testing_file << "\n";
//
//	//auto minimize_function = [&test, &layers, &wavelengths, &data_results]( const column_vector& input_to_optimize )
//	auto minimize_function = [&test, layers, &wavelengths, &data_results]( const arma::vec& input_to_optimize, arma::vec* grad_out, void* opt_data )
//	{
//		std::vector<Material_Layer> copy_layers = layers;
//		//layers[ 0 ].thickness = layer1;
//		//layers[ 1 ].thickness = layer2;
//		//layers[ 2 ].thickness = layer3;
//		bool invalid_input = false;
//		for( int i = 0; i < input_to_optimize.size(); i++ )
//		{
//			copy_layers[ i ].thickness = input_to_optimize( i );
//			if( input_to_optimize( i ) < 0 )
//				invalid_input = true;
//		}
//		if( invalid_input )
//			return 9999999999.;
//		arma::vec results = test.Get_Expected_Transmission( 300.0, copy_layers, wavelengths );
//		arma::vec difference = results - data_results;
//		double error = arma::dot( difference, difference );
//		//std::cout << input_to_optimize << std::endl;
//		//std::cout << layer1 << " " << layer2 << " " << layer3 << std::endl;
//		//std::cout << error << std::endl;
//		return error;
//	};
//	auto minimize_function2 = [&test, layers, &wavelengths, &data_results, &testing_file]( const arma::vec& input_to_optimize )
//	{
//		std::vector<Material_Layer> copy_layers = layers;
//		//layers[ 0 ].thickness = layer1;
//		//layers[ 1 ].thickness = layer2;
//		//layers[ 2 ].thickness = layer3;
//		bool invalid_input = false;
//		for( int i = 0; i < input_to_optimize.size(); i++ )
//		{
//			copy_layers[ i ].thickness = input_to_optimize( i );
//			if( input_to_optimize( i ) < 0 )
//				invalid_input = true;
//		}
//		if( invalid_input )
//			return 9999999999.;
//		arma::vec results = test.Get_Expected_Transmission( 300.0, copy_layers, wavelengths );
//		testing_file << "Other Data\n";
//		for( double x : results )
//			testing_file << x << "\t";
//		testing_file << "\n";
//		arma::vec difference = results - data_results;
//		testing_file << "Difference\n";
//		for( double x : difference )
//			testing_file << x << "\t";
//		testing_file << "\n";
//
//		double sum_of_squares = 0;
//		for( double x : difference )
//			sum_of_squares += x * x;
//
//		double error = arma::dot( difference, difference );
//		//std::cout << input_to_optimize << std::endl;
//		//std::cout << error << ": " << input_to_optimize( 0 ) << " " << input_to_optimize( 1 ) << " " << input_to_optimize( 2 ) << std::endl;
//		//std::cout << error << ": " << input_to_optimize( 0 ) << " " << input_to_optimize( 1 ) << std::endl;
//		//std::cout << error << std::endl;
//		return error;
//	};
//	arma::vec testpoint3 = { 1E-6, 5E-6 };
//	double result_again = minimize_function2( testpoint3 );
//	for( double x : data_results )
//		testing_file << x << "\t";
//	testing_file << "\n";
//	arma::vec data_results3 = test.Get_Expected_Transmission( 300.0, layers, wavelengths );
//	for( double x : data_results3 )
//		testing_file << x << "\t";
//	testing_file << "\n";
//
//	arma::vec testpoint1 = { 7.77993e-07, 5.19409e-06, 5.2127e-07 };
//	double result1 = minimize_function2( testpoint1 );
//	testing_file << "Higher: " << result1 << " - " << testpoint1( 0 ) << ", " << testpoint1( 1 ) << ", " << testpoint1( 2 ) << "\n";
//	arma::vec testpoint2 = { 9.88584e-07 - 1E-9, 4.97057e-06, 9.7873e-07 };
//	double result2 = minimize_function2( testpoint2 );
//	testing_file << "Lower: " << result2 << " - " << testpoint2( 0 ) << ", " << testpoint2( 1 ) << ", " << testpoint2( 2 ) << "\n";
//	double result3 = minimize_function2( testpoint3 );
//	testing_file << "Higher: " << result3 << " - " << testpoint3( 0 ) << ", " << testpoint3( 1 ) << "\n";
//	testing_file.flush();
//	//arma::vec starting_point = { 7.77993e-07, 5.19409e-06 };// , 5.2127e-07 };
//	//arma::vec starting_point = { 1.1E-6, 5.1E-06, 1.1E-6 };// , 5.2127e-07 };
//	arma::vec starting_point = { 1.1E-6, 5.5E-06, 1.0E-6 };// , 5.2127e-07 };
//	arma::vec solution = Minimize_Function_Starting_Point( minimize_function2, starting_point, 1000, 1.0, 1E-10 );
//
//	arma::vec testpoint4 = { 9.8853623499091619e-07, 5.0059360929261567e-06, 9.9480259067058758e-07 };
//	testing_file << "Use this data:\n";
//	arma::vec offset_5 = { 0, 1E-9, 0 };
//	double result4 = minimize_function2( testpoint4 );
//	double result5 = minimize_function2( testpoint4 + offset_5 );
//	double result6 = minimize_function2( testpoint4 - offset_5 );
//	testing_file.flush();
//	std::cout << "Another Point: " << result4 << " : " << result5 << " : " << result6 << " : " << testpoint4( 0 ) << ", " << testpoint4( 1 ) << ", " << testpoint4( 2 ) << "\n";
//	arma::vec gradient_yup = Gradient_Approximate( testpoint4, minimize_function2, 1E-9 );
//	std::cout << "Another Gradient: " << gradient_yup( 0 ) << ", " << gradient_yup( 1 ) << ", " << gradient_yup( 2 ) << "\n";
//
//#if 0
//	std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
//	//auto result = dlib::find_min_global( minimize_function,
//	//									 //{ 0.5E-6, 2.5E-6, 0.5E-6 }, // lower bounds
//	//									 //{ 2E-6, 10E-6, 2E-6 }, // upper bounds
//	//									 { 100E-9, 100E-9, 100E-9 }, // lower bounds
//	//									 { 20E-6, 20E-6, 20E-6 }, // upper bounds
//	//									 dlib::max_function_calls( 3000 ) );
//	//column_vector starting_point = { 100E-9, 100E-9, 100E-9 };
//	column_vector starting_point = { 1.1E-6, 5.1E-6, 1.1E-6 };
//	dlib::find_min_using_approximate_derivatives( dlib::bfgs_search_strategy(),
//												  dlib::objective_delta_stop_strategy( 1e-7 ),
//												  minimize_function, starting_point, -1 );
//	std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
//	std::chrono::duration<double> elapsed_seconds = end - start;
//	std::cout << "elapsed time: " << elapsed_seconds.count() << "s\n";
//	std::cout << "result:\n" << starting_point << "\n";
//#endif
//
//	if( 0 )
//	{
//		arma::vec out_range_x = arma::linspace( 0.5E-6, 1.1E-6, 3 );
//		arma::vec out_range_y = arma::linspace( 4.5E-6, 5.1E-6, 3 );
//		arma::vec out_range_z = arma::linspace( 0.5E-6, 1.1E-6, 3 );
//		for( double z : out_range_z )
//		{
//			for( double y : out_range_y )
//			{
//				for( double x : out_range_x )
//				{
//					arma::vec values_to_try{ x, y, z };
//					arma::vec result = Minimize_Function_Starting_Point( minimize_function2, values_to_try );
//					std::cout << values_to_try( 0 ) << ", " << values_to_try( 1 ) << ", " << values_to_try( 2 )
//						<< " : " << result( 0 ) << ", " << result( 1 ) << ", " << result( 2 ) << "\n";
//
//				}
//			}
//		}
//	}
//
//	if( 0 )
//	{
//		arma::vec startpoint = { 1E-6, 5e-06, 1.0E-6 };
//		//arma::vec startpoint = { 9.8853623499091619e-07, 5.0059360929261567e-06, 9.9480259067058758e-07 };
//		arma::vec out_range_x = arma::linspace( startpoint( 0 ) - 1000e-9, startpoint( 0 ) + 1000e-9, 201 );
//		arma::vec out_range_y = arma::linspace( startpoint( 1 ) - 1000e-9, startpoint( 1 ) + 1000e-9, 201 );
//		arma::vec out_range_z = arma::linspace( startpoint( 1 ) - 100e-9, startpoint( 1 ) + 100e-9, 21 );
//		//arma::vec out_range_z = { startpoint( 2 ) };
//		//arma::vec out_range_x = arma::linspace( 0.5E-6, 1.5E-6, 101 );
//		//arma::vec out_range_y = arma::linspace( 0.5E-6, 1.5E-6, 101 );
//		////arma::vec out_range_y = { 1.1E-6 };
//		////arma::vec out_range_z = arma::linspace( 4.5E-6, 5.5E-6, 101 );
//		//arma::vec out_range_z = { 5.1E-6 };
//		//arma::vec out_range_y = arma::linspace( 4.5E-6, 5.5E-6, 101 );
//		std::ofstream test_out( "Test.csv" );
//		for( double x : out_range_x )
//		{
//			test_out << x << "\t";
//		}
//		test_out << std::endl;
//		for( double x : out_range_y )
//		{
//			test_out << x << "\t";
//		}
//		test_out << std::endl;
//
//		//int i = 0;
//		for( double z : out_range_z )
//		{
//			//std::ofstream test_out( "Test" + std::to_string(i++) +  ".csv" );
//
//			for( double y : out_range_y )
//			{
//				for( double x : out_range_x )
//				{
//					//arma::vec values_to_try{ x, y, 4.49048e-07 };
//					//arma::vec values_to_try{ x, 5.328E-6, y };
//					arma::vec values_to_try{ x, y, z };
//					//column_vector values_to_try{ x, y, 1.2168e-06 };
//					test_out << minimize_function( values_to_try, nullptr, nullptr ) << "\t";
//					//test_out << minimize_function( values_to_try ) << "\t";
//				}
//				test_out << std::endl;
//			}
//		}
//	}
//
//	while( 1 );
//	return 0;
//}

# include <cppad/ipopt/solve.hpp>

namespace {
	using CppAD::AD;

	class FG_eval {
	public:
		typedef CPPAD_TESTVECTOR( AD<double> ) ADvector;
		void operator()( ADvector& fg, const ADvector& x )
		{
			assert( fg.size() == 3 );
			assert( x.size() == 4 );

			// Fortran style indexing
			AD<double> x1 = x[ 0 ];
			AD<double> x2 = x[ 1 ];
			AD<double> x3 = x[ 2 ];
			AD<double> x4 = x[ 3 ];
			// f(x)
			fg[ 0 ] = x1 * x4 * (x1 + x2 + x3) + x3;
			// g_1 (x)
			fg[ 1 ] = x1 * x2 * x3 * x4;
			// g_2 (x)
			fg[ 2 ] = x1 * x1 + x2 * x2 + x3 * x3 + x4 * x4;
			//
			return;
		}
	};
}

bool get_started( void )
{
	size_t i;
	using Dvector = arma::vec;

	// number of independent variables (domain dimension for f and g)
	size_t nx = 4;
	// number of constraints (range dimension for g)
	size_t ng = 2;
	// initial value of the independent variables
	Dvector xi = { 1.0, 5.0, 5.0, 1.0 };
	Dvector xl = { 1.0, 1.0, 1.0, 1.0 };
	Dvector xu = { 5.0, 5.0, 5.0, 5.0 };
	// lower and upper limits for g
	Dvector gl = { 25.0, 40.0 };
	Dvector gu = { 1.0e19, 40.0 };

	// object that computes objective and constraints
	FG_eval fg_eval;

	// options
	std::string options;
	// turn off any printing
	options += "Integer print_level  0\n";
	options += "String  sb           yes\n";
	// maximum number of iterations
	options += "Integer max_iter     10\n";
	// approximate accuracy in first order necessary conditions;
	// see Mathematical Programming, Volume 106, Number 1,
	// Pages 25-57, Equation (6)
	options += "Numeric tol          1e-6\n";
	// derivative testing
	options += "String  derivative_test            second-order\n";
	// maximum amount of random pertubation; e.g.,
	// when evaluation finite diff
	options += "Numeric point_perturbation_radius  0.\n";

	// place to return solution
	CppAD::ipopt::solve_result<Dvector> solution;

	// solve the problem
	CppAD::ipopt::solve<Dvector, FG_eval>(
		options, xi, xl, xu, gl, gu, fg_eval, solution
		);
	//
	// Check some of the solution values
	//
	bool ok = solution.status == CppAD::ipopt::solve_result<Dvector>::success;
	//
	double check_x[] = { 1.000000, 4.743000, 3.82115, 1.379408 };
	double check_zl[] = { 1.087871, 0.,       0.,      0. };
	double check_zu[] = { 0.,       0.,       0.,      0. };
	double rel_tol = 1e-6;  // relative tolerance
	double abs_tol = 1e-6;  // absolute tolerance
	for( i = 0; i < nx; i++ )
	{
		ok &= CppAD::NearEqual(
			check_x[ i ], solution.x[ i ], rel_tol, abs_tol
		);
		ok &= CppAD::NearEqual(
			check_zl[ i ], solution.zl[ i ], rel_tol, abs_tol
		);
		ok &= CppAD::NearEqual(
			check_zu[ i ], solution.zu[ i ], rel_tol, abs_tol
		);
	}

	return ok;
}