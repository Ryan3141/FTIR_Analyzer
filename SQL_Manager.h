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

class SQL_Manager : public QObject
{
	Q_OBJECT

public:
	using XY_Data = std::tuple< QVector<double>, QVector<double> >;
	using ID_To_XY_Data = std::map<QString, XY_Data>;
	using Metadata = std::vector<QVariant>;
	using ID_To_Metadata = std::map<QString, Metadata>;

signals:
	void Error_Connecting_To_SQL( const QSqlError & error_message );
	void SQL_XY_Data_Ready( ID_To_XY_Data data_per_id );
	void SQL_Meta_Data_Ready( ID_To_Metadata data_per_id );

public slots:
	void Start_Thread();

public:
	SQL_Manager( QObject* parent, QFileInfo config_filename );


	void Grab_SQL_XY_Data_From_Measurement_IDs(  const QStringList & what_to_collect, const QString & table_name, const QStringList & measurement_ids, QObject* callback_context,
												 std::function< void( ID_To_XY_Data ) > callback, const QString & extra_filtering = "" ) const;
	void Grab_SQL_Metadata_From_Measurement_IDs( const QStringList & what_to_collect, const QString & table_name, const QStringList & measurement_ids, QObject* callback_context,
												 std::function< void( ID_To_Metadata ) > callback, const QString & extra_filtering = "" ) const;
	void Grab_All_SQL_Metadata(                  const QStringList & what_to_collect, const QString & table_name, QObject* context,
												 std::function< void( std::vector<Metadata> ) > callback, const QString & extra_filtering = "" ) const;

private:
	bool Initialize_DB_Connection();
	QSqlDatabase sql_db;
	QFileInfo config_filename_;
	QThread* worker_thread = nullptr;
};
