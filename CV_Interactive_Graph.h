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

struct Axes
{
	Axes( std::function<void()> regraph_function );

	std::tuple< QVector<double>, QVector<double> >
		Prepare_XY_Data( const Single_Graph< X_Units, Y_Units > & graph );

	const static X_Units default_x_units = X_Units::VOLTAGE_V;
	const static Y_Units default_y_units = Y_Units::CAPACITANCE_F;
	X_Units x_units = default_x_units;
	Y_Units y_units = default_y_units;
	std::function<void()> graph_function;

	const static QString X_Unit_Names[ 1 ];
	const static QString Y_Unit_Names[ 5 ];
};

class Interactive_Graph :
	public ::Interactive_Graph<X_Units, Y_Units, Axes>
{
public:
	Interactive_Graph( QWidget *parent = nullptr );
};

}
