#pragma once
#define GLOG_NO_ABBREVIATED_SEVERITIES
#include "ceres/ceres.h"
#include "glog/logging.h"
#include <armadillo>

int ceres_main( int argc, char* argv[] );

struct DoubleExponentialResidual {
	DoubleExponentialResidual( double x, double y ) : time( x ), y_( y ) {}
	template <typename T>
	bool operator()( const T* const params, T* residual ) const {
		auto A = params[ 0 ];
		auto offset1 = params[ 1 ];
		auto tau1 = params[ 2 ];
		auto B = params[ 3 ];
		auto offset2 = params[ 4 ];
		auto tau2 = params[ 5 ];

		auto x1 = -(time - offset1) / tau1;
		auto x2 = -(time - offset2) / tau2;
		residual[ 0 ] = y_ - (A * exp( x1 ) + B * exp( x2 ));
		//residual[ 0 ] *= residual[ 0 ];
		return true;
	}
private:
	const double time;
	const double y_;
};


template< typename Single_Graph, typename Fit_Results >
std::array<Fit_Results, 2> Ceres_Fit_Lifetime( const arma::vec& initial_guess, const arma::vec& lower_limits, const arma::vec& upper_limits, const Single_Graph& graph )
{
	if( graph.x_data.empty() || graph.y_data.empty() || (graph.x_data.size() != graph.y_data.size()) )
		return { Fit_Results{ arma::datum::nan, arma::datum::nan, arma::datum::nan }, Fit_Results{ arma::datum::nan, arma::datum::nan, arma::datum::nan } };
	arma::vec x = arma::conv_to<arma::vec>::from( graph.x_data.toStdVector() );
	arma::vec y = arma::conv_to<arma::vec>::from( graph.y_data.toStdVector() );

	double x_offset = x( y.index_max() ); // Align all of the peaks
	x = x - x_offset;
	y = y - y.min() + 1E-9;
	arma::uvec selection_region = arma::find( x > graph.lower_x_fit && x < graph.upper_x_fit2 );
	x = x( selection_region );
	y = y( selection_region );
	using ceres::AutoDiffCostFunction;
	using ceres::CostFunction;
	using ceres::Problem;
	using ceres::Solve;
	using ceres::Solver;
	arma::vec fit_vars = initial_guess;
	Problem problem;
	for( int i = 0; i < selection_region.size(); ++i ) {
		problem.AddResidualBlock(
			new AutoDiffCostFunction<DoubleExponentialResidual, 1, 6>(
				new DoubleExponentialResidual( x[ i ], y[ i ] ) ),
			nullptr,
			fit_vars.memptr() );
	}
	for( int i = 0; i < lower_limits.size(); i++ )
	{
		problem.SetParameterLowerBound( fit_vars.memptr(), i, lower_limits[ i ] );
		problem.SetParameterUpperBound( fit_vars.memptr(), i, upper_limits[ i ] );
	}
	Solver::Options options;
	options.max_num_iterations = 100;
	options.linear_solver_type = ceres::DENSE_QR;
	//options.minimizer_progress_to_stdout = true;
	//options.parameter_tolerance = 1E-12;
	Solver::Summary summary;
	Solve( options, &problem, &summary );
	//std::cout << summary.FullReport() << "\n";
	//std::cout << "Initial m: " << initial_guess << "\n";
	//std::cout << "Final   m: " << copy << "\n";
	//return { Fit_Results{fit_vars[ 0 ], fit_vars[ 1 ], fit_vars[ 2 ]}, Fit_Results{fit_vars[ 3 ], fit_vars[ 4 ], fit_vars[ 5 ]}};
	if( fit_vars[ 2 ] < fit_vars[ 5 ] )
		return { Fit_Results{ (fit_vars[ 0 ]), fit_vars[ 1 ], fit_vars[ 2 ] }, Fit_Results{ (fit_vars[ 3 ]), fit_vars[ 4 ], fit_vars[ 5 ] } };
	else
		return { Fit_Results{ (fit_vars[ 3 ]), fit_vars[ 4 ], fit_vars[ 5 ] }, Fit_Results{ (fit_vars[ 0 ]), fit_vars[ 1 ], fit_vars[ 2 ] } };
}


