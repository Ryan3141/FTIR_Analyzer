#include "SPA_File.h"

#include <fstream>

std::tuple<std_XY_Data, std_Metadata> Load_From_SPA( std::string file_name )
{
	using namespace std;
	std::ifstream in_file( file_name, ios::binary | ios::in );

	std::uint32_t data_location_in_file;
	in_file.seekg( 0x172, ios::beg );
	in_file.read( (char*)&data_location_in_file, sizeof( data_location_in_file ) );

	std::uint32_t amount_of_data;
	in_file.seekg( 0x176, ios::beg );
	in_file.read( (char*)&amount_of_data, sizeof( amount_of_data ) );

	std::vector<float> all_data( amount_of_data / 4 );
	in_file.seekg( data_location_in_file, ios::beg );
	in_file.read( (char*)&all_data[ 0 ], amount_of_data );
	std::ofstream test( "test.txt" );
	for( float data : all_data )
	{
		test << data << "\n";
	}

	//this->Graph( "", x_data, y_data, file_name, false );
	return std::tuple<std_XY_Data, std_Metadata>(); // Obviously unfinished
}