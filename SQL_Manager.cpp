#include "SQL_Manager.h"

#include <QString>
#include <QSettings>
#include <QFileDialog>
#include <QDesktopServices>
#include <QMessageBox>
#include <QDebug>
#include <QtSql>

using namespace std;


static QString Sanitize_SQL( const QString & raw_string )
{ // Got regex from https://stackoverflow.com/questions/9651582/sanitize-table-column-name-in-dynamic-sql-in-net-prevent-sql-injection-attack
	if( raw_string.contains( ';' ) )
		return QString();

	QRegularExpression re( R"(^[\p{L}{\p{Nd}}$#_][\p{L}{\p{Nd}}@$#_]*$)" );
	QRegularExpressionMatch match = re.match( raw_string );
	bool hasMatch = match.hasMatch();

	if( hasMatch )
		return QString();
	else
		return raw_string;
}

SQL_Manager::SQL_Manager( QObject* parent, QFileInfo config_filename, QString unique_name ) :
	config_filename_( config_filename ),
	unique_name( unique_name )
{
}

void SQL_Manager::Start_Thread()
{
	worker_thread = new QThread;
	connect( worker_thread, &QThread::started, this, &SQL_Manager::Initialize_DB_Connection );
	//QTimer::singleShot( 0, this, &SQL_Manager::Initialize_DB_Connection );

	// Startup behavior communication
	//connect( sql_manager, &SQL_Worker::Error_Connecting_To_SQL, thread, &QThread::quit );
	//connect( sql_manager, &SQL_Worker::Error_Connecting_To_SQL, thread, &QThread::quit );

	//automatically delete thread and task object when work is done:
	connect( worker_thread, &QThread::finished, this, &QObject::deleteLater );
	connect( worker_thread, &QThread::finished, worker_thread, &QObject::deleteLater );

	this->moveToThread( worker_thread );
	worker_thread->start();
}

bool SQL_Manager::Initialize_DB_Connection()
{
	QSettings settings( config_filename_.filePath(), QSettings::IniFormat, this );
	const QString database_type = settings.value( "SQL_Server/database_type" ).toString();
	const QString host_location = settings.value( "SQL_Server/host_location" ).toString();
	const QString database_name = settings.value( "SQL_Server/database_name" ).toString();
	const QString username      = settings.value( "SQL_Server/username" ).toString();
	const QString password      = settings.value( "SQL_Server/password" ).toString();

	sql_db = QSqlDatabase::addDatabase( database_type, this->unique_name );
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
		const QSqlError & problem = sql_db.lastError();
		qCritical() << "Error with SQL Connection: " << problem;
		emit Error_Connecting_To_SQL( std::move( problem ) );
	}

	return sql_worked;
}

void SQL_Manager::Grab_SQL_XY_Data_From_Measurement_IDs( const QStringList & what_to_collect, const QString & table_name, const QStringList & measurement_ids, QObject* context,
														 std::function< void( ID_To_XY_Data ) > callback, const QString & extra_filtering ) const
{
	QTimer::singleShot( 0, this, [ this, what_to_collect, table_name, measurement_ids, context, callback, extra_filtering ]
	{
		int measurement_id_index = what_to_collect.indexOf( "measurement_id" );
		if( measurement_id_index == -1 )
			return;

		QSqlQuery query( sql_db );
		query.prepare( QString( "SELECT %1 FROM %2 WHERE measurement_id=\"%3\" %4" )
					   .arg( what_to_collect.join( ',' ), table_name, measurement_ids.join( "\" OR measurement_id=\"" ), extra_filtering ) );
		if( !query.exec() )
		{
			qDebug() << "Error pulling data from ftir_measurments: "
				<< query.lastError();
			return;
		}

		ID_To_XY_Data data_per_id;
		while( query.next() )
		{
			QString measurement_id = query.value( measurement_id_index ).toString();
			XY_Data & one_measurement = data_per_id[ measurement_id ];
			std::get<0>( one_measurement ).push_back( query.value( 1 ).toDouble() );
			std::get<1>( one_measurement ).push_back( query.value( 2 ).toDouble() );
		}

		QTimer::singleShot( 0, context, [ callback, data_per_id = std::move(data_per_id) ]
		{
			callback( std::move( data_per_id ) );
		} );
	} );
}

