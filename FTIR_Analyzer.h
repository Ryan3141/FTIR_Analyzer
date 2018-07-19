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
	void showPointToolTip( QMouseEvent * event );
	void treeContextMenuRequest( QPoint pos );
	bool Initialize_DB_Connection( const QString & database_type, const QString & host_location, const QString & database_name, const QString & username, const QString & password );
	void Initialize_SQL();

	void Recursive_Tree_Table_Build( const QStringList & what_to_collect, QTreeWidgetItem* parent_tree, int current_collectable_i, QStringList filters );

	void Initialize_Tree_Table();
	void Reinitialize_Tree_Table();
	void Initialize_Graph();

	void Graph_Tree_Node( const QTreeWidgetItem* tree_item );

	std::vector<const QTreeWidgetItem*> Get_Bottom_Children_Elements_Under( const QTreeWidgetItem * tree_item ) const;

	void Grab_SQL_Data_From_Measurement_ID( QString measurement_id, QVector<double>& x_data, QVector<double>& y_data );
	void Graph( QString measurement_id, QVector<double> x_data, QVector<double> y_data );

	QSqlDatabase sql_db;
	QLabel* statusLabel;

	std::function<double( double )> x_display_method{ []( double x ) { return 10000 / x; } };
	std::function<double( double, double )> y_display_method{ []( double x, double y ) { return y; } };
};
