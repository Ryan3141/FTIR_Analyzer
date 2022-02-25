#pragma once
#include "qcustomplot.h"

#include <vector>
#include <functional>
#include <map>
#include <optional>

#include <QVector>

#include "Units.h"
#include "Handy_Types_And_Conversions.h"


template< typename X_Types, typename Y_Types >
struct Single_Graph
{
	X_Types x_units;
	Y_Types y_units;
	QVector<double> x_data;
	QVector<double> y_data;
	QCPGraph* graph_pointer;
	Labeled_Metadata meta;
	std::optional<double> x_location_for_y_alignment;
};

class Interactive_Graph_QObject_Adapter :
	public QCustomPlot
{
	Q_OBJECT

signals:
	void Graph_Selected( QCPGraph* selected_graph );
	void X_Units_Changed();
	void Y_Units_Changed();

public:
	Interactive_Graph_QObject_Adapter( QWidget* parent ) :
		QCustomPlot( parent )
	{
	}
};

template<typename X_Unit_Type, typename Y_Unit_Type, typename Axes_Scales>
class Interactive_Graph :
	public Interactive_Graph_QObject_Adapter
{
	using base_type = Interactive_Graph<X_Unit_Type, Y_Unit_Type, Axes_Scales>;
public:
	explicit Interactive_Graph( QWidget *parent = nullptr );
	void Initialize_Graph();

	void selectionChanged();
	void mousePress();
	//void mouseDoubleClickEvent( QMouseEvent* event ) override;
	void titleDoubleClick( QMouseEvent * event );
	void axisLabelDoubleClick( QCPAxis * axis, QCPAxis::SelectablePart part, QMouseEvent * event );
	void legendDoubleClick( QCPLegend * legend, QCPAbstractLegendItem * item, QMouseEvent * event );
	void mouseWheel();
	void refitGraphs( QMouseEvent* event );
	void removeSelectedGraph();
	void removeAllGraphs();
	void graphContextMenuRequest( QPoint pos );
	void moveLegend();
	void graphClicked( QCPAbstractPlottable *plottable, int dataIndex );
	void saveCurrentGraph();
	void Hide_Graph( QString graph_name );
	void RegraphAll();
	void UpdateGraph( QCPGraph* existing_graph, QVector<double> x_data, QVector<double> y_data );
	const Single_Graph< X_Unit_Type, Y_Unit_Type > & GetSelectedGraphData() const;
	const Single_Graph< X_Unit_Type, Y_Unit_Type > & FindDataFromGraphPointer( QCPGraph* graph_pointer ) const;
	void Recolor_Graphs( QCPColorGradient::GradientPreset gradient );

	template< X_Unit_Type X_Units, Y_Unit_Type Y_Units >
	const Single_Graph< X_Unit_Type, Y_Unit_Type > & Graph( QVector<double> x_data, QVector<double> y_data, QString measurement_name, QString graph_title = QString(), Labeled_Metadata meta = Labeled_Metadata{} );

	Axes_Scales axes;

protected:
	std::map< QString, Single_Graph< X_Unit_Type, Y_Unit_Type > > remembered_graphs;
	std::vector< Single_Graph< X_Unit_Type, Y_Unit_Type >* > graphs_in_order;
	std::vector< std::function<void( base_type*, QMenu*, QPoint )> > x_axis_menu_functions;
	std::vector< std::function<void( base_type*, QMenu*, QPoint )> > y_axis_menu_functions;
	std::vector< std::function<void( base_type*, QMenu*, QPoint )> > general_menu_functions;
};


#include "Interactive_Graph.cpp"