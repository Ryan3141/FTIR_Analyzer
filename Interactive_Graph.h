#pragma once
#include "qcustomplot.h"

#include <vector>
#include <functional>
#include <map>

#include <QVector>

#include "Units.h"

template<typename FloatType>
constexpr QVector<FloatType> toQVec( const std::vector<FloatType> & input )
{
	return QVector<FloatType>::fromStdVector( input );
}

inline QVector<double> toQVec( const arma::vec & input )
{
	return toQVec( std::move( arma::conv_to<std::vector<double>>::from( input ) ) );
}

using Metadata = std::vector<QVariant>;

struct Single_Graph
{
	Unit_Type x_units;
	QVector<double> x_data;
	QVector<double> y_data;
	QCPGraph* graph_pointer;
	bool allow_y_scaling;
	Metadata meta;
};

class Interactive_Graph :
	public QCustomPlot
{
	Q_OBJECT


signals:
	void Graph_Selected( QCPGraph* selected_graph );
	void X_Units_Changed( Unit_Type new_units );
public:
	using XY_Data = std::tuple< QVector<double>, QVector<double> >;

	explicit Interactive_Graph( QWidget *parent = 0 );
	void Initialize_Graph();

	void selectionChanged();
	void mousePress();
	void mouseDoubleClickEvent( QMouseEvent* event ) override;
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
	const Single_Graph & Graph( QVector<double> x_data, QVector<double> y_data, QString unique_name, QString graph_title = QString(), bool allow_y_scaling = true, Metadata meta = Metadata() );
	void Hide_Graph( QString graph_name );
	void RegraphAll();
	void UpdateGraph( QCPGraph* existing_graph, QVector<double> x_data, QVector<double> y_data );
	const Single_Graph & GetSelectedGraphData() const;
	const Single_Graph & FindDataFromGraphPointer( QCPGraph* graph_pointer ) const;

	Unit_Type x_axis_units = Unit_Type::WAVELENGTH_MICRONS;
	//std::function<double( double )> x_display_method{ []( double x ) { return 10000 / x; } };
	void Set_As_Background( XY_Data xy );
	void Clear_Background();
	std::function<double( double, double )> y_display_method{ []( double x, double y ) { return y; } };

private:
	std::map< QString, Single_Graph > remembered_graphs;
	std::vector< std::function<void( Interactive_Graph*, QMenu*, QPoint )> > x_axis_menu_functions;
};

std::tuple< std::vector<double>, std::vector<double> > Load_XY_CSV_Data( const std::string & data_to_parse, const char* delimiter = "," );
