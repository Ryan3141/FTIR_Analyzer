#include "HgCdTe.h"

namespace HgCdTe
{

struct temp_ratio
{
	double composition;
	arma::vec temperature_k;
	arma::vec ratio;
};
using X_to_T_vs_Ratio = std::vector<temp_ratio>;
X_to_T_vs_Ratio Load_Auger_Data()
{
	std::ifstream auger_file( "Auger Ratio Data.csv" );
	if( !auger_file.is_open() )
		std::cerr << "Auger Ratio Data.csv not found!\n";

	std::string header_row;
	std::getline( auger_file, header_row );
	std::vector<double> compositions;
	{
		std::string partial_string;
		for( int delim_count = 0; char c : header_row )
		{
			if( c == ',' )
				delim_count++;
			else
				partial_string += c;

			if( delim_count == 2 )
			{
				compositions.push_back( std::stod( partial_string ) );
				partial_string.clear();
				delim_count = 0;
			}
		}
		compositions.push_back( std::stod( partial_string ) );
	}
	arma::vec compositions_vec = arma::conv_to<arma::vec>::from( compositions );
	arma::uvec compositions_sorted_indices = arma::sort_index( compositions_vec );
	std::string ignore_row;
	std::getline( auger_file, ignore_row ); // Throw away second line
	arma::mat actual_data;
	actual_data.load( auger_file, arma::csv_ascii ); // The rest of the data is standard csv_values

	X_to_T_vs_Ratio output( compositions_vec.size() );
	for( int i = 0; i < compositions_vec.size(); i++ )
	{
		arma::uvec nonzeros = arma::find( actual_data.col( 2 * i ) != 0 );
		arma::vec T_column = arma::vec( actual_data.col( 2 * i ) )(nonzeros);
		arma::vec Ratio_column = arma::vec( actual_data.col( 2 * i + 1 ) )(nonzeros);
		arma::uvec indices = arma::sort_index( T_column );
		output[ compositions_sorted_indices[ i ] ] = { compositions_vec[ i ], T_column( indices ), Ratio_column( indices ) };
	}

	return output;
}

//template< typename X_Data, typename Predicate >
//arma::uvec searchsorted( typename const X_Data & x_data, const arma::vec & x_to_find, Predicate p = []( const auto & x ) { return x; } )
arma::uvec searchsorted( const arma::vec& x_data, const arma::vec& x_to_find )
{
	const int total_values = x_to_find.size();
	arma::uvec output( total_values );
	for( int i = 0; double x : x_to_find )
	{
		auto upper_i = std::upper_bound( x_data.begin(), x_data.end(), x );
		output[ i++ ] = std::distance( x_data.begin(), upper_i );
	}
	return output;
}

//template< typename X_Data, typename Predicate >
//auto linear_interpolation( const arma::vec & x_to_find, typename const X_Data & x_data, const arma::vec & y_data, Predicate p = []( const auto & x ) { return x; } )
arma::vec linear_interpolation( const arma::vec& x_to_find, const arma::vec& x_data, const arma::vec& y_data )
{
	arma::uvec first_over = searchsorted( x_data, x_to_find );
	arma::uvec first_over_clamped = arma::clamp( first_over, 1, x_data.size() - 1 );
	arma::vec to_lower_x = x_to_find - x_data( first_over_clamped - 1 );
	arma::vec to_upper_x = x_data( first_over_clamped ) - x_to_find;
	arma::vec output = (to_lower_x % y_data( first_over_clamped ) + to_upper_x % y_data( first_over_clamped - 1 )) / (to_lower_x + to_upper_x);
	output( to_lower_x <= 0 ).fill( y_data( 0 ) );
	output( to_upper_x <= 0 ).fill( y_data( y_data.size() - 1 ) );
	return output;
}

arma::vec Auger1_to_7_Lifetime_Ratio( double Cd_composition, const arma::vec& temperature_in_K ) // in cm^-3 / s
{
	static const X_to_T_vs_Ratio Auger_1to7_Ratio = Load_Auger_Data();
	auto lower_i = std::lower_bound( Auger_1to7_Ratio.begin(), Auger_1to7_Ratio.end(), Cd_composition, []( const temp_ratio& x, double y ) { return x.composition < y; } );
	auto upper_i = std::upper_bound( Auger_1to7_Ratio.begin(), Auger_1to7_Ratio.end(), Cd_composition, []( double x, const temp_ratio& y ) { return x < y.composition; } );
	lower_i = (lower_i == Auger_1to7_Ratio.end()) ? Auger_1to7_Ratio.begin()     : lower_i;
	upper_i = (upper_i == Auger_1to7_Ratio.end()) ? (Auger_1to7_Ratio.end() - 1) : upper_i;

	arma::vec overT = 1000 / temperature_in_K;
	arma::vec lower_comp_ratios = linear_interpolation( overT, lower_i->temperature_k, lower_i->ratio );
	arma::vec upper_comp_ratios = linear_interpolation( overT, upper_i->temperature_k, upper_i->ratio );

	double to_lower_x = Cd_composition - lower_i->composition;
	if( to_lower_x <= 0 )
		return lower_comp_ratios;
	double to_upper_x = upper_i->composition - Cd_composition;
	if( to_upper_x <= 0 )
		return upper_comp_ratios;

	return (to_upper_x * lower_comp_ratios + to_lower_x * upper_comp_ratios) / (to_lower_x + to_upper_x);
}

//arma::vec Auger1_to_7_Lifetime_Ratio( double Cd_composition, const arma::vec& temperature_in_K ) // in cm^-3 / s
//{
//	static X_to_T_vs_Ratio Auger_1to7_Ratio = Load_Auger_Data();
//	auto lower_i = std::lower_bound( Auger_1to7_Ratio.begin(), Auger_1to7_Ratio.end(), Cd_composition, []( const temp_ratio& x ) { return x.composition; } );
//	auto upper_i = std::upper_bound( Auger_1to7_Ratio.begin(), Auger_1to7_Ratio.end(), Cd_composition, []( const temp_ratio& x ) { return x.composition; } );
//
//	lower_i = (lower_i == Auger_1to7_Ratio.end()) ? Auger_1to7_Ratio.begin()     : lower_i;
//	upper_i = (upper_i == Auger_1to7_Ratio.end()) ? (Auger_1to7_Ratio.end() - 1) : upper_i;
//	double to_lower = std::abs( Cd_composition - lower_i->composition );
//	double to_upper = std::abs( upper_i->composition - Cd_composition );
//	double sum = to_lower + to_upper;
//
//	for( double one_temp : temperature_in_K )
//	{
//		auto closest = arma::abs( lower_i->temperature_k - temperature_in_K );
//		arma::uvec indices = arma::sort_index( closest );
//		auto [lower_T, upper_T] = std::make_tuple( lower_i->ratio[ indices[ 0 ] ], lower_i->ratio[ indices[ 1 ] ] );
//		double weighted_average = std::lerp( lower_i->ratio[ indices[ 0 ] ], lower_i->ratio[ indices[ 1 ] ] );
//	}
//}

}