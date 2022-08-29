#pragma once

#include <tuple>
#include <map>
#include <string>
#include <vector>

#include <QVector>
#include <QVariant>
#include <QWidget>
#include <QSqlDatabase>
#include <QFileInfo>
#include <QThread>

using XY_Data = std::tuple< QVector<double>, QVector<double> >;
using ID_To_XY_Data = std::map<QString, XY_Data>;
using Metadata = std::vector<QVariant>;
using ID_To_Metadata = std::map<QString, Metadata>;


struct Structured_Metadata
{
	QStringList column_names;
	std::vector<Metadata> data;
};

class SQL_Manager : public QObject
{
	Q_OBJECT

public:

signals:
	void Error_Connecting_To_SQL( const QSqlError & error_message );
	void SQL_XY_Data_Ready( ID_To_XY_Data data_per_id );
	void SQL_Meta_Data_Ready( ID_To_Metadata data_per_id );

public slots:
	void Start_Thread();

public:
	SQL_Manager( QObject* parent, QFileInfo config_filename, QString unique_name, QString config_category = "SQL_Server" );


	void Grab_SQL_XY_Blob_Data_From_Measurement_IDs( const QStringList & what_to_collect, const QString & table_name, const QStringList & measurement_ids, QObject* callback_context,
												 std::function< void( ID_To_XY_Data ) > callback, const QString & extra_filtering = "" ) const;
	void Grab_SQL_XY_Data_From_Measurement_IDs(  const QStringList & what_to_collect, const QString & table_name, const QStringList & measurement_ids, QObject* callback_context,
												 std::function< void( ID_To_XY_Data ) > callback, const QString & extra_filtering = "" ) const;
	void Grab_SQL_Metadata_From_Measurement_IDs( const QStringList & what_to_collect, const QString & table_name, const QStringList & measurement_ids, QObject* callback_context,
												 std::function< void( ID_To_Metadata ) > callback, const QString & extra_filtering = "" ) const;
	void Grab_All_SQL_Metadata(                  const QStringList & what_to_collect, const QString & table_name, QObject* context,
												 std::function< void( Structured_Metadata ) > callback, const QString & extra_filtering = "" ) const;

	void Write_SQL_Metadata( const ID_To_Metadata & meta_data, const QStringList & what_to_write, const QString & table_name,
							 QObject* callback_context = nullptr, std::function< void() > callback = {} );
	void Write_SQL_XY_Blob_Data( const ID_To_XY_Data & data, const QStringList & what_to_write, const QString & table_name,
							QObject* callback_context = nullptr, std::function< void() > callback = {} );
	void Write_SQL_XY_Data( const ID_To_XY_Data & data, const QStringList & what_to_write, const QString & table_name,
							QObject* callback_context = nullptr, std::function< void() > callback = {} );
	void Write_SQL_XY_Data_No_Duplicates( const ID_To_XY_Data & data, const QStringList & what_to_write, const QString & table_name,
										  QObject* callback_context = nullptr, std::function< void() > callback = {} );


private:
	bool Initialize_DB_Connection();
	QSqlDatabase sql_db;
	QThread* worker_thread = nullptr;
	QString unique_name;

	QString database_type;
	QString host_location;
	QString database_name;
	QString username     ;
	QString password     ;

};

class SQL_Manager_With_Local_Cache : public QObject
{
	Q_OBJECT

public:

signals:
	void Error_Connecting_To_SQL( const QSqlError & error_message );
	void SQL_XY_Data_Ready( ID_To_XY_Data data_per_id );
	void SQL_Meta_Data_Ready( ID_To_Metadata data_per_id );

public slots:
	void Start_Thread();

public:
	SQL_Manager_With_Local_Cache( QObject* parent, QFileInfo config_filename, QString unique_name, QString config_category = "SQL_Server" );

	void Grab_SQL_XY_Blob_Data_From_Measurement_IDs( const QStringList & what_to_collect, const QString & table_name, const QStringList & measurement_ids, QObject* callback_context,
												std::function< void( ID_To_XY_Data ) > callback, const QString & extra_filtering = "" );
	void Grab_SQL_XY_Data_From_Measurement_IDs( const QStringList & what_to_collect, const QString & table_name, const QStringList & measurement_ids, QObject* callback_context,
												std::function< void( ID_To_XY_Data ) > callback, const QString & extra_filtering = "" );
	void Grab_SQL_Metadata_From_Measurement_IDs( const QStringList & what_to_collect, const QString & table_name, const QStringList & measurement_ids, QObject* callback_context,
												 std::function< void( ID_To_Metadata ) > callback, const QString & extra_filtering = "" );
	void Grab_All_SQL_Metadata( const QStringList & what_to_collect, const QString & table_name, QObject* context,
								std::function< void( Structured_Metadata ) > callback, const QString & extra_filtering = "" );

private:
	SQL_Manager remote_sql;
	SQL_Manager local_sql;
};

//class SQL_Buffered_Manager
//{
//	Q_OBJECT
//
//public:
//	SQL_Buffered_Manager( QObject* parent, QFileInfo config_filename );
//
//signals:
//	void Error_Connecting_To_SQL( const QSqlError & error_message );
//	void SQL_XY_Data_Ready(   SQL_Manager::ID_To_XY_Data data_per_id );
//	void SQL_Meta_Data_Ready( SQL_Manager::ID_To_Metadata data_per_id );
//
//public slots:
//	void Start_Thread();
//
//private:
//	SQL_Manager remote_db;
//	SQL_Manager local_db
//};
