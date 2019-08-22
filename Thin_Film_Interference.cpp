#include "Thin_Film_Interference.h"

#include <filesystem>
#include <string>
#include <regex>

#include "boost/algorithm/string.hpp"

#ifdef _WIN32
namespace fs = std::experimental::filesystem;
#else
namespace fs = std::filesystem;
#endif

using namespace std;
using namespace boost;
using namespace arma;


arma::cx_double MCT_Index( double wavelength, double temperature_k, double composition );

std::map<std::string, Material> name_to_material
{
	{ "Si"          ,  Material::Si },
	{ "HgCdTe"      ,  Material::HgCdTe },
	{ "CdTe"        ,  Material::CdTe },
	{ "ZnSe"        ,  Material::ZnSe },
	{ "ZnS"         ,  Material::ZnS },
	{ "Ge"          ,  Material::Ge },
	{ "Si3N4"       ,  Material::Si3N4 },
	{ "BaF2"        ,  Material::BaF2 },
	{ "SuperLattice",  Material::SuperLattice },
	{ "TestA"       ,  Material::TestA },
	{ "TestB"       ,  Material::TestB },
	{ "Air"         ,  Material::Air }
};

Material_To_Refraction_Component Load_Index_Of_Refraction_Files( const fs::path & directory, const char* indicator );
std::tuple< std::vector<double>, std::vector<double> > Load_XY_CSV_Data( const fs::path & file_name );

Thin_Film_Interference::Thin_Film_Interference()
{
	for( const auto &[ material, x_y_data ] : Refraction_Coefficient )
	{
		const X_And_Y_Data & n_vs_lambda = Refraction_Coefficient[ material ];
		const X_And_Y_Data & k_vs_lambda = Attenuation_Coefficient[ material ];

		all_material_indices[ material ] = [&n_vs_lambda, &k_vs_lambda]( double wavelength, double temperature, double composition )
		{
			double n = Find_Closest_Datapoint( wavelength, std::get<0>( n_vs_lambda ), std::get<1>( n_vs_lambda ) );
			double k = Find_Closest_Datapoint( wavelength, std::get<0>( k_vs_lambda ), std::get<1>( k_vs_lambda ) );
			return arma::cx_double{ n, k };
		};
	}

	all_material_indices[ Material::HgCdTe ] = MCT_Index;
	all_material_indices[ Material::CdTe ] = []( double wavelength, double temperature, double composition ) { return MCT_Index( wavelength, temperature, 1.0 ); };

	all_material_indices[ Material::Air ] = []( double wavelength, double temperature, double composition ) { return arma::cx_double{ 1, 0 }; };
}


Thin_Film_Interference::~Thin_Film_Interference()
{
}

