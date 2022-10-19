#pragma once

#include <QWidget>
#include <vector>
#include <tuple>
#include <map>

#include "ui_Lifetime_Plotter.h"

#include "Handy_Types_And_Conversions.h"

#include "SQL_Manager.h"
#include "Lifetime_Interactive_Graph.h"

namespace Lifetime
{

class Plotter : public QWidget
{
	Q_OBJECT

signals:

public slots:

public:
	Plotter( QWidget *parent = Q_NULLPTR );
	Ui::Lifetime_WidgetClass ui;

private:
	using XY_Data = std::tuple< QVector<double>, QVector<double> >;
	using ID_To_XY_Data = std::map<QString, XY_Data>;
	//FTIR_Analyzer & self{ *this };
	void treeContextMenuRequest( QPoint pos );

	void Initialize_Tree_Table();
	void Initialize_Graph();
	void Initialize_Theoretical_Plots();
	void Initialize_SQL( QString config_filename );

	void Graph_Data( const ID_To_Metadata & selected_data );

	QLabel* statusLabel;
	SQL_Manager_With_Local_Cache* sql_manager;
	SQL_Configuration config;
};

void Graph_Measurement( ID_To_XY_Data data, Interactive_Graph* graph, QString measurement_id, Labeled_Metadata metadata, QString legend_label = "" );
}
