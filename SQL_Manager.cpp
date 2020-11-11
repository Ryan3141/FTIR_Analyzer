#include "SQL_Manager.h"

#include <QString>
#include <QSettings>
#include <QFileDialog>
#include <QDesktopServices>
#include <QMessageBox>
#include <QDebug>
#include <QtSql>

using namespace std;

void SQL_Manager::Connect_To_DB( QString config_filename )
{
	QSettings settings( config_filename, QSettings::IniFormat, this );

	if( !Initialize_DB_Connection( settings.value( "SQL_Server/database_type" ).toString(),
									settings.value( "SQL_Server/host_location" ).toString(),
									settings.value( "SQL_Server/database_name" ).toString(),
									settings.value( "SQL_Server/username" ).toString(),
									settings.value( "SQL_Server/password" ).toString() ) )
	{
		QMessageBox msgBox;
		msgBox.setText( "Error Opening SQL" );
		msgBox.setInformativeText( "Do you want to open your config file?\n(Restart program to try SQL again)" );
		msgBox.setStandardButtons( QMessageBox::Yes | QMessageBox::No );
		msgBox.setDefaultButton( QMessageBox::Save );
		int ret = msgBox.exec();
		if( ret == QMessageBox::Yes )
		{
			QDesktopServices::openUrl( QUrl( config_filename ) ); // Open config file in default program
		}
	}

	//this->sql_insert_command.prepare( "INSERT INTO " + Sanitize_SQL( identifier_to_table[ this->listener ] )
	//								  + " (location," + headers.join( ',' ) + ") "
	//								  "VALUES (:location," + value_binds.join( ',' ) + ")" );

	//this->sql_insert_command.bindValue( ":location", this->name );

}


bool SQL_Manager::Initialize_DB_Connection( const QString & database_type, const QString & host_location, const QString & database_name, const QString & username, const QString & password )
{
	sql_db = QSqlDatabase::addDatabase( database_type );
	if( database_type == "QMYSQL" )
		sql_db.setConnectOptions( "MYSQL_OPT_RECONNECT=1" );

	if( !host_location.isEmpty() )
		sql_db.setHostName( host_location );
	sql_db.setDatabaseName( database_name );
	if( !username.isEmpty() )
		sql_db.setUserName( username );
	if( !password.isEmpty() )
		sql_db.setPassword( password );

	bool sql_worked = sql_db.open();
	if( sql_worked )
		qInfo() << QString( "Connected to %1 Database %2" ).arg( host_location ).arg( database_name );
	else
	{
		auto problem = sql_db.lastError();
		qCritical() << "Error with SQL Connection: " << problem;
	}

	return sql_worked;
}

SQL_Manager::ID_To_XY_Data SQL_Manager::Grab_SQL_Data_From_Measurement_IDs( const QStringList & what_to_collect, const QString & table_name, const QStringList & measurement_ids, const QString & extra_filtering ) const
{
	// Grab SQL data
	QSqlQuery query( sql_db );
	query.prepare( QString( "SELECT %1 FROM %2 WHERE measurement_id=\"%3\" %4" )
				   .arg( what_to_collect.join( ',' ), table_name, measurement_ids.join( "\" OR measurement_id=\"" ), extra_filtering ) );
	if( !query.exec() )
	{
		qDebug() << "Error pulling data from ftir_measurments: "
			<< query.lastError();
		return ID_To_XY_Data();
	}

	ID_To_XY_Data data_per_id;
	while( query.next() )
	{
		QString measurement_id = query.value( 0 ).toString();
		XY_Data & one_measurement = data_per_id[ measurement_id ];
		std::get<0>( one_measurement ).push_back( query.value( 1 ).toDouble() );
		std::get<1>( one_measurement ).push_back( query.value( 2 ).toDouble() );
	}

	return data_per_id;
}

SQL_Manager::ID_To_Metadata SQL_Manager::Grab_SQL_Metadata_From_Measurement_IDs( const QStringList & what_to_collect, const QString & table_name, const QStringList & measurement_ids, const QString & extra_filtering ) const
{
	int measurement_id_index = what_to_collect.indexOf( "measurement_id" );
	if( measurement_id_index == -1 )
		return ID_To_Metadata();

	QSqlQuery query( this->sql_db );
	//QStringList what_to_collect{ "sample_name", "time", "temperature_in_k", "bias_in_v" };
	query.prepare( QString( "SELECT %1 FROM %2 WHERE measurement_id=\"%3\" %4" )
				   .arg( what_to_collect.join( ',' ), table_name, measurement_ids.join( "\" OR measurement_id=\"" ), extra_filtering ) );
	if( !query.exec() )
	{
		qDebug() << "Error pulling data from ftir_measurments: "
			<< query.lastError();
		return ID_To_Metadata();
	}

	ID_To_Metadata data_per_id;
	while( query.next() )
	{
		QString measurement_id = query.value( measurement_id_index ).toString();
		Metadata & one_measurement = data_per_id[ measurement_id ];
		one_measurement.resize( what_to_collect.size() );
		for( int i = 0; i < what_to_collect.size(); i++ )
			one_measurement[i] = query.value( i );
	}

	return data_per_id;
}
