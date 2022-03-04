#pragma once

#include <QWidget>
#include "ui_Report_Plots.h"
#include "SQL_Manager.h"

#include "Handy_Types_And_Conversions.h"

namespace Report
{

class Report_Plots : public QWidget
{
	Q_OBJECT

public:
	Report_Plots(QWidget *parent = Q_NULLPTR);
	Data_Configuration config;
	SQL_Manager_With_Local_Cache* sql_manager;

private:
	void Initialize_SQL( QString config_filename );
	void Initialize_Graph();
	void Load_Report( QFileInfo file );

	Ui::Report_Plots ui;
};

}
