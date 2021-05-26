#include "Interactive_Graph_Toolbar.h"

#include "Interactive_Graph.h"
//#include "IV_Interactive_Graph.h"

Interactive_Graph_Toolbar::Interactive_Graph_Toolbar(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
}

void Interactive_Graph_Toolbar::Connect_To_Graph( Interactive_Graph* graph )
{
	auto range_x = graph->xAxis->range();
	ui.xMin_doubleSpinBox->setValue( range_x.lower );
	ui.xMax_doubleSpinBox->setValue( range_x.upper );
	auto range_y = graph->yAxis->range();
	ui.yMin_doubleSpinBox->setValue( range_y.lower );
	ui.yMax_doubleSpinBox->setValue( range_y.upper );

	auto update_x_axes = [ graph, this ]
	{
		graph->xAxis->setRange( ui.xMin_doubleSpinBox->value(), ui.xMax_doubleSpinBox->value() );
		graph->replot();
	};

	auto update_y_axes = [ graph, this ]
	{
		graph->yAxis->setRange( ui.yMin_doubleSpinBox->value(), ui.yMax_doubleSpinBox->value() );
		graph->replot();
	};

	connect( graph->xAxis, qOverload<const QCPRange &>( &QCPAxis::rangeChanged ), [ graph, this ]
	{
		QCPRange range = graph->xAxis->range();
		bool oldState_min = ui.xMin_doubleSpinBox->blockSignals( true ); // Prevent remove from triggering another changed signal
		bool oldState_max = ui.xMax_doubleSpinBox->blockSignals( true ); // Prevent remove from triggering another changed signal
		ui.xMin_doubleSpinBox->setValue( range.lower );
		ui.xMax_doubleSpinBox->setValue( range.upper );
		ui.xMin_doubleSpinBox->blockSignals( oldState_min );
		ui.xMax_doubleSpinBox->blockSignals( oldState_max );
	} );
	connect( graph->yAxis, qOverload<const QCPRange &>( &QCPAxis::rangeChanged ), [ graph, this ]
	{
		QCPRange range = graph->yAxis->range();
		bool oldState_min = ui.yMin_doubleSpinBox->blockSignals( true ); // Prevent remove from triggering another changed signal
		bool oldState_max = ui.yMax_doubleSpinBox->blockSignals( true ); // Prevent remove from triggering another changed signal
		ui.yMin_doubleSpinBox->setValue( range.lower );
		ui.yMax_doubleSpinBox->setValue( range.upper );
		ui.yMin_doubleSpinBox->blockSignals( oldState_min );
		ui.yMax_doubleSpinBox->blockSignals( oldState_max );
	} );

	connect( ui.xMin_doubleSpinBox, qOverload<double>( &QDoubleSpinBox::valueChanged ), update_x_axes );
	connect( ui.xMax_doubleSpinBox, qOverload<double>( &QDoubleSpinBox::valueChanged ), update_x_axes );
	connect( ui.yMin_doubleSpinBox, qOverload<double>( &QDoubleSpinBox::valueChanged ), update_y_axes );
	connect( ui.yMax_doubleSpinBox, qOverload<double>( &QDoubleSpinBox::valueChanged ), update_y_axes );

	connect( graph, &Interactive_Graph::X_Units_Changed, [ this ]( X_Unit_Type new_units )
	{
		//using namespace Unit_Type;
		switch( new_units )
		{
		case X_Unit_Type::WAVE_NUMBER:
		ui.xMin_doubleSpinBox->setSingleStep( 100.0 );
		ui.xMax_doubleSpinBox->setSingleStep( 100.0 );
		break;
		case X_Unit_Type::WAVELENGTH_MICRONS:
		ui.xMin_doubleSpinBox->setSingleStep( 0.5 );
		ui.xMax_doubleSpinBox->setSingleStep( 0.5 );
		break;
		case X_Unit_Type::ENERGY_EV:
		ui.xMin_doubleSpinBox->setSingleStep( 0.1 );
		ui.xMax_doubleSpinBox->setSingleStep( 0.1 );
		break;
		}
	} );
}