#include "Thin_Film_Interference.h"
struct ThinFilmResidual {
	ThinFilmResidual( const std::vector<Material_Layer> & layers, const Material_Layer & backside_mat,
		double x, double y ) :
		copy_layers( layers ), backside_material( backside_mat ), wavelength( x ), transmission( y )
	{
		fit_parameters = Get_Things_To_Fit( copy_layers );
		fit_parameters.push_back( &height_scale );
	}
	//template <typename T>
	//bool operator()( const T* const params, T* residual ) const {
	bool operator()( const double* const input_to_optimize, double* residual ) const {
		for( int i = 0; std::optional< double >* parameter : fit_parameters )
			*parameter = input_to_optimize[ i++ ];

		Result_Data results = Get_Expected_Transmission( copy_layers, { wavelength }, backside_material);
		residual[ 0 ] = results.transmission[ 0 ] * height_scale.value() - transmission;

		return true;
	}
private:
	std::optional<double> height_scale;
	std::vector<std::optional<double>*> fit_parameters;
	std::vector<Material_Layer> copy_layers;
	const Material_Layer backside_material;
	const double wavelength;
	const double transmission;
};

struct ThinFilmAllResiduals {
	ThinFilmAllResiduals( const std::vector<Material_Layer> & layers, const Material_Layer & backside_mat,
		const arma::vec & x, const arma::vec & y ) :
		copy_layers( layers ), backside_material( backside_mat ), wavelengths( x ), transmissions( y )
	{
		fit_parameters = Get_Things_To_Fit( copy_layers );
		fit_parameters.push_back( &height_scale );
	}
	//template <typename T>
	//bool operator()( const T* const input_to_optimize, T* residuals ) const {
	bool operator()( const double* const input_to_optimize, double* residuals ) const {
		for( int i = 0; std::optional< double >* parameter : fit_parameters )
			*parameter = input_to_optimize[i++];

		Result_Data results = Get_Expected_Transmission( copy_layers, wavelengths, backside_material);
		arma::vec residual_vec = results.transmission * height_scale.value() - transmissions;
		for( int i = 0; double residual : residual_vec )
			residuals[ i++ ] = residual;

		return true;
	}
private:
	std::optional<double> height_scale;
	std::vector<std::optional<double>*> fit_parameters;
	std::vector<Material_Layer> copy_layers;
	const Material_Layer backside_material;
	const arma::vec & wavelengths;
	const arma::vec & transmissions;
};


