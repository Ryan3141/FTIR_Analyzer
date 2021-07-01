#pragma once
#include "qcustomplot.h"

#include <vector>
#include <functional>
#include <armadillo>

#include <QString>
#include "Handy_Types_And_Conversions.h"

#include "Interactive_Graph.h"

//enum class Unit_Type
//{
//	WAVE_NUMBER = 0,
//	WAVELENGTH_MICRONS = 1,
//	ENERGY_EV = 2
//};


namespace IV
{


//struct Single_Graph
//{
//	QVector<double> x_data;
//	QVector<double> y_data;
//	QCPGraph* graph_pointer;
//	bool allow_y_scaling;
//	Labeled_Metadata meta;
//};
//if( current_info.x_data.size() == x_data.size() &&
//	current_info.y_data.size() == y_data.size() )
struct Axes
{
	inline std::tuple< QVector<double>, QVector<double> > Prepare_XY_Data( const Single_Graph< X_Units, Y_Units > & graph )
	{
		arma::vec x_data = arma::conv_to<arma::vec>::from( graph.x_data.toStdVector() );
		arma::vec y_data = arma::conv_to<arma::vec>::from( graph.y_data.toStdVector() );
		switch( this->y_units )
		{
			case Y_Units::CURRENT_A:
			break;
			case Y_Units::CURRENT_A_PER_AREA_CM:
			{
				double side_cm = graph.meta.find( "Device Side Length (" + QString( QChar( 0x03BC ) ) + "m)" )->second.toDouble() * 1E-4;
				y_data /= side_cm * side_cm;
			}
			break;
			case Y_Units::LOG_CURRENT_A:
			{
				//y_data = arma::log10( arma::abs( y_data ) );
				y_data = arma::abs( y_data );
			}
			break;
			case Y_Units::LOG_CURRENT_A_PER_AREA_CM:
			{
				double side_cm = graph.meta.find( "Device Side Length (" + QString( QChar( 0x03BC ) ) + "m)" )->second.toDouble() * 1E-4;
				//y_data = arma::log10( arma::abs( y_data / ( side_cm * side_cm ) ) );
				y_data = arma::abs( y_data / ( side_cm * side_cm ) );
			}
			break;
			case Y_Units::ONE_SIDE_LOG_CURRENT_A_PER_AREA_CM:
			{
				double side_cm = graph.meta.find( "Device Side Length (" + QString( QChar( 0x03BC ) ) + "m)" )->second.toDouble() * 1E-4;
				//y_data = arma::log10( arma::abs( y_data / ( side_cm * side_cm ) ) );
				y_data = arma::abs( y_data / ( side_cm * side_cm ) );
				x_data = arma::abs( x_data );
			}
			break;
		}
		return { toQVec( x_data ), toQVec( y_data ) };
	}

	const static X_Units default_x_units = X_Units::VOLTAGE_V;
	const static Y_Units default_y_units = Y_Units::CURRENT_A;
	X_Units x_units = default_x_units;
	Y_Units y_units = default_y_units;
	std::function<void()> graph_function;

	Axes( std::function<void()> regraph_function ) : graph_function( regraph_function )
	{
	}

	const static QString X_Unit_Names[ 1 ];
	const static QString Y_Unit_Names[ 5 ];

	//const static QString X_Unit_Names[ 4 ] = { "Current (A)",
	//						"Current (mA)",
	//						"Current (" + QString( QChar( 0x03BC ) ) + "A)",
	//						"Current (nA)" };

};

class Interactive_Graph :
	public ::Interactive_Graph<X_Units, Y_Units, Axes>
{
public:
	Interactive_Graph( QWidget *parent = nullptr );

private:
	QSharedPointer<QCPAxisTicker> linearTicker;
	QSharedPointer<QCPAxisTickerLog> logTicker;
};
//class Interactive_Graph :
//	public ::Interactive_Graph
//{
//	Q_OBJECT
//
//
//signals:
//};

//class Interactive_Graph :
//	public QCustomPlot
//{
//	Q_OBJECT
//
//
//signals:
//	void Graph_Selected( QCPGraph* selected_graph );
//
//public:
//	explicit Interactive_Graph( QWidget *parent = 0 );
//	void Initialize_Graph();
//
//	void selectionChanged();
//	void mousePress();
//	void titleDoubleClick( QMouseEvent * event );
//	void axisLabelDoubleClick( QCPAxis * axis, QCPAxis::SelectablePart part, QMouseEvent * event );
//	void legendDoubleClick( QCPLegend * legend, QCPAbstractLegendItem * item, QMouseEvent * event );
//	void mouseWheel();
//	void refitGraphs( QMouseEvent* event = nullptr );
//	void removeSelectedGraph();
//	void removeAllGraphs();
//	void graphContextMenuRequest( QPoint pos );
//	void moveLegend();
//	void graphClicked( QCPAbstractPlottable *plottable, int dataIndex );
//	void saveCurrentGraph();
//	const Single_Graph & Graph( QVector<double> x_data, QVector<double> y_data, QString unique_name, Labeled_Metadata meta, QString graph_title = QString(), bool allow_y_scaling = true );
//	void Hide_Graph( QString graph_name );
//	void mouseDoubleClickEvent( QMouseEvent* event ) override;
//
//	std::tuple< QVector<double>, QVector<double> > Convert_Units( const Single_Graph & graph );
//	void RegraphAll();
//	void UpdateGraph( QCPGraph* existing_graph, QVector<double> x_data, QVector<double> y_data );
//	Single_Graph GetSelectedGraphData() const;
//	Single_Graph FindDataFromGraphPointer( QCPGraph* graph_pointer ) const;
//
//	std::vector< std::function<void( Interactive_Graph*, QMenu*, QPoint )> > y_axis_menu_functions;
//	Unit_Type x_axis_units = Unit_Type::CURRENT_A;
//	Y_Unit_Type y_axis_units = Y_Unit_Type::CURRENT_A;
//	//std::function<double( double )> x_display_method{ []( double x ) { return 10000 / x; } };
//	std::function<double( double, double )> y_display_method{ []( double x, double y ) { return y; } };
//
//private:
//	std::map< QString, Single_Graph > remembered_graphs;
//
//};


}
