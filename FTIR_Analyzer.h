#pragma once

#include <QtWidgets/QMainWindow>
#include <QtSql>
#include <vector>

#include "ui_FTIR_Analyzer.h"

class FTIR_Analyzer : public QMainWindow
{
	Q_OBJECT

		signals :

	public slots :
		void Regraph_All_Plots();

public:
	FTIR_Analyzer( QWidget *parent = Q_NULLPTR );

private:
	FTIR_Analyzer & self{ *this };
	Ui::FTIR_AnalyzerClass ui;
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
	void treeContextMenuRequest( QPoint pos );
	void moveLegend();
	void graphClicked( QCPAbstractPlottable *plottable, int dataIndex );

	bool Initialize_DB_Connection( const QString & database_type, const QString & host_location, const QString & database_name, const QString & username, const QString & password );
	void Initialize_SQL();

	void Recursive_Tree_Table_Build( const QStringList & what_to_collect, QTreeWidgetItem* parent_tree, int current_collectable_i, QStringList filters );

	void Initialize_Tree_Table();

	void Graph_Tree_Node( const QTreeWidgetItem* tree_item );

	std::vector<const QTreeWidgetItem*> Get_All_Children_Full_Tree_Elements_Under( const QTreeWidgetItem * tree_item ) const;
	std::vector<const QTreeWidgetItem*> Recursive_Get_All_Children_Full_Tree_Elements_Under( const QTreeWidgetItem * tree_item ) const;

	void Initialize_Table();

	void Grab_SQL_Data_From_Measurement_ID( QString measurement_id, QVector<double>& x_data, QVector<double>& y_data );

	void saveCurrentGraph();
	struct GraphInfo
	{
		QString graph_title;
		QString measurement_id;
	};
	void Graph_Row( int row );
	void Graph( GraphInfo meta_data, QVector<double> x_data, QVector<double> y_data );

	std::map<QCPGraph*, GraphInfo> active_graphs;

	std::function<double( double )> x_display_method{ []( double x ) { return 10000 / x; } };
	std::function<double( double )> y_display_method{ []( double y ) { return y; } };
	QSqlDatabase sql_db;
};
