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
struct Default_Single_Graph
{
	X_Types x_units;
	Y_Types y_units;
	QVector<double> x_data;
	QVector<double> y_data;
	QCPGraph* graph_pointer;
	Labeled_Metadata meta;
	std::optional<double> x_location_for_y_alignment;

	void SetColor( const QColor & color ) const
	{
		auto pen = graph_pointer->pen();
		pen.setColor( color );
		graph_pointer->setPen( pen );
	}
	std::vector<QCPGraph*> Get_Graphs() const
	{
		return { graph_pointer };
	}
};

struct Prepared_Data
{
	QVector<double> x_data;
	QVector<double> y_data;
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
	Interactive_Graph_QObject_Adapter( QWidget* parent );
};

template< typename Single_Graph, typename Axes_Scales >
class Interactive_Graph :
	public Interactive_Graph_QObject_Adapter
{
	using base_type = Interactive_Graph<Single_Graph, Axes_Scales>;
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
	void Hide_Graph( QString graph_name, bool should_hide = true );
	void RegraphAll();
	const Single_Graph & GetSelectedGraphData() const;
	Single_Graph & FindDataFromGraphPointer( QCPGraph* graph_pointer );
	const Single_Graph & FindDataFromGraphPointer( QCPGraph* graph_pointer ) const;
	void Recolor_Graphs( QCPColorGradient::GradientPreset gradient );
	void Set_Title( QString title );

	//void saveRastered( const QString &fileName, int width, int height, double scale, const char *format, int quality, int resolution, QCP::ResolutionUnit resolutionUnit );
	bool savePdf( const QString &fileName, int width, int height, QCP::ExportPen exportPen = QCP::epAllowCosmetic, const QString &pdfCreator = QString(), const QString &pdfTitle = QString() );
	bool saveAsStandardPdf( const QString & fileName );

	template< decltype(Single_Graph::x_units) X, decltype( Single_Graph::y_units ) Y >
	Single_Graph & Graph( QVector<double> x_data, QVector<double> y_data, QString measurement_name, QString graph_title = QString(), Labeled_Metadata meta = Labeled_Metadata{} );

	Axes_Scales axes;

protected:
	std::map< QString, Single_Graph > remembered_graphs;
	std::vector< const Single_Graph* > graphs_in_order;
	QCPTextElement* title_display = nullptr;
	std::vector< std::function<void( QMenu*, QPoint )> > x_axis_menu_functions;
	std::vector< std::function<void( QMenu*, QPoint )> > y_axis_menu_functions;
	std::vector< std::function<void( QMenu*, QPoint )> > general_menu_functions;
};


#include "Interactive_Graph_Implementation.hpp"