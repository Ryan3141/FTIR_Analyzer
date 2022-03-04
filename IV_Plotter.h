#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_IV_Plotter.h"
#include "SQL_Manager.h"

#include "Handy_Types_And_Conversions.h"

namespace IV
{

class Plotter : public QWidget
{
	Q_OBJECT

public:
	Plotter( QWidget *parent = Q_NULLPTR );
	Ui::IV_Plotter ui;

private:
	void Initialize_SQL( QString config_filename );
	void Initialize_Tree_Table();
	void Initialize_Graph();
	void Initialize_Rule07();
	void Graph_Measurement( QString measurement_id, Labeled_Metadata metadata );
	void treeContextMenuRequest( QPoint pos );
	void Graph_Rule07( double dark_current_a_cm2, double temperature_in_k );
	void Save_To_CSV( const ID_To_Metadata & things_to_save );


	void Update_Preview_Graph();

	int simulated_graph_number = 0;
	SQL_Manager_With_Local_Cache* sql_manager;
	Data_Configuration config;
};

}
