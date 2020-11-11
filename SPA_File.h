#pragma once

#include <vector>
#include <tuple>
#include <string>

using std_XY_Data = std::tuple< std::vector<double>, std::vector<double> >;
using std_Metadata = std::vector<std::string>;

std::tuple<std_XY_Data, std_Metadata> Load_From_SPA( std::string file_name );