#pragma once

#include <tuple>
#include <map>
#include <string>

#include <QVector>
#include <QVariant>
#include <QWidget>
#include <QSqlDatabase>

class SQL_Manager : QWidget
{
	Q_OBJECT

signals:
public slots:

public:
	using XY_Data = std::tuple< QVector<double>, QVector<double> >;
	using ID_To_XY_Data = std::map<QString, XY_Data>;
	using Metadata = std::vector<QVariant>;
	using ID_To_Metadata = std::map<QString, Metadata>;

	void Connect_To_DB( QString config_filename );

	ID_To_XY_Data Grab_SQL_Data_From_Measurement_IDs(  const QStringList & what_to_collect, const QString & table_name, const QStringList & measurement_ids, const QString & extra_filtering = "" ) const;
	ID_To_Metadata Grab_SQL_Metadata_From_Measurement_IDs( const QStringList & what_to_collect, const QString & table_name, const QStringList & measurement_ids, const QString & extra_filtering = "" ) const;


	QSqlDatabase sql_db;

private:
	bool Initialize_DB_Connection( const QString & database_type, const QString & host_location, const QString & database_name, const QString & username, const QString & password );

};