std::vector<double> Thin_Film_Interference::Get_Expected_Transmission( double temperature_k, const std::vector<Material_Layer> & layers, const arma::vec & wavelengths ) const
{
	//ofstream out_file( "Log.txt" );
	vector<double> output;

	for( double wavelength : wavelengths )
	{
		cx_double previous_n{ 1, 0 }; // Air
		cx_mat Overall_Matrix = arma::eye<cx_mat>( 2, 2 );
		//{
		//	cx_double current_n{ Get_Refraction_Index( Material::ZnSe, wavelength ) };
		//	cx_double sum = (current_n + 1.) / (2. * current_n);
		//	cx_double difference = (current_n - 1.) / (2. * current_n);
		//	Overall_Matrix = { { sum, difference },
		//					{ difference, sum } };
		//}
		for( const auto & layer : layers )
		{
			cx_double current_n{ Get_Refraction_Index( layer.material, wavelength, temperature_k, layer.composition ) };
			double k = 2 * datum::pi / wavelength;
			//cx_double sum = current_n + std::conj(previous_n);
			//cx_double difference = current_n - previous_n;
			//cx_mat A_matrix = { { sum, difference },
			//					{ std::conj(difference), std::conj(sum) } };
			//A_matrix /= 2 * current_n.real();
			cx_double sum = current_n + previous_n;
			cx_double difference = current_n - previous_n;
			cx_mat A_matrix = { { sum, difference },
								{ difference, sum } };
			A_matrix /= 2. * current_n;


			cx_double k_current = k * current_n;
			cx_double exponent = 1i * k_current * layer.thickness;
			cx_double exponent_conj = 1i * std::conj( k_current ) * layer.thickness;
			cx_double exponent_conj_neg = -1i * std::conj( k_current ) * layer.thickness;
			cx_double test_exponent_conj_neg = -1.*( 1i * std::conj( k_current ) * layer.thickness );
			cx_double test_exponent_conj_neg2 = cx_double{ 0,-1 } * std::conj( k_current ) * layer.thickness;
			//out_file << "exponent:" << exponent << std::endl;
			//out_file << "exponent_conj:" << exponent_conj << std::endl;
			//out_file << "exponent_conj_neg:" << exponent_conj_neg << std::endl;
			//out_file << "e^:" << std::exp( exponent ) << std::endl;
			//out_file << "e^conj:" << std::exp( exponent_conj_neg ) << std::endl;
			//out_file << "--" << wavelength << " " << current_n.imag() << std::endl;

			//cx_double sin_value = std::sin( k_current * layer.thickness );
			//cx_double cos_value = std::cos( k_current * layer.thickness );
			//cx_mat M_matrix = { { cos_value + 1i * sin_value, 0 },
			//					{ 0, cos_value + 1i * sin_value } };
			cx_mat M_matrix = { { std::exp( -1i * std::conj( k_current ) * layer.thickness ), 0 },
								{ 0, std::exp( 1i * k_current * layer.thickness ) } };

			//out_file << "A:" << A_matrix << std::endl;
			//out_file << "M:" << M_matrix << std::endl;
			cx_mat One_Interface_matrix = M_matrix * A_matrix;
			cx_mat partial_matrix = A_matrix * Overall_Matrix;
			Overall_Matrix = One_Interface_matrix * Overall_Matrix;
			//out_file << "Overall:" << Overall_Matrix << std::endl;
			{
				double transmission = 0;
				if( partial_matrix( 1, 1 ) != 0. )
					transmission = current_n.real() / previous_n.real() * std::norm( (partial_matrix( 0, 0 ) - partial_matrix( 0, 1 ) * partial_matrix( 1, 0 ) / partial_matrix( 1, 1 )) );
				//out_file << "Partial Transmission:" << transmission << std::endl;
				double reflection = 0;
				if( partial_matrix( 1, 1 ) != 0. )
					reflection = std::norm( partial_matrix( 1, 0 ) / partial_matrix( 1, 1 ) );
				//out_file << "Reflection:" << reflection << std::endl;
				//out_file << "Absorption:" << 1. - reflection - transmission << std::endl;
			}
			{
				double transmission = 0;
				if( Overall_Matrix( 1, 1 ) != 0. )
					transmission = current_n.real() / previous_n.real() * std::norm( (Overall_Matrix( 0, 0 ) - Overall_Matrix( 0, 1 ) * Overall_Matrix( 1, 0 ) / Overall_Matrix( 1, 1 )) );
				//out_file << "After M Transmission:" << transmission << std::endl;
			}
			previous_n = current_n;
			//cx_double last_exponent = { k_current.real() * thickness, k_current.imag() * thickness };
		}
		{
			//cx_double sum = 1. + std::conj( previous_n );
			//cx_double difference = 1. - previous_n;
			//cx_mat A_matrix = { { sum, difference },
			//					{ std::conj( difference ), std::conj( sum ) } };
			//A_matrix /= 2;
			cx_double sum = 1. + previous_n;
			cx_double difference = 1. - previous_n;
			cx_mat A_matrix = { { sum, difference },
								{ difference, sum } };
			A_matrix /= 2.;

			Overall_Matrix = A_matrix * Overall_Matrix;
			//out_file << "A:" << A_matrix << std::endl;
		}
		//auto debug00 = Overall_Matrix( 0, 0 );
		//auto debug01 = Overall_Matrix( 0, 1 );
		//auto debug10 = Overall_Matrix( 1, 0 );
		//auto debug11 = Overall_Matrix( 1, 1 );
		//auto debug_numerator = Overall_Matrix( 0, 1 ) * Overall_Matrix( 1, 0 );
		//auto debug_right_side = Overall_Matrix( 0, 1 ) * Overall_Matrix( 1, 0 ) / Overall_Matrix( 1, 1 );
		//auto debug_t = Overall_Matrix( 0, 0 ) - Overall_Matrix( 0, 1 ) * Overall_Matrix( 1, 0 ) / Overall_Matrix( 1, 1 );
		//auto debug_r = Overall_Matrix( 1, 0 ) / Overall_Matrix( 1, 1 );

		double transmission = 0;
		if( Overall_Matrix( 1, 1 ) != 0. )
			transmission = std::norm( (Overall_Matrix( 0, 0 ) - Overall_Matrix( 0, 1 ) * Overall_Matrix( 1, 0 ) / Overall_Matrix( 1, 1 )) );
		double reflection = 0;
		if( Overall_Matrix( 1, 1 ) != 0. )
			reflection = std::norm( Overall_Matrix( 1, 0 ) / Overall_Matrix( 1, 1 ) );
		output.push_back( transmission );
		//out_file << Overall_Matrix << std::endl;
		//out_file << "Wavelength:" << wavelength << std::endl;
		//out_file << "Final Transmission:" << transmission << std::endl;
		//out_file << "Final Reflection:" << reflection << std::endl;
		//out_file << "Final Absorption:" << 1. - reflection - transmission << std::endl;
		//out_file.flush();
	}

	return output;
}

