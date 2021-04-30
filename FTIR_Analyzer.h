#pragma once

#include <QtWidgets/QMainWindow>
#include <vector>
#include <tuple>
#include <map>

#include "ui_FTIR_Analyzer.h"

#include "SQL_Manager.h"

//Q_DECLARE_METATYPE( std::tuple<bool, bool, bool> )

namespace FTIR
{

struct Data_Configuration
{
	QStringList header_titles;
	QStringList what_to_collect;
	QString sql_table;
	QStringList raw_data_columns;
	QString raw_data_table;
	QString sorting_strategy;
	int columns_to_show;
};

class FTIR_Analyzer : public QMainWindow
{
	Q_OBJECT

signals:

public slots:

public:
	FTIR_Analyzer( QWidget *parent = Q_NULLPTR );

private:
	using XY_Data = std::tuple< QVector<double>, QVector<double> >;
	using ID_To_XY_Data = std::map<QString, XY_Data>;
	//FTIR_Analyzer & self{ *this };
	Ui::FTIR_AnalyzerClass ui;
	void treeContextMenuRequest( QPoint pos );

	Material_Layer Get_Backside_Material( double temperature_in_k );

	void Initialize_Tree_Table();
	void Initialize_Graph();
	void Initialize_Simulation();
	void Initialize_SQL( QString config_filename );

	void Graph_Measurement( QString measurement_id, Metadata metadata );
	void Graph_Simulation( std::vector<Material_Layer> layers, std::tuple<bool, bool, bool> what_to_plot, double largest_transmission = 100.0, Material_Layer backside_material = Material_Layer() );
	void Graph_Blackbody( double temperature_in_k, double amplitude );
	void Graph_Refractive_Index( std::string material_name, Optional_Material_Parameters parameters );

	//void Graph( QString measurement_id, const QVector<double> & x_data, const QVector<double> & y_data, QString data_title = QString(), bool allow_y_scaling = true, Metadata meta = Metadata() );
	void Run_Fit();

	void Save_To_CSV( const ID_To_Metadata & things_to_save );
	void Add_Mouse_Position_Label();

	Data_Configuration config;
	QLabel* statusLabel;
	Thin_Film_Interference* thin_film_manager;
	SQL_Manager* sql_manager;
};

}