void SQL_Manager::Grab_SQL_Metadata_From_Measurement_IDs( const QStringList & what_to_collect, const QString & table_name, const QStringList & measurement_ids, QObject* callback_context,
														  std::function< void( ID_To_Metadata ) > callback, const QString & extra_filtering ) const
{
	QTimer::singleShot( 0, this, [ this, what_to_collect, table_name, measurement_ids, callback_context, callback, extra_filtering ]
	{
		int measurement_id_index = what_to_collect.indexOf( "measurement_id" );
		if( measurement_id_index == -1 )
			return;

		QSqlQuery query( this->sql_db );
		//QStringList what_to_collect{ "sample_name", "time", "temperature_in_k", "bias_in_v" };
		query.prepare( QString( "SELECT %1 FROM %2 WHERE measurement_id=\"%3\" %4" )
						.arg( what_to_collect.join( ',' ), table_name, measurement_ids.join( "\" OR measurement_id=\"" ), extra_filtering ) );
		if( !query.exec() )
		{
			qDebug() << "Error pulling data from " << table_name << ":\n"
				<< query.lastError();
			return;
		}

		ID_To_Metadata data_per_id;
		while( query.next() )
		{
			QString measurement_id = query.value( measurement_id_index ).toString();
			Metadata & one_measurement = data_per_id[ measurement_id ];
			one_measurement.resize( what_to_collect.size() );
			for( int i = 0; i < what_to_collect.size(); i++ )
				one_measurement[ i ] = query.value( i );
		}

		QTimer::singleShot( 0, callback_context, [ callback, data_per_id = std::move( data_per_id ) ]
		{
			callback( std::move( data_per_id ) );
		} );
	} );
}

void SQL_Manager::Grab_All_SQL_Metadata( const QStringList & what_to_collect, const QString & table_name, QObject* callback_context,
										 std::function< void( Structured_Metadata ) > callback, const QString & extra_filtering ) const
{
	QTimer::singleShot( 0, this, [ this, what_to_collect, table_name, callback_context, callback, extra_filtering ]
	{
		QSqlQuery query( this->sql_db );
		QString query_string = QString( "SELECT %1 FROM %2" ).arg( what_to_collect.join( "," ), table_name );
		if( !extra_filtering.isEmpty() )
			query_string += Sanitize_SQL( extra_filtering );

		query.prepare( query_string );
		if( !query.exec() )
		{
			qDebug() << "Error pulling data from " << table_name << ": "
				<< query.lastError();
			return;
		}

		std::vector<Metadata> all_meta_data;
		int number_of_rows = 0;
		if( query.last() ) // Count the number of rows returned
		{
			number_of_rows = query.at() + 1;
			all_meta_data.resize( number_of_rows );
			query.first();
			query.previous();
		}

		int number_of_columns = query.record().count();
		for( int row_i = 0; row_i < number_of_rows; row_i++ )
		{
			query.next();
			for( int col_i = 0; col_i < number_of_columns; col_i++ )
			{
				QVariant current_value = query.value( col_i );
				all_meta_data[ row_i ].push_back( current_value );
			}
		}

		//QTimer* timer = new QTimer();
		////timer->moveToThread( qApp->thread() );
		//timer->moveToThread( callback_context->thread() );
		//timer->setSingleShot( true );
		//QObject::connect( timer, &QTimer::timeout, [ callback, all_meta_data = std::move( all_meta_data ) ]()
		//{
		QTimer::singleShot( 0, callback_context, [ callback, what_to_collect, all_meta_data = std::move( all_meta_data ) ]
		{
			callback( { what_to_collect, std::move( all_meta_data ) } );
		} );

		//timer->deleteLater();
	} );

	//QMetaObject::invokeMethod( timer, "start", Qt::QueuedConnection, Q_ARG( int, 0 ) );
}