inline ceres::ResidualBlockId Dynamic_Fit_Parameters(
	ceres::Problem& problem, std::vector<Material_Layer>& layers, const Material_Layer& backside_material,
	const arma::vec & wavelengths, const arma::vec & transmissions, arma::vec& fit_vars )
{
	switch( fit_vars.size() )
	{
	case 1: return problem.AddResidualBlock(
		new ceres::NumericDiffCostFunction<ThinFilmAllResiduals, ceres::CENTRAL, ceres::DYNAMIC, 1>(
			new ThinFilmAllResiduals( layers, backside_material, wavelengths, transmissions ),
				ceres::TAKE_OWNERSHIP, transmissions.size() ),
		nullptr, fit_vars.memptr() );
	case 2: return problem.AddResidualBlock(
		new ceres::NumericDiffCostFunction<ThinFilmAllResiduals, ceres::CENTRAL, ceres::DYNAMIC, 2>(
			new ThinFilmAllResiduals( layers, backside_material, wavelengths, transmissions ),
			ceres::TAKE_OWNERSHIP, transmissions.size() ),
		nullptr, fit_vars.memptr() );
	case 3: return problem.AddResidualBlock(
		new ceres::NumericDiffCostFunction<ThinFilmAllResiduals, ceres::CENTRAL, ceres::DYNAMIC, 3>(
			new ThinFilmAllResiduals( layers, backside_material, wavelengths, transmissions ),
			ceres::TAKE_OWNERSHIP, transmissions.size() ),
		nullptr, fit_vars.memptr() );
	case 4: return problem.AddResidualBlock(
		new ceres::NumericDiffCostFunction<ThinFilmAllResiduals, ceres::CENTRAL, ceres::DYNAMIC, 4>(
			new ThinFilmAllResiduals( layers, backside_material, wavelengths, transmissions ),
			ceres::TAKE_OWNERSHIP, transmissions.size() ),
		nullptr, fit_vars.memptr() );
	case 5: return problem.AddResidualBlock(
		new ceres::NumericDiffCostFunction<ThinFilmAllResiduals, ceres::CENTRAL, ceres::DYNAMIC, 5>(
			new ThinFilmAllResiduals( layers, backside_material, wavelengths, transmissions ),
			ceres::TAKE_OWNERSHIP, transmissions.size() ),
		nullptr, fit_vars.memptr() );
	case 6: return problem.AddResidualBlock(
		new ceres::NumericDiffCostFunction<ThinFilmAllResiduals, ceres::CENTRAL, ceres::DYNAMIC, 6>(
			new ThinFilmAllResiduals( layers, backside_material, wavelengths, transmissions ),
			ceres::TAKE_OWNERSHIP, transmissions.size() ),
		nullptr, fit_vars.memptr() );
	case 7: return problem.AddResidualBlock(
		new ceres::NumericDiffCostFunction<ThinFilmAllResiduals, ceres::CENTRAL, ceres::DYNAMIC, 7>(
			new ThinFilmAllResiduals( layers, backside_material, wavelengths, transmissions ),
			ceres::TAKE_OWNERSHIP, transmissions.size() ),
		nullptr, fit_vars.memptr() );
	case 8: return problem.AddResidualBlock(
		new ceres::NumericDiffCostFunction<ThinFilmAllResiduals, ceres::CENTRAL, ceres::DYNAMIC, 8>(
			new ThinFilmAllResiduals( layers, backside_material, wavelengths, transmissions ),
			ceres::TAKE_OWNERSHIP, transmissions.size() ),
		nullptr, fit_vars.memptr() );
	case 9: return problem.AddResidualBlock(
		new ceres::NumericDiffCostFunction<ThinFilmAllResiduals, ceres::CENTRAL, ceres::DYNAMIC, 9>(
			new ThinFilmAllResiduals( layers, backside_material, wavelengths, transmissions ),
			ceres::TAKE_OWNERSHIP, transmissions.size() ),
		nullptr, fit_vars.memptr() );
	case 10: return problem.AddResidualBlock(
		new ceres::NumericDiffCostFunction<ThinFilmAllResiduals, ceres::CENTRAL, ceres::DYNAMIC, 10>(
			new ThinFilmAllResiduals( layers, backside_material, wavelengths, transmissions ),
			ceres::TAKE_OWNERSHIP, transmissions.size() ),
		nullptr, fit_vars.memptr() );
	case 11: return problem.AddResidualBlock(
		new ceres::NumericDiffCostFunction<ThinFilmAllResiduals, ceres::CENTRAL, ceres::DYNAMIC, 11>(
			new ThinFilmAllResiduals( layers, backside_material, wavelengths, transmissions ),
			ceres::TAKE_OWNERSHIP, transmissions.size() ),
		nullptr, fit_vars.memptr() );
	case 12: return problem.AddResidualBlock(
		new ceres::NumericDiffCostFunction<ThinFilmAllResiduals, ceres::CENTRAL, ceres::DYNAMIC, 12>(
			new ThinFilmAllResiduals( layers, backside_material, wavelengths, transmissions ),
			ceres::TAKE_OWNERSHIP, transmissions.size() ),
		nullptr, fit_vars.memptr() );
	case 13: return problem.AddResidualBlock(
		new ceres::NumericDiffCostFunction<ThinFilmAllResiduals, ceres::CENTRAL, ceres::DYNAMIC, 13>(
			new ThinFilmAllResiduals( layers, backside_material, wavelengths, transmissions ),
			ceres::TAKE_OWNERSHIP, transmissions.size() ),
		nullptr, fit_vars.memptr() );
	case 14: return problem.AddResidualBlock(
		new ceres::NumericDiffCostFunction<ThinFilmAllResiduals, ceres::CENTRAL, ceres::DYNAMIC, 14>(
			new ThinFilmAllResiduals( layers, backside_material, wavelengths, transmissions ),
			ceres::TAKE_OWNERSHIP, transmissions.size() ),
		nullptr, fit_vars.memptr() );
	case 15: return problem.AddResidualBlock(
		new ceres::NumericDiffCostFunction<ThinFilmAllResiduals, ceres::CENTRAL, ceres::DYNAMIC, 15>(
			new ThinFilmAllResiduals( layers, backside_material, wavelengths, transmissions ),
			ceres::TAKE_OWNERSHIP, transmissions.size() ),
		nullptr, fit_vars.memptr() );
	case 16: return problem.AddResidualBlock(
		new ceres::NumericDiffCostFunction<ThinFilmAllResiduals, ceres::CENTRAL, ceres::DYNAMIC, 16>(
			new ThinFilmAllResiduals( layers, backside_material, wavelengths, transmissions ),
			ceres::TAKE_OWNERSHIP, transmissions.size() ),
		nullptr, fit_vars.memptr() );
	case 17: return problem.AddResidualBlock(
		new ceres::NumericDiffCostFunction<ThinFilmAllResiduals, ceres::CENTRAL, ceres::DYNAMIC, 17>(
			new ThinFilmAllResiduals( layers, backside_material, wavelengths, transmissions ),
			ceres::TAKE_OWNERSHIP, transmissions.size() ),
		nullptr, fit_vars.memptr() );
	case 18: return problem.AddResidualBlock(
		new ceres::NumericDiffCostFunction<ThinFilmAllResiduals, ceres::CENTRAL, ceres::DYNAMIC, 18>(
			new ThinFilmAllResiduals( layers, backside_material, wavelengths, transmissions ),
			ceres::TAKE_OWNERSHIP, transmissions.size() ),
		nullptr, fit_vars.memptr() );
	case 19: return problem.AddResidualBlock(
		new ceres::NumericDiffCostFunction<ThinFilmAllResiduals, ceres::CENTRAL, ceres::DYNAMIC, 19>(
			new ThinFilmAllResiduals( layers, backside_material, wavelengths, transmissions ),
			ceres::TAKE_OWNERSHIP, transmissions.size() ),
		nullptr, fit_vars.memptr() );
	case 20: return problem.AddResidualBlock(
		new ceres::NumericDiffCostFunction<ThinFilmAllResiduals, ceres::CENTRAL, ceres::DYNAMIC, 20>(
			new ThinFilmAllResiduals( layers, backside_material, wavelengths, transmissions ),
			ceres::TAKE_OWNERSHIP, transmissions.size() ),
		nullptr, fit_vars.memptr() );
	default: return nullptr;
	}
}

