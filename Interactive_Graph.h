#pragma once
#include "qcustomplot.h"

#include <vector>
#include <functional>

class Interactive_Graph :
	public QCustomPlot
{
	Q_OBJECT


signals:
	void Graph_Selected( QCPGraph* selected_graph );

public:
	explicit Interactive_Graph( QWidget *parent = 0 );
	void Initialize_Graph();

	void selectionChanged();
	void mousePress();
	void titleDoubleClick( QMouseEvent * event );
	void axisLabelDoubleClick( QCPAxis * axis, QCPAxis::SelectablePart part );
	void legendDoubleClick( QCPLegend * legend, QCPAbstractLegendItem * item );
	void mouseWheel();
	void refitGraphs();
	void removeSelectedGraph();
	void removeAllGraphs();
	void graphContextMenuRequest( QPoint pos );
	void moveLegend();
	void graphClicked( QCPAbstractPlottable *plottable, int dataIndex );
	void saveCurrentGraph();
	QCPGraph* Graph( QVector<double> x_data, QVector<double> y_data, QString measurement_name = QString() );
	//void RegraphAll();
	void UpdateGraph( QCPGraph* existing_graph, QVector<double> x_data, QVector<double> y_data );

	std::vector< std::function<void( Interactive_Graph*, QMenu*, QPoint )> > menu_functions;
private:


};