std::vector<Material_Layer> Thin_Film_Interference::Get_Best_Fit( double temperature_k,
																  const std::vector<Material_Layer> & layers,
																  const Wavelength_Array & wavelengths,
																  const Transmission_Array & transmissions )
{
	return std::vector<Material_Layer>();
}

Material_To_Refraction_Component Thin_Film_Interference::Attenuation_Coefficient{ Load_Index_Of_Refraction_Files( "./Refractive_Index", "_k" ) };
Material_To_Refraction_Component Thin_Film_Interference::Refraction_Coefficient{ Load_Index_Of_Refraction_Files( "./Refractive_Index", "_n" ) };

arma::cx_double MCT_Index( double wavelength, double temperature_k, double composition )
{
	double n;
	double x = composition;
	{
		double A1 = 13.173 - 9.852 * x + 2.909 * x * x + 0.0001 * (300 - temperature_k);
		double B1 = 0.83 - 0.246 * x - 0.0961 * x * x + 8 * 0.00001 * (300 - temperature_k);
		double C1 = 6.706 - 14.437 * x + 8.531 * x * x + 7 * 0.00001 * (300 - temperature_k);
		double D1 = 1.953 * 0.00001 - 0.00128 * x + 1.853 * 0.00001 * x * x;

		n = std::sqrt( A1 + B1 / (1 - (C1 / wavelength) * (C1 / wavelength)) + D1 * wavelength * wavelength );
	}

	double k;
	//std::ofstream save_file( "Look at crossover.txt" );
	//for( double x : arma::linspace( 0.0, 1.0, 1001 ) )
	if( false )
	{
		double T0 = 61.9;  // Initial parameter is 81.9.Adjusted.
		double W = T0 + temperature_k;
		double E0 = -0.3424 + 1.838 * x + 0.148 * x * x * x * x;
		double sigma = 3.267E4 * (1 + x);
		double alpha0 = std::exp( 53.61 * x - 18.88 );
		double beta = 3.109E5 * std::sqrt( (1 + x) / W );  // Initial parameter is 2.109E5.Adjusted.
		double Eg = E0 + (6.29E-2 + 7.68E-4 * temperature_k) * ((1 - 2.14 * x) / (1 + x));

		double E = 4.13566743 * 3 / 10 / wavelength;
		double ab1 = alpha0 * std::exp( sigma * (E - E0) / W );
		double ab2 = 0;
		if( E >= Eg )
			ab2 = beta * std::sqrt( E - Eg );

		//if( ab1 < crossover_a && ab2 < crossover_a )
		//	return ab1 / 4 / datum::pi * wavelength / 10000;
		//else
		//{
		//	if( ab2 != 0 )
		//		return ab2 / 4 / datum::pi * wavelength / 10000;
		//	else
		//		return ab1 / 4 / datum::pi * wavelength / 10000;
		//}
	}

	{
		double T0 = 61.9;  // Initial parameter is 81.9.Adjusted.
		double W = T0 + temperature_k;
		double E0 = -0.3424 + 1.838 * x + 0.148 * x * x * x * x;
		double sigma = 3.267E4 * (1 + x);
		double alpha0 = std::exp( 53.61 * x - 18.88 );
		double beta = 3.109E5 * std::sqrt( (1 + x) / W );  // Initial parameter is 2.109E5.Adjusted.
		double Eg = E0 + (6.29E-2 + 7.68E-4 * temperature_k) * ((1 - 2.14 * x) / (1 + x));

		double E = 4.13566743 * 3 / 10 / wavelength;
		double ab1 = alpha0 * std::exp( sigma * (E - E0) / W );
		double ab2 = 0;
		if( E >= Eg )
			ab2 = beta * std::sqrt( E - Eg );

		//if( ab1 < crossover_a && ab2 < crossover_a )
		//	return ab1 / 4 / datum::pi * wavelength / 10000;
		//else
		//{
		//	if( ab2 != 0 )
		//		return ab2 / 4 / datum::pi * wavelength / 10000;
		//	else
		//		return ab1 / 4 / datum::pi * wavelength / 10000;
		//}
		k = ab2 / 4 / datum::pi * wavelength / 10000;
	}

	return arma::cx_double{ n, k };
}

