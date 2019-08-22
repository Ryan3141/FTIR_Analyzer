#pragma once

#include <QtWidgets/QMainWindow>
#include <QtSql>
#include <vector>
#include <tuple>
#include <map>

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
	using XY_Data = std::tuple< QVector<double>, QVector<double> >;
	using ID_To_XY_Data = std::map<QString, XY_Data>;
	//FTIR_Analyzer & self{ *this };
	Ui::FTIR_AnalyzerClass ui;
	void showPointToolTip( QMouseEvent * event );
	void treeContextMenuRequest( QPoint pos );
	bool Initialize_DB_Connection( const QString & database_type, const QString & host_location, const QString & database_name, const QString & username, const QString & password );
	void Initialize_SQL();


	void Initialize_Tree_Table();
	void Initialize_Graph();

	void Graph_Tree_Node( const QTreeWidgetItem* tree_item );
	void Graph_Simulation( const std::vector<Material_Layer> & layers );

	void Grab_SQL_Data_From_Measurement_IDs( const QStringList & measurement_ids, ID_To_XY_Data & data_per_id );
	std::map<QString, QString> Grab_SQL_Metadata_From_Measurement( const QString & measurement_id ) const;
	void Graph( QString measurement_id, const QVector<double> & x_data, const QVector<double> & y_data, QString data_title = QString(), bool allow_y_scaling = true );
	void Run_Fit();

	void FTIR_Analyzer::Load_From_SPA();
	void Save_To_CSV( const std::vector<const QTreeWidgetItem*> & things_to_save );


	QSqlDatabase sql_db;
	QLabel* statusLabel;
};
