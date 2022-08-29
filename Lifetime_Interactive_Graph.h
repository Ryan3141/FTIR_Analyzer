#pragma once

#include <tuple>
#include <QVector>
#include <functional>

#include "Interactive_Graph.h"


namespace Lifetime
{

enum class X_Units
{
	TIME_US = 0,
	TEMPERATURE_K,
	FIT_TIME_US,
	DONT_CHANGE
};

enum class Y_Units
{
	VOLTAGE_V = 0,
	TIME_US,
	FIT_VOLTAGE_V,
	DONT_CHANGE
};

using Single_Graph = Default_Single_Graph<X_Units, Y_Units>;

struct Axes
{
	using XY_Data = std::tuple< QVector<double>, QVector<double> >;

	const static X_Units default_x_units = X_Units::TIME_US;
	const static Y_Units default_y_units = Y_Units::VOLTAGE_V;
	X_Units x_units = default_x_units;
	Y_Units y_units = default_y_units;
	std::function<void()> graph_function;

	Axes( std::function<void()> regraph_function ) : graph_function( regraph_function )
	{
	}

	void Set_X_Units( X_Units units );
	void Set_Y_Units( Y_Units units );

	Prepared_Data Prepare_XY_Data( const Single_Graph & graph_data ) const;
	void Graph_XY_Data( QString measurement_name, const Single_Graph & graph );

	QVector<double> background_x_data;
	QVector<double> background_y_data;

	const static double X_Unit_Sensible_Maximum[ 3 ];
	const static QString X_Unit_Names[ 3 ];
	const static QString Y_Unit_Names[ 3 ];
	const static QString Change_To_X_Unit_Names[ 3 ];
	const static QString Change_To_Y_Unit_Names[ 3 ];
};

using Graph_Base = ::Interactive_Graph<Single_Graph, Axes>;

class Interactive_Graph :
	public Graph_Base
{
public:
	Interactive_Graph( QWidget* parent = nullptr );

	void Change_Axes( int index );
};

template< typename FloatType >
constexpr FloatType Convert_Units( X_Units original, X_Units converted, FloatType input )
{
	return input;
}

}