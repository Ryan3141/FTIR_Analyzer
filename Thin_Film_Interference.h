#pragma once

#include <array>
#include <optional>
#include <algorithm>
#include <numeric>
#include <vector>
#include <map>
#include <ostream>
#include <armadillo>
//#include <boost/units/systems/si.hpp>
#include <QObject>
#include <qmetatype.h>
#include <cppad/ipopt/solve.hpp>

#include "rangeless_helper.hpp"

#include "Units.h"

enum class Material
{
	Si,
	HgCdTe,
	CdTe,
	ZnSe,
	ZnS,
	Ge,
	Si3N4,
	BaF2,
	SuperLattice,
	SiO2,
	QDots,
	TestA,
	TestB,
	Air,
	AlAs,
	GaAs,
	InAs,
	Mirror,
	INVALID
};

extern std::map<std::string, Material> name_to_material;
extern std::map< Material, std::array< std::optional<double>, 4 > > defaults_per_material;

//using Length = boost::units::quantity<boost::units::si::length, double>;

struct Optional_Material_Parameters
{
	Optional_Material_Parameters() : all()
	{
	}
	Optional_Material_Parameters( const Optional_Material_Parameters& rhs )
	{
		this->material_name = rhs.material_name;
		this->temperature =   rhs.temperature;
		this->all =           rhs.all;
		this->thickness        = this->all[ 0 ];
		this->composition      = this->all[ 1 ];
		this->tauts_gap_eV     = this->all[ 2 ];
		this->urbach_energy_eV = this->all[ 3 ];
	}
	Optional_Material_Parameters & operator=( const Optional_Material_Parameters & rhs )
	{
		this->material_name = rhs.material_name;
		this->temperature =   rhs.temperature;
		this->all =           rhs.all;
		this->thickness        = this->all[ 0 ];
		this->composition      = this->all[ 1 ];
		this->tauts_gap_eV     = this->all[ 2 ];
		this->urbach_energy_eV = this->all[ 3 ];
		return *this;
	}


	Optional_Material_Parameters( std::string material_name,
								  std::optional< double > temperature = {},
								  std::optional< double > thickness = {},
								  std::optional< double > composition = {},
								  std::optional< double > tauts_gap_eV = {},
								  std::optional< double > urbach_energy_eV = {} )
	{
		this->material_name =    material_name;
		this->temperature =      temperature;
		this->thickness =        thickness;
		this->composition =      composition;
		this->tauts_gap_eV =     tauts_gap_eV;
		this->urbach_energy_eV = urbach_energy_eV;
	}

	std::string material_name;
	std::optional< double > temperature;

	std::optional< double > & thickness        = all[ 0 ];
	std::optional< double > & composition      = all[ 1 ];
	std::optional< double > & tauts_gap_eV     = all[ 2 ];
	std::optional< double > & urbach_energy_eV = all[ 3 ];

	std::array< std::optional< double >, 4 > all;
};

#if 0
struct Optional_Material_Parameters_AD
{
	using Real = std::optional< CppAD::AD<double> >;

	std::string material_name;
	Real temperature;
	const std::optional< double > & thickness() const        { return all[ 0 ]; }
	const std::optional< double > & composition() const      { return all[ 1 ]; }
	const std::optional< double > & tauts_gap_eV() const     { return all[ 2 ]; }
	const std::optional< double > & urbach_energy_eV() const { return all[ 3 ]; }
	std::optional< double > & thickness()        { return all[ 0 ]; }
	std::optional< double > & composition()      { return all[ 1 ]; }
	std::optional< double > & tauts_gap_eV()     { return all[ 2 ]; }
	std::optional< double > & urbach_energy_eV() { return all[ 3 ]; }

	std::optional< double > all[ 4 ];
};
#endif

template< typename Real >
inline std::ostream& operator<<( std::ostream& os, const Optional_Material_Parameters& params )
{
	return os << "material_name = " << params.material_name << "\n"
		<< "temperature = "      << params.temperature     .value_or(-1.) << "\n"
		<< "thickness = "        << params.thickness       .value_or(-1.) << "\n"
		<< "composition = "      << params.composition     .value_or(-1.) << "\n"
		<< "tauts_gap_eV = "     << params.tauts_gap_eV    .value_or(-1.) << "\n"
		<< "urbach_energy_eV = " << params.urbach_energy_eV.value_or(-1.) << "\n";
}

struct Material_Layer
{
	Material_Layer()
	{
		this->material = Material::Air;
	}

	Material_Layer( Material material, Optional_Material_Parameters parameters )
	{
		this->material = material;
		this->parameters = parameters;
	}

	Material material;
	Optional_Material_Parameters parameters;
	std::array< bool, 4 > what_to_fit;
};

inline std::vector<std::optional<double>*> Get_Things_To_Fit( std::vector< Material_Layer > & layers )
{
	std::vector<std::optional<double>*> output;
	for( Material_Layer & layer : layers )
	{
		for( int i = 0; auto & value : layer.parameters.all )
		{
			if( layer.what_to_fit[ i++ ] )
				output.push_back( &value );
		}
	}

	return output;
}

