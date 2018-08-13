#pragma once

#include <algorithm>
#include <numeric>
#include <vector>
#include <map>
#include <armadillo>
//#include <boost/units/systems/si.hpp>

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
	TestA,
	TestB,
	Air
};

extern std::map<const char*, Material> name_to_material;

//using Length = boost::units::quantity<boost::units::si::length, double>;

struct Material_Layer
{
	Material material;
	double thickness;
};

using X_And_Y_Data = std::tuple< std::vector<double>, std::vector<double> >;
using Material_To_Refraction_Component = std::map< Material, X_And_Y_Data >;

inline double Find_Closest_Datapoint( double x, const std::vector<double> & x_values, const std::vector<double> & y_values )
{
	if( x_values.size() == 0 || x_values.size() != y_values.size() )
		throw "Issue with data";
	if( x >= x_values.back() )
		return y_values.back();
	else if( x <= x_values.front() )
		return y_values.front();

	int i = std::distance( x_values.begin(), std::lower_bound( x_values.begin(), x_values.end(), x ) );
	double to_left_point = x - x_values[ i - 1 ];
	double to_right_point = x_values[ i ] - x;
	double weighted_average = to_left_point * y_values[ i ] + to_right_point * y_values[ i - 1 ];
	weighted_average /= x_values[ i ] - x_values[ i - 1 ];
	return weighted_average;
}

class Thin_Film_Interference
{
public:
	Thin_Film_Interference();
	~Thin_Film_Interference();

	std::vector<double> Get_Expected_Transmission( std::vector<Material_Layer> & layers, const arma::vec & wavelengths ) const;

	inline arma::cx_double Get_Refraction_Index( Material mat, double wavelength ) const
	{
		const X_And_Y_Data & n_vs_lambda = Refraction_Coefficient[ mat ];
		const X_And_Y_Data & k_vs_lambda = Attenuation_Coefficient[ mat ];

		double n = Find_Closest_Datapoint( wavelength, std::get<0>( n_vs_lambda ), std::get<1>( n_vs_lambda ) );
		double k = Find_Closest_Datapoint( wavelength, std::get<0>( k_vs_lambda ), std::get<1>( k_vs_lambda ) );
		return arma::cx_double{ n, k };
	}

private:

	static Material_To_Refraction_Component Attenuation_Coefficient;
	static Material_To_Refraction_Component Refraction_Coefficient;

};

template < typename T, typename Compare = std::less<T> >
std::vector<std::size_t> make_sort_permutation(
	const std::vector<T>& vec,
	Compare& compare = Compare() )
{
	std::vector<std::size_t> p( vec.size() );
	std::iota( p.begin(), p.end(), 0 );
	std::sort( p.begin(), p.end(),
			   [&]( std::size_t i, std::size_t j ) { return compare( vec[ i ], vec[ j ] ); } );
	return p;
}

template <typename T>
std::vector<T> apply_permutation(
	const std::vector<T>& vec,
	const std::vector<std::size_t>& p )
{
	std::vector<T> sorted_vec( vec.size() );
	std::transform( p.begin(), p.end(), sorted_vec.begin(),
					[&]( std::size_t i ) { return vec[ i ]; } );
	return sorted_vec;
}

template <typename T>
void apply_permutation_in_place(
	std::vector<T>& vec,
	const std::vector<std::size_t>& p )
{
	std::vector<bool> done( vec.size() );
	for( std::size_t i = 0; i < vec.size(); ++i )
	{
		if( done[ i ] )
		{
			continue;
		}
		done[ i ] = true;
		std::size_t prev_j = i;
		std::size_t j = p[ i ];
		while( i != j )
		{
			std::swap( vec[ prev_j ], vec[ j ] );
			done[ j ] = true;
			prev_j = j;
			j = p[ j ];
		}
	}
}