Material_To_Refraction_Component Load_Index_Of_Refraction_Files( const fs::path & directory, const char* indicator )
{
	if( !fs::exists( directory ) || !fs::is_directory( directory ) )
		return Material_To_Refraction_Component();
		//throw "Cannot access directory: " + directory.string();

	Material_To_Refraction_Component full_list;

	for( auto & file : fs::directory_iterator( directory ) )
	{
		string just_file_name = file.path().filename().string();
		if( !fs::is_regular_file( file ) || !contains( just_file_name, indicator ) )
			continue;

		for( const auto & material : name_to_material )
		{
			string material_name = material.first;
			if( std::regex_search( just_file_name, std::smatch(), std::regex( "^" + material_name + "_[kn]\\.csv" ) ) )
				full_list[ material.second ] = Load_XY_CSV_Data( file.path() );
		}
	}

	return full_list;
}

std::tuple< std::vector<double>, std::vector<double> > Load_XY_CSV_Data( const fs::path & file_name )
{
	std::tuple< std::vector<double>, std::vector<double> > output;
	ifstream data_file( file_name );
	string whole_file( (std::istreambuf_iterator<char>( data_file )),
					   std::istreambuf_iterator<char>() );

	vector< string > split_by_line;
	split( split_by_line, whole_file, is_any_of( "\r\n" ), algorithm::token_compress_on ); // this works for \r or \n file endings
	for( const string & one_line : split_by_line )
	{
		if( one_line.size() == 0 )
			continue; // Ignore blank lines

		vector< string > split_by_commas;
		split( split_by_commas, one_line, is_any_of(",") );
		if( split_by_commas.size() != 2 )
			throw "Invalid formatting in file " + file_name.string();
		
		std::get<0>( output ).push_back( std::stod( split_by_commas[ 0 ] ) * 1e-6 );
		std::get<1>( output ).push_back( std::stod( split_by_commas[ 1 ] ) );
	}

	const auto how_to_sort = make_sort_permutation( std::get<0>( output ) );
	apply_permutation_in_place( std::get<0>( output ), how_to_sort );
	apply_permutation_in_place( std::get<1>( output ), how_to_sort );

	return output;
}