struct Result_Data
{
	arma::vec transmission;
	arma::vec refelection;
};

Q_DECLARE_METATYPE( Material_Layer );
Q_DECLARE_METATYPE( std::vector<Material_Layer> );

using X_And_Y_Data = std::tuple< std::vector<double>, std::vector<double> >;
using Material_To_Refraction_Component = std::map< Material, X_And_Y_Data >;

inline arma::vec Find_Closest_Datapoint( const arma::vec & x_for_lookup, const std::vector<double> & x_values, const std::vector<double> & y_values )
{
	if( x_values.size() == 0 || x_values.size() != y_values.size() )
		throw "Issue with data";
	// .transform( [](double val) { return (val + 123.0); } );
	double highest_value = x_values.back();
	double lowest_value = x_values.front();

	arma::vec output = x_for_lookup;
	output.transform( [&]( double x )
	{
		auto test = (x >= highest_value) * highest_value;
		if( x >= highest_value )
			return y_values.back();
		else if( x <= lowest_value )
			return y_values.front();

		int i = std::distance( x_values.begin(), std::lower_bound( x_values.begin(), x_values.end(), x ) );
		double to_left_point = x - x_values[ i - 1 ];
		double to_right_point = x_values[ i ] - x;
		double weighted_average = to_left_point * y_values[ i ] + to_right_point * y_values[ i - 1 ];
		weighted_average /= x_values[ i ] - x_values[ i - 1 ];
		return weighted_average;
	} );
	return output;
}

class Thin_Film_Interference : public QObject
{
	Q_OBJECT

signals:
	void Updated_Guess( std::vector<Material_Layer> layers );
	void Final_Guess( std::vector<Material_Layer> layers );
	void Debug_Plot( arma::vec wavelengths, arma::vec y_data );

public:
	using IndexFunction = std::function< arma::cx_vec( const arma::vec& wavelengths, Optional_Material_Parameters optional_parameters ) >;

	//std::map< Material, std::function<arma::cx_double( Material, double, double )> > Get_Refraction_Index;

	void Get_Best_Fit( const std::vector<Material_Layer> & layers, const arma::vec & wavelengths, const arma::vec & transmissions, Material_Layer backside_material );

	void Quit_Early();
	static std::map< Material, IndexFunction > all_material_indices;

private:
	static std::map< Material, IndexFunction > CreateIndexFunctions();

	bool quit_early = false;

	static Material_To_Refraction_Component Attenuation_Coefficient;
	static Material_To_Refraction_Component Refraction_Coefficient;

};

inline arma::cx_vec Get_Refraction_Index( Material mat,
	const arma::vec& wavelengths,
	Optional_Material_Parameters optional_parameters )
{
	using IndexFunction = std::function< arma::cx_vec( const arma::vec& wavelengths, Optional_Material_Parameters optional_parameters ) >;
	auto refraction_index_function = Thin_Film_Interference::all_material_indices.find( mat );
	if( refraction_index_function != Thin_Film_Interference::all_material_indices.end() )
		return refraction_index_function->second( wavelengths, optional_parameters );
	else
		throw "Material unavailable";
}

Result_Data Get_Expected_Transmission( const std::vector<Material_Layer>& layers, const arma::vec& wavelengths, Material_Layer backside_material );

//template < typename T, typename Compare = std::less<T> >
//std::vector<std::size_t> make_sort_permutation(
//	const std::vector<T>& vec,
//	Compare compare = Compare() )
//{
//	std::vector<std::size_t> p( vec.size() );
//	std::iota( p.begin(), p.end(), 0 );
//	std::sort( p.begin(), p.end(),
//			   [&]( std::size_t i, std::size_t j ) { return compare( vec[ i ], vec[ j ] ); } );
//	return p;
//}
//
//template <typename T>
//std::vector<T> apply_permutation(
//	const std::vector<T>& vec,
//	const std::vector<std::size_t>& p )
//{
//	std::vector<T> sorted_vec( vec.size() );
//	std::transform( p.begin(), p.end(), sorted_vec.begin(),
//					[&]( std::size_t i ) { return vec[ i ]; } );
//	return sorted_vec;
//}
//
//template <typename T>
//void apply_permutation_in_place(
//	std::vector<T>& vec,
//	const std::vector<std::size_t>& p )
//{
//	std::vector<bool> done( vec.size() );
//	for( std::size_t i = 0; i < vec.size(); ++i )
//	{
//		if( done[ i ] )
//		{
//			continue;
//		}
//		done[ i ] = true;
//		std::size_t prev_j = i;
//		std::size_t j = p[ i ];
//		while( i != j )
//		{
//			std::swap( vec[ prev_j ], vec[ j ] );
//			done[ j ] = true;
//			prev_j = j;
//			j = p[ j ];
//		}
//	}
//}