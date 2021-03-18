#pragma once

#include <array>
#include <optional>
#include <algorithm>
#include <numeric>
#include <vector>
#include <map>
#include <armadillo>
//#include <boost/units/systems/si.hpp>
#include <QObject>
#include <qmetatype.h>

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

	Optional_Material_Parameters( std::string material_name,
								  std::optional< double > temperature = {},
								  std::optional< double > thickness = {},
								  std::optional< double > composition = {},
								  std::optional< double > tauts_gap_eV = {},
								  std::optional< double > urbach_energy_eV = {} )
	{
		this->material_name = material_name;
		this->temperature = temperature;
		this->thickness = thickness;
		this->composition = composition;
		this->tauts_gap_eV = tauts_gap_eV;
		this->urbach_energy_eV = urbach_energy_eV;
	}

	std::string material_name;
	std::optional< double > temperature;
	union
	{
		struct
		{
			std::optional< double > thickness;
			std::optional< double > composition;
			std::optional< double > tauts_gap_eV;
			std::optional< double > urbach_energy_eV;
		};

		std::optional< double > all[ 4 ];
	};
};

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
		for( auto [ should_fit, value ] : fn::zip( layer.what_to_fit, layer.parameters.all ) )
		{
			if( should_fit )
				output.emplace_back( &value );
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
	Thin_Film_Interference();

	Result_Data Get_Expected_Transmission( const std::vector<Material_Layer> & layers, const arma::vec & wavelengths, Material_Layer backside_material ) const;
	//std::map< Material, std::function<arma::cx_double( Material, double, double )> > Get_Refraction_Index;

	void Get_Best_Fit( const std::vector<Material_Layer> & layers, const arma::vec & wavelengths, const arma::vec & transmissions, Material_Layer backside_material );

	inline arma::cx_vec Get_Refraction_Index( Material mat,
											  const arma::vec & wavelengths,
											  Optional_Material_Parameters optional_parameters ) const
	{
		auto refraction_index_function = all_material_indices.find( mat );
		if( refraction_index_function != all_material_indices.end() )
			return refraction_index_function->second( wavelengths, optional_parameters );
		else
			throw "Material unavailable";
	}

private:
	using IndexFunction = std::function< arma::cx_vec( const arma::vec & wavelengths, Optional_Material_Parameters optional_parameters ) >;
	std::map< Material, IndexFunction > all_material_indices;

	static Material_To_Refraction_Component Attenuation_Coefficient;
	static Material_To_Refraction_Component Refraction_Coefficient;

};

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