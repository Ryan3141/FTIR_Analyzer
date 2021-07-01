#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_CV_Plotter.h"
#include "SQL_Manager.h"

#include "Handy_Types_And_Conversions.h"

namespace CV
{

class Plotter : public QWidget
{
	Q_OBJECT

public:
	Plotter( QWidget *parent = Q_NULLPTR );
	Ui::CV_Plotter ui;

private:
	void Initialize_SQL( QString config_filename );
	void Initialize_Tree_Table();
	void Initialize_Graph();
	void Graph_Measurement( QString measurement_id, Labeled_Metadata metadata );
	void treeContextMenuRequest( QPoint pos );

	void Update_Preview_Graph();

	int simulated_graph_number = 0;
	SQL_Manager* sql_manager;
	Data_Configuration config;

	using XY_Data = std::tuple< QVector<double>, QVector<double> >;
};

}
