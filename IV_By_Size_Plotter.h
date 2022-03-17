#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_IV_By_Size_Plotter.h"
#include "SQL_Manager.h"

#include "Handy_Types_And_Conversions.h"

namespace IV_By_Size
{

class Plotter : public QWidget
{
	Q_OBJECT

public:
	Plotter( QWidget *parent = Q_NULLPTR );
	Ui::IV_By_Size_Plotter ui;
	SQL_Configuration config;
	SQL_Manager_With_Local_Cache* sql_manager;

private:
	void Initialize_SQL( QString config_filename );
	void Initialize_Tree_Table();
	void Initialize_Graph();
	void Initialize_Rule07();
	void Graph_Measurement( ID_To_Metadata selected, QStringList column_names );
	void treeContextMenuRequest( QPoint pos );
	void Graph_Rule07( double dark_current_a_cm2, double temperature_in_k );


	void Update_Preview_Graph();
};

template< typename IV_By_Size_Requestor >
void Graph_IV_By_Size_Raw_Data( IV_By_Size_Requestor* requestor, const SQL_Configuration & config, IV_By_Size::Interactive_Graph* graph, Structured_Metadata metadata, QString plot_title = "" )
{
	const auto & column_names = metadata.column_names;

	int measurement_id_i = column_names.indexOf( "measurement_id" );
	const std::vector<QString> measurement_ids_as_vec = metadata.data
		% fn::transform( [ measurement_id_i ]( const auto & row_of_metadata ) { return row_of_metadata[ measurement_id_i ].toString(); } );
	const QStringList measurement_ids = QStringList::fromVector( QVector<QString>::fromStdVector( measurement_ids_as_vec ) );

	requestor->sql_manager->Grab_SQL_XY_Data_From_Measurement_IDs(
		config.raw_data_columns, config.raw_data_table, measurement_ids, requestor,
		[ graph, metadata = std::move( metadata ), plot_title ]( ID_To_XY_Data data )
	{
		graph->Graph( metadata, data, plot_title );
		graph->replot();
	}, config.sorting_strategy );
}

}

