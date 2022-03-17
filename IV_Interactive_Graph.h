#pragma once
#include "qcustomplot.h"

#include <vector>
#include <functional>
#include <armadillo>

#include <QString>
#include "Handy_Types_And_Conversions.h"

#include "Interactive_Graph.h"


namespace IV
{

enum class X_Units
{
	VOLTAGE_V = 0
};

enum class Y_Units
{
	CURRENT_A = 0,
	CURRENT_A_PER_AREA_CM = 1,
	LOG_CURRENT_A = 2,
	LOG_CURRENT_A_PER_AREA_CM = 3,
	ONE_SIDE_LOG_CURRENT_A_PER_AREA_CM = 4,
	RESISTANCE_OHM = 5,
	RESISTANCE_OHM_PER_AREA_CM = 6
};

using Single_Graph = Default_Single_Graph<X_Units, Y_Units>;

struct Axes
{
	inline Prepared_Data Prepare_XY_Data( const Single_Graph & graph )
	{
		arma::vec x_data = arma::conv_to<arma::vec>::from( graph.x_data.toStdVector() );
		arma::vec y_data = arma::conv_to<arma::vec>::from( graph.y_data.toStdVector() );
		switch( this->y_units )
		{
			case Y_Units::CURRENT_A:
			break;
			case Y_Units::CURRENT_A_PER_AREA_CM:
			{
				double side_cm = graph.meta.find( "device_side_length_in_um" )->second.toDouble() * 1E-4;
				y_data /= side_cm * side_cm;
			}
			break;
			case Y_Units::LOG_CURRENT_A:
			{
				//y_data = arma::log10( arma::abs( y_data ) ); // Plotting on log scale now so taking log is unnecessary
				y_data = arma::abs( y_data );
			}
			break;
			case Y_Units::LOG_CURRENT_A_PER_AREA_CM:
			{
				double side_cm = graph.meta.find( "device_side_length_in_um" )->second.toDouble() * 1E-4;
				//y_data = arma::log10( arma::abs( y_data / ( side_cm * side_cm ) ) ); // Plotting on log scale now so taking log is unnecessary
				y_data = arma::abs( y_data / ( side_cm * side_cm ) );
			}
			break;
			case Y_Units::ONE_SIDE_LOG_CURRENT_A_PER_AREA_CM:
			{
				double side_cm = graph.meta.find( "device_side_length_in_um" )->second.toDouble() * 1E-4;
				//y_data = arma::log10( arma::abs( y_data / ( side_cm * side_cm ) ) ); // Plotting on log scale now so taking log is unnecessary
				y_data = arma::abs( y_data / ( side_cm * side_cm ) );
				x_data = arma::abs( x_data );
			}
			break;
			case Y_Units::RESISTANCE_OHM:
			{
				y_data = arma::diff( x_data ) / arma::diff( y_data );
				x_data = x_data( arma::span( 0, x_data.size() - 1 ) );
			}
			break;
			case Y_Units::RESISTANCE_OHM_PER_AREA_CM:
			{
				double side_cm = graph.meta.find( "device_side_length_in_um" )->second.toDouble() * 1E-4;
				y_data = ( arma::diff( x_data ) / arma::diff( y_data ) ) / ( side_cm * side_cm );
				x_data = x_data( arma::span( 0, x_data.size() - 1 ) );
			}
			break;
		}
		return { toQVec( x_data ), toQVec( y_data ) };
	}

	void Graph_XY_Data( QString measurement_name, const Single_Graph & graph )
	{
		auto[ x_data, y_data ] = this->Prepare_XY_Data( graph );
		graph.graph_pointer->setData( x_data, y_data );
	}


	const static X_Units default_x_units = X_Units::VOLTAGE_V;
	const static Y_Units default_y_units = Y_Units::CURRENT_A;
	X_Units x_units = default_x_units;
	Y_Units y_units = default_y_units;
	std::function<void()> graph_function;

	Axes( std::function<void()> regraph_function ) : graph_function( regraph_function )
	{
	}

	const static QString X_Unit_Names[ 1 ];
	const static QString Y_Unit_Names[ 7 ];
	const static QString Change_To_Y_Unit_Names[ 7 ];

	//const static QString X_Unit_Names[ 4 ] = { "Current (A)",
	//						"Current (mA)",
	//						"Current (" + QString( QChar( 0x03BC ) ) + "A)",
	//						"Current (nA)" };

};

using Graph_Base = ::Interactive_Graph<Default_Single_Graph<X_Units, Y_Units>, Axes>;

class Interactive_Graph :
	public Graph_Base
{
public:
	Interactive_Graph( QWidget *parent = nullptr );
	void Change_X_Axis( int index );
	void Change_Y_Axis( int index );

private:
	QSharedPointer<QCPAxisTicker> linearTicker;
	QSharedPointer<QCPAxisTickerLog> logTicker;
};


}
