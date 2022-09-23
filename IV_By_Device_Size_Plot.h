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


namespace IV_By_Size
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
	X_Units x_units;
	Y_Units y_units;
	Structured_Metadata metadata;
	ID_To_XY_Data data;
	QCPGraph* graph_pointer;
	QCPGraph* line_fit_pointer;

	void SetPen( const QPen & graphPen ) const
	{
		graph_pointer->setPen( graphPen );
		line_fit_pointer->setPen( graphPen );
	}
	std::vector<QCPGraph*> Get_Graphs() const
	{
		return { graph_pointer, line_fit_pointer };
	}
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
			case X_Units::SIDE_LENGTH_UM:         return 4 / input;	break;
			case X_Units::AREA_OVER_PERIMETER_UM: return 1 / input; break;
		};
	break;
	};

	throw "Error with datatype in " __FUNCTION__;
}

struct Linear_Equation
{
	double y_intercept;
	double slope;
};
Linear_Equation Linear_Regression( arma::vec x_data, arma::vec y_data );

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
				currents_a /= side_lengths_um % side_lengths_um * 1E-8; // A / um^2 * 1E8 = A / cm^2
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
				currents_a = arma::abs( currents_a / ( side_lengths_um % side_lengths_um * 1E-8 ) ); // A / um^2 * 1E8 = A / cm^2
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

	void Graph_XY_Data( QString measurement_name, const IV_Scatter_Plot & graph );

	inline Prepared_Data_And_Name Prepare_Linear_Fit_Data( QVector<double> alread_scaled_x_Qdata, QVector<double> alread_scaled_y_Qdata )
	{
		arma::vec x_data = arma::conv_to<arma::vec>::from( alread_scaled_x_Qdata.toStdVector() );
		arma::vec y_data = arma::conv_to<arma::vec>::from( alread_scaled_y_Qdata.toStdVector() );
		auto [ b, m ] = Linear_Regression( x_data, y_data );

		double x_max = arma::max( x_data );
		arma::vec fit_x = arma::linspace( 0.0, x_max, 1001 );
		arma::vec fit_y = m * fit_x + b;

		//switch( this->y_units )
		//{
		//	case Y_Units::CURRENT_A:
		//	break;
		//	case Y_Units::CURRENT_A_PER_AREA_CM:
		//	{
		//		currents_a /= side_lengths_um % side_lengths_um * 1E-8; // A / um^2 * 1E8 = A / cm^2
		//	}
		//	break;
		//	case Y_Units::LOG_CURRENT_A:
		//	{
		//		//y_data = arma::log10( arma::abs( y_data ) ); // Plotting on log scale now so taking log is unnecessary
		//		currents_a = arma::abs( currents_a );
		//	}
		//	break;
		//	case Y_Units::LOG_CURRENT_A_PER_AREA_CM:
		//	{
		//		// Plotting on log scale now so taking log is unnecessary
		//		currents_a = arma::abs( currents_a / ( side_lengths_um % side_lengths_um * 1E-8 ) ); // A / um^2 * 1E8 = A / cm^2
		//	}
		//	break;
		//}

		//switch( this->x_units )
		//{
		//	case X_Units::SIDE_LENGTH_UM:
		//	break;
		//	case X_Units::AREA_OVER_PERIMETER_UM:
		//	{
		//		side_lengths_um /= 4;
		//	}
		//	break;
		//	case X_Units::PERIMETER_OVER_AREA_UM:
		//	{
		//		//y_data = arma::log10( arma::abs( y_data ) );
		//		side_lengths_um = 4 / side_lengths_um;
		//	}
		//	break;
		//}
		return { QString::number( m, 'E', 4 ) + " x + " + QString::number( b, 'E', 4 ), toQVec( fit_x ), toQVec( fit_y ) };
	}

	const static X_Units default_x_units = X_Units::PERIMETER_OVER_AREA_UM;
	const static Y_Units default_y_units = Y_Units::CURRENT_A_PER_AREA_CM;
	X_Units x_units = default_x_units;
	Y_Units y_units = default_y_units;
	std::function<void()> graph_function;
	double set_voltage = -0.1;

	const static QString X_Unit_Names[ 3 ];
	const static QString Y_Unit_Names[ 4 ];
	const static QString Change_To_X_Unit_Names[ 3 ];
	const static QString Change_To_Y_Unit_Names[ 4 ];
};

using Single_Graph = IV_Scatter_Plot;

class Interactive_Graph :
	public ::Interactive_Graph<IV_Scatter_Plot, Axes>
{
public:
	Interactive_Graph( QWidget *parent = nullptr );
	void Change_X_Axis( int index );
	void Change_Y_Axis( int index );

	const Single_Graph & Graph( Structured_Metadata metadata, ID_To_XY_Data data, QString plot_title = "" );
	void Show_Reference_Graph( QString measurement_name, QVector<double> x_data, QVector<double> y_data );
	void Hide_Reference_Graph( QString measurement_name );
	void Hide_Fit_Graphs( bool should_hide );

private:
	const Single_Graph & Store_IV_Scatter_Plot( QString measurement_name, Structured_Metadata metadata, ID_To_XY_Data data );

	//const Single_Graph< X_Units, Y_Units > & Graph( QVector<double> x_data, QVector<double> y_data, QString measurement_name, QString graph_title = QString(), Labeled_Metadata meta = Labeled_Metadata{} );
	QCPGraph* reference_graph_pointer = nullptr;

	QSharedPointer<QCPAxisTicker> linearTicker;
	QSharedPointer<QCPAxisTickerLog> logTicker;
};

}
