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

private:
	void Initialize_SQL( QString config_filename );
	void Initialize_Graph();
	void Load_Report( QFileInfo file );

	SQL_Manager* sql_manager;
	Data_Configuration config;

	Ui::Report_Plots ui;
};

}
