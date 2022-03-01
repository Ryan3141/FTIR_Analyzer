#pragma once
#include "qcustomplot.h"

#include <vector>
#include <functional>
#include <armadillo>

#include <QString>
#include <QVector>
#include "SQL_Manager.h"

#include "Handy_Types_And_Conversions.h"

#include "Interactive_Graph.h"


namespace Report
{

enum class X_Units
{
	SIDE_LENGTH_UM = 0,
	AREA_OVER_PERIMETER_UM = 1,
	PERIMETER_OVER_AREA_UM = 2,
};

enum class Y_Units
{
	CURRENT_A = 0,
	CURRENT_A_PER_AREA_CM = 1,
	LOG_CURRENT_A = 2,
	LOG_CURRENT_A_PER_AREA_CM = 3,
};

struct IV_Scatter_Plot
{
	Structured_Metadata metadata;
	ID_To_XY_Data data;
	QCPGraph* graph_pointer;
};

template< typename FloatType >
constexpr FloatType Convert_Units( X_Units original, X_Units converted, FloatType input )
{
	if( original == converted )
		return input;
	switch( original )
	{
	case X_Units::SIDE_LENGTH_UM:
		switch( converted )
		{
			case X_Units::AREA_OVER_PERIMETER_UM: return input / 4; break;
			case X_Units::PERIMETER_OVER_AREA_UM: return 4 / input; break;
		};
	break;
	case X_Units::AREA_OVER_PERIMETER_UM:
		switch( converted )
		{
			case X_Units::SIDE_LENGTH_UM:         return 4 * input; break;
			case X_Units::PERIMETER_OVER_AREA_UM: return 1 / input; break;
		};
	break;
	case X_Units::PERIMETER_OVER_AREA_UM:
		switch( converted )
		{
			case X_Units::SIDE_LENGTH_UM:         return input / 4;	break;
			case X_Units::AREA_OVER_PERIMETER_UM: return 1 / input; break;
		};
	break;
	};

	throw "Error with datatype in " __FUNCTION__;
}

struct Axes
{
	struct Prepared_Data_And_Name
	{
		QString name;
		QVector<double> x_data;
		QVector<double> y_data;
	};

	Axes( std::function<void()> regraph_function ) : graph_function( regraph_function )
	{
	}

	static Prepared_Data_And_Name Build_XY_Data( const Structured_Metadata & metadata, const ID_To_XY_Data & data, double voltage_to_use );
	inline Prepared_Data Prepare_XY_Data( const IV_Scatter_Plot & graph, QVector<double> x_Qdata = {}, QVector<double> y_Qdata = {} )
	{
		if( x_Qdata.empty() || y_Qdata.empty() )
		{
			auto[ measurement_name, x_built_data, y_built_data ] = Axes::Build_XY_Data( graph.metadata, graph.data, this->set_voltage );
			x_Qdata = std::move( x_built_data );
			y_Qdata = std::move( y_built_data );
		}

		arma::vec side_lengths_um = arma::conv_to<arma::vec>::from( x_Qdata.toStdVector() );
		arma::vec currents_a = arma::conv_to<arma::vec>::from( y_Qdata.toStdVector() );
		switch( this->y_units )
		{
			case Y_Units::CURRENT_A:
			break;
			case Y_Units::CURRENT_A_PER_AREA_CM:
			{
				currents_a /= side_lengths_um % side_lengths_um * 1E-4;
			}
			break;
			case Y_Units::LOG_CURRENT_A:
			{
				//y_data = arma::log10( arma::abs( y_data ) ); // Plotting on log scale now so taking log is unnecessary
				currents_a = arma::abs( currents_a );
			}
			break;
			case Y_Units::LOG_CURRENT_A_PER_AREA_CM:
			{
				// Plotting on log scale now so taking log is unnecessary
				currents_a = arma::abs( currents_a / ( side_lengths_um % side_lengths_um * 1E-4 ) );
			}
			break;
		}

		switch( this->x_units )
		{
			case X_Units::SIDE_LENGTH_UM:
			break;
			case X_Units::AREA_OVER_PERIMETER_UM:
			{
				side_lengths_um /= 4;
			}
			break;
			case X_Units::PERIMETER_OVER_AREA_UM:
			{
				//y_data = arma::log10( arma::abs( y_data ) );
				side_lengths_um = 4 / side_lengths_um;
			}
			break;
		}
		return { toQVec( side_lengths_um ), toQVec( currents_a ) };
	}

	const static X_Units default_x_units = X_Units::AREA_OVER_PERIMETER_UM;
	const static Y_Units default_y_units = Y_Units::CURRENT_A;
	X_Units x_units = default_x_units;
	Y_Units y_units = default_y_units;
	std::function<void()> graph_function;
	double set_voltage = 0.1;

	const static QString X_Unit_Names[ 3 ];
	const static QString Y_Unit_Names[ 4 ];
	const static QString Change_To_Y_Unit_Names[ 4 ];
};

class Interactive_Graph :
	public ::Interactive_Graph<IV_Scatter_Plot, Axes>
{
public:
	Interactive_Graph( QWidget *parent = nullptr );

	template< X_Units X, Y_Units Y >
	const IV_Scatter_Plot & Graph( QVector<double> x_data, QVector<double> y_data, QString measurement_name, QString graph_title, Labeled_Metadata meta )
	{
	}

	const IV_Scatter_Plot & Graph( Structured_Metadata metadata, ID_To_XY_Data data );

private:
	const IV_Scatter_Plot & Store_IV_Scatter_Plot( QString measurement_name, Structured_Metadata metadata, ID_To_XY_Data data );

	//const Single_Graph< X_Units, Y_Units > & Graph( QVector<double> x_data, QVector<double> y_data, QString measurement_name, QString graph_title = QString(), Labeled_Metadata meta = Labeled_Metadata{} );

	QSharedPointer<QCPAxisTicker> linearTicker;
	QSharedPointer<QCPAxisTickerLog> logTicker;
};

}
