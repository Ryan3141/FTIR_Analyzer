#pragma once

#include <QWidget>
#include "ui_Report_Plots.h"
#include "SQL_Manager.h"

#include "Handy_Types_And_Conversions.h"

namespace Report
{

using Data_Is_Returned_Func = std::function< void( Structured_Metadata, ID_To_XY_Data ) >;
using Request_Data_Func = std::function< void( QStringList, Data_Is_Returned_Func ) >;

class Report_Plots : public QWidget
{
	Q_OBJECT

public:
	Report_Plots(QWidget *parent = Q_NULLPTR);
	void Get_SQL_Data( const SQL_Configuration & config, QStringList measurement_ids, Data_Is_Returned_Func run_on_data );
		
	SQL_Configuration iv_sql_config;
	SQL_Configuration ftir_sql_config;
	SQL_Configuration iv_by_size_sql_config;
	SQL_Configuration spectral_response_sql_config;
	SQL_Configuration lifetime_sql_config;
	SQL_Manager_With_Local_Cache* sql_manager;

private:
	void Initialize_SQL( QString config_filename );
	void Initialize_Graph();
	void Load_Report( QFileInfo file );

public:
	Ui::Report_Plots ui;
};

}
