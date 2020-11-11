#pragma once

#include <armadillo>

namespace III_V_Data
{
arma::cx_vec Get_AlAs_Refraction_Index( const arma::vec & wavelengths, double temperature_k );
arma::cx_vec Get_GaAs_Refraction_Index( const arma::vec & wavelengths, double temperature_k );
arma::cx_vec Get_InAs_Refraction_Index( const arma::vec & wavelengths, double temperature_k );

}