#include "Thin_Film_Interference.h"
#include "numeric_diff_dynamic_cost_function.h"
inline arma::vec Ceres_Thin_Film_Fit(
	const std::vector<Material_Layer>& layers,
	const arma::vec& wavelengths, // in meters
	const arma::vec& transmissions, // Scaled to between 0.0 and 1.0
	Material_Layer backside_material )
{
	std::vector<Material_Layer> copy_layers = layers;
	auto fit_parameters = Get_Things_To_Fit( copy_layers );
	//if( fit_parameters.empty() )
	//	return {};
	arma::vec fit_vars = arma::vec( fit_parameters.size() + 1 );
	for( int i = 0; std::optional< double >*parameter : fit_parameters )
		fit_vars[ i++ ] = parameter->value();
	fit_vars.back() = 1.0;
	arma::vec initial_guess = fit_vars;
	ceres::Problem problem;
	//for( int i = 0; i < wavelengths.size(); ++i ) {
	//	Dynamic_Fit_Parameters( problem, copy_layers, backside_material,
	//		wavelengths[ i ], transmissions[ i ], fit_vars );
	//}
	//double* parameter_groups[ 1 ] = { fit_vars.memptr() };
	//problem.AddResidualBlock(
	//	new ceres::NumericDiffDynamicCostFunction<ThinFilmAllResiduals, ceres::CENTRAL, ceres::DYNAMIC>(
	//		new ThinFilmAllResiduals( layers,
	//			backside_material,
	//			wavelengths,
	//			transmissions ), fit_vars.size(), ceres::TAKE_OWNERSHIP, transmissions.size() ),
	//	nullptr, fit_vars.memptr() );
	Dynamic_Fit_Parameters( problem, copy_layers, backside_material,
				wavelengths, transmissions, fit_vars );
	//problem.AddParameterBlock( fit_vars.memptr(), fit_vars.size() );
	for( int i = 0; i < fit_vars.size(); i++ )
	{
		problem.SetParameterLowerBound( fit_vars.memptr(), i, fit_vars[ i ] * 0.5 );
		problem.SetParameterUpperBound( fit_vars.memptr(), i, fit_vars[ i ] * 1.5 );
	}
	ceres::Solver::Options options;
	options.max_num_iterations = 100;
	options.linear_solver_type = ceres::DENSE_QR;
	//options.minimizer_progress_to_stdout = true;
	//options.parameter_tolerance = 1E-12;
	ceres::Solver::Summary summary;
	ceres::Solve( options, &problem, &summary );
	//std::cout << summary.FullReport() << "\n";
	//std::cout << "Initial x: " << initial_guess << "\n";
	//std::cout << "Final   x: " << fit_vars << "\n";

	return fit_vars;
}
