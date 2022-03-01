#pragma once
#include "qcustomplot.h"

#include <vector>
#include <functional>
#include <armadillo>

#include <QString>
#include "Handy_Types_And_Conversions.h"

#include "Interactive_Graph.h"


namespace CV
{

using Single_Graph = Default_Single_Graph<X_Units, Y_Units>;

struct Axes
{
	Axes( std::function<void()> regraph_function );

	Prepared_Data Prepare_XY_Data( const Single_Graph & graph );

	const static X_Units default_x_units = X_Units::VOLTAGE_V;
	const static Y_Units default_y_units = Y_Units::CAPACITANCE_F;
	X_Units x_units = default_x_units;
	Y_Units y_units = default_y_units;
	std::function<void()> graph_function;

	const static QString X_Unit_Names[ 1 ];
	const static QString Y_Unit_Names[ 5 ];
};

using Graph_Base = ::Interactive_Graph<Single_Graph, Axes>;

class Interactive_Graph :
	public Graph_Base
{
public:
	Interactive_Graph( QWidget *parent = nullptr );
};

}
