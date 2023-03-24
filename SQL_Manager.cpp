#include "SQL_Manager.h"

#include <algorithm>
#include <set>
#include <iterator>
#include "base64pp.h"

#include <QString>
#include <QSettings>
#include <QFileDialog>
#include <QDesktopServices>
#include <QMessageBox>
#include <QDebug>
#include <QtSql>

#include "rangeless_helper.hpp"

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

static int unique_number = 0;
SQL_Manager::SQL_Manager( QObject* parent, QFileInfo config_filename, QString unique_name, QString config_category ) :
	unique_name( unique_name + QString::number( unique_number++ ) )
{
	QSettings settings( config_filename.filePath(), QSettings::IniFormat, this );
	database_type = settings.value( config_category + "/database_type" ).toString();
	host_location = settings.value( config_category + "/host_location" ).toString();
	database_name = settings.value( config_category + "/database_name" ).toString();
	username      = settings.value( config_category + "/username" ).toString();
	password      = settings.value( config_category + "/password" ).toString();
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
	sql_db = QSqlDatabase::addDatabase( database_type, this->unique_name );
	if( database_type == "QMYSQL" )
	{
		// std::cout << "Using MySQL" << std::endl;
		// sql_db.setConnectOptions( "MYSQL_OPT_RECONNECT=1" );
		// sql_db.setConnectOptions( "CLIENT_SSL=1" );
		sql_db.setConnectOptions( "MYSQL_OPT_RECONNECT=1;CLIENT_SSL=1" );
		// sql_db.setConnectOptions( "MYSQL_OPT_SSL_VERIFY_SERVER_CERT=false;MYSQL_OPT_RECONNECT=1" );
		// sql_db.setConnectOptions( "CLIENT_SSL=1;MYSQL_OPT_RECONNECT=1" );
		// sql_db.setConnectOptions( "SSL_CA=certs/ca-cert.pem;MYSQL_OPT_RECONNECT=1" );
		// sql_db.setConnectOptions( "SSL_KEY=certs/client-key.pem;SSL_CERT=certs/client-cert.pem;SSL_CA=certs/ca-cert.pem;MYSQL_OPT_RECONNECT=1" );
	}

	if( !host_location.isEmpty() )
		sql_db.setHostName( host_location );
	sql_db.setDatabaseName( database_name );
	if( !username.isEmpty() )
		sql_db.setUserName( username );
	if( !password.isEmpty() )
		sql_db.setPassword( password );

	bool sql_worked = sql_db.open();
	if( sql_worked )
	{
		qInfo() << QString( "Connected to %1 Database %2" ).arg( host_location ).arg( database_name );
		emit Database_Opened();
	}
	else
	{
		const QSqlError & problem = sql_db.lastError();
		qCritical() << "Error with SQL Connection: " << problem;
		emit Error_Connecting_To_SQL( std::move( problem ) );
		QTimer::singleShot( 10000, this, &SQL_Manager::Initialize_DB_Connection ); // Try again in 10 seconds if it failed
	}

	return sql_worked;
}

void SQL_Manager::Grab_SQL_XY_Blob_Data_From_Measurement_IDs( const QStringList & what_to_collect, const QString & table_name, const QStringList & measurement_ids, QObject* context,
														 std::function< void( ID_To_XY_Data ) > callback, const QString & extra_filtering ) const
{
	QTimer::singleShot( 0, this, [ this, what_to_collect, table_name, measurement_ids, context, callback, extra_filtering ]
	{
		int measurement_id_index = what_to_collect.indexOf( "measurement_id" );
		if( measurement_id_index == -1 )
			return;
		QSqlQuery query( sql_db );
		query.setForwardOnly( true );
		query.prepare( QString( "SELECT %1 FROM %2 WHERE measurement_id IN (%3) %4" )
					   .arg( what_to_collect.join( ',' ), table_name, measurement_ids.join( "," ), Sanitize_SQL(extra_filtering) ) );
		if( !query.exec() )
		{
			qDebug() << "Error pulling data from sql table: "
				<< query.lastError();
			return;
		}
		QString debug = query.executedQuery();

		ID_To_XY_Data data_per_id;
		while( query.next() )
		{
			QString measurement_id = query.value( measurement_id_index ).toString();
			XY_Data & one_measurement = data_per_id[ measurement_id ];
			//QByteArray ba1 = query.value( 1 ).toByteArray();
			//QByteArray ba2 = query.value( 2 ).toByteArray();
			std::string x64_data = query.value( 1 ).toString().toStdString();
			std::string y64_data = query.value( 2 ).toString().toStdString();
			auto ba1 = base64pp::decode( x64_data ).value();
			auto ba2 = base64pp::decode( y64_data ).value();
			auto &[ x_data, y_data ] = one_measurement;
			x_data.resize( ba1.size() / sizeof( decltype(+x_data[0]) ) + 1 ); // Oversize a bit to avoid potential memory overrun
			y_data.resize( ba2.size() / sizeof( decltype(+y_data[0]) ) + 1 ); // Oversize a bit to avoid potential memory overrun
			std::copy( ba1.begin(), ba1.end(), reinterpret_cast<char*>( x_data.data() ) );
			std::copy( ba2.begin(), ba2.end(), reinterpret_cast<char*>( y_data.data() ) );
			x_data.resize( x_data.size() - 1 ); // Undo previous oversize
			y_data.resize( y_data.size() - 1 );	// Undo previous oversize
		}

		QTimer::singleShot( 0, context, [ callback, data_per_id = std::move(data_per_id) ]
		{
			callback( std::move( data_per_id ) );
		} );
	} );
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
					   .arg( what_to_collect.join( ',' ), table_name, measurement_ids.join( "\" OR measurement_id=\"" ), Sanitize_SQL(extra_filtering) ) );
		if( !query.exec() )
		{
			qDebug() << "Error pulling data from sql table: "
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
						.arg( what_to_collect.join( ',' ), table_name, measurement_ids.join( "\" OR measurement_id=\"" ), Sanitize_SQL(extra_filtering) ) );
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

		QTimer::singleShot( 0, callback_context, [ callback, what_to_collect, all_meta_data = std::move( all_meta_data ) ]
		{
			callback( { what_to_collect, std::move( all_meta_data ) } );
		} );
	} );
}

void SQL_Manager::Write_SQL_Metadata( const ID_To_Metadata & meta_data, const QStringList & what_to_write, const QString & table_name,
									  QObject* callback_context, std::function< void() > callback )
{
	if( meta_data.empty() )
		return;
	QTimer::singleShot( 0, this, [ this, meta_data, what_to_write, table_name, callback_context, callback ]
	{
		QSqlQuery query( this->sql_db );
		std::vector<QVariantList> data_to_write = what_to_write
			% fn::transform( fn::get::enumerated{} )
			% fn::transform( [ &meta_data ]( const auto & thing_to_write ) -> QVariantList
				{
					const auto &[ index, header ] = thing_to_write;
					QVariantList one_column;
					for( const auto & [measurement_id, row] : meta_data )
						one_column.push_back( row[ index ] );
					return one_column;
				} );
		QStringList dont_put_duplicates = what_to_write
			% fn::transform( []( const QString & s ) { return QString( s + "=?" ); } )
			% fn::to( QStringList{} );

		std::vector<QString> lots_of_qs( what_to_write.size(), "?" );
		QString question_marks = QStringList::fromVector( QVector<QString>(lots_of_qs.begin(), lots_of_qs.end()) ).join( ',' );
		query.prepare( QString( "INSERT INTO %1 (%2) SELECT %3 EXCEPT SELECT %2 FROM %1 WHERE %4;" )
					   .arg( table_name,
							 what_to_write.join( ',' ),
							 question_marks,
							 dont_put_duplicates.join( " AND " ) ) );
		// Bind values twice, once for insert values, once to filter out duplicates
		for( int i = 0; i < 2; i++ )
			for( const auto & one_column : data_to_write )
				query.addBindValue( one_column );

		if( !query.execBatch() )
		{
			qDebug() << "Error writing data to " << table_name << ":\n"
				<< query.lastError();
			return;
		}

		if( callback_context != nullptr )
			QTimer::singleShot( 0, callback_context, callback );
	} );
}
void SQL_Manager::Write_SQL_XY_Data_No_Duplicates( const ID_To_XY_Data & data, const QStringList & what_to_write, const QString & table_name,
									 QObject* callback_context, std::function< void() > callback )
{
	if( data.empty() )
		return;
	QTimer::singleShot( 0, this, [ this, data, what_to_write, table_name, callback_context, callback ]
	{
		QSqlQuery query( this->sql_db );
		// Write each set of xy_data one measurement_id at a time
		for( const auto &[ measurement_id, xy_data ] : data )
		{
			QStringList dont_put_duplicates = what_to_write
				% fn::transform( []( const QString & s ) { return QString( s + "=?" ); } )
				% fn::to( QStringList{} );

			std::vector<QString> lots_of_qs( what_to_write.size(), "?" );
			QString question_marks = QStringList::fromVector( QVector<QString>(lots_of_qs.begin(), lots_of_qs.end()) ).join( ',' );
			query.prepare( QString( "INSERT INTO %1 (%2) SELECT %3 EXCEPT SELECT %2 FROM %1 WHERE %4;" )
						   .arg( table_name,
								 what_to_write.join( ',' ),
								 question_marks,
								 dont_put_duplicates.join( " AND " ) ) );
			auto to_variant_list = []( const auto & xy_data ) -> std::tuple<QVariantList, QVariantList>
			{
				QVariantList output1;
				for( const auto & x : std::get<0>( xy_data ) )
					output1.push_back( x );
				QVariantList output2;
				for( const auto & y : std::get<1>( xy_data ) )
					output2.push_back( y );
				return { std::move( output1 ), std::move( output2 ) };
			};
			const auto [ x_data, y_data ] = to_variant_list( xy_data );
			std::vector<QVariant> lots_of_ids( x_data.size(), measurement_id.toInt() );
			QVariantList measurement_id_copies = QVariantList::fromVector( QVector<QVariant>(lots_of_qs.begin(), lots_of_qs.end()) );

			for( int i = 0; i < 2; i++ )
			{
				query.addBindValue( measurement_id_copies );
				query.addBindValue( x_data );
				query.addBindValue( y_data );
			}

			if( !query.execBatch() )
			{
				qDebug() << "Error writing data to " << table_name << ":\n"
					<< query.lastError();
			}
			else
			{
				qDebug() << "Successfully wrote data to " << table_name << ":\n"
					<< query.executedQuery();
			}
		}

		if( callback_context != nullptr )
			QTimer::singleShot( 0, callback_context, callback );
	} );
}

void SQL_Manager::Write_SQL_XY_Blob_Data( const ID_To_XY_Data & data, const QStringList & what_to_write, const QString & table_name,
									 QObject* callback_context, std::function< void() > callback )
{
	if( data.empty() )
		return;
	QTimer::singleShot( 0, this, [ this, data, what_to_write, table_name, callback_context, callback ]
	{
		QSqlQuery query( this->sql_db );
		// Write each set of xy_data one measurement_id at a time
		for( const auto &[ measurement_id, xy_data ] : data )
		{
			std::vector<QString> lots_of_qs( what_to_write.size(), "?" );
			QString question_marks = QStringList::fromVector( QVector<QString>(lots_of_qs.begin(), lots_of_qs.end()) ).join( ',' );
			query.prepare( QString( "INSERT INTO %1 (%2) SELECT %3;" )
						   .arg( table_name,
								 what_to_write.join( ',' ),
								 question_marks ) );
			const auto & [ x_data, y_data ] = xy_data;
			QByteArray encoded_x = base64pp::encode<QVector<double>, QByteArray>( x_data );
			QByteArray encoded_y = base64pp::encode<QVector<double>, QByteArray>( y_data );
			query.addBindValue( measurement_id );
			query.addBindValue( encoded_x );
			query.addBindValue( encoded_y );

			if( !query.exec() )
			{
				qDebug() << "Error writing data to " << table_name << ":\n"
					<< query.lastError();
			}
		}

		if( callback_context != nullptr )
			QTimer::singleShot( 0, callback_context, callback );
	} );
}

void SQL_Manager::Write_SQL_XY_Data( const ID_To_XY_Data & data, const QStringList & what_to_write, const QString & table_name,
									 QObject* callback_context, std::function< void() > callback )
{
	if( data.empty() )
		return;
	QTimer::singleShot( 0, this, [ this, data, what_to_write, table_name, callback_context, callback ]
	{
		QSqlQuery query( this->sql_db );
		// Write each set of xy_data one measurement_id at a time
		for( const auto &[ measurement_id, xy_data ] : data )
		{
			std::vector<QString> many_qs( what_to_write.size(), "?" );
			QString question_marks = QStringList::fromVector(
				QVector<QString>( many_qs.begin(), many_qs.end() )
				).join( ',' );
			query.prepare( QString( "INSERT INTO %1 (%2) SELECT %3;" )
						   .arg( table_name,
								 what_to_write.join( ',' ),
								 question_marks ) );
			auto to_variant_list = []( const auto & xy_data ) -> std::tuple<QVariantList, QVariantList>
			{
				QVariantList output1;
				for( const auto & x : std::get<0>( xy_data ) )
					output1.push_back( x );
				QVariantList output2;
				for( const auto & y : std::get<1>( xy_data ) )
					output2.push_back( y );
				return { std::move( output1 ), std::move( output2 ) };
			};
			const auto[ x_data, y_data ] = to_variant_list( xy_data );
			std::vector<QVariant> many_ids( x_data.size(), measurement_id.toInt() );
			QVariantList measurement_id_copies = QVariantList::fromVector(
				QVector<QVariant>( many_ids.begin(), many_ids.end() ) );
			{
				query.addBindValue( measurement_id_copies );
				query.addBindValue( x_data );
				query.addBindValue( y_data );
			}

			if( !query.execBatch() )
			{
				qDebug() << "Error writing data to " << table_name << ":\n"
					<< query.lastError();
			}
			else
			{
				//qDebug() << "Successfully wrote data to " << table_name << ":\n"
				//	<< query.executedQuery();
			}
		}

		if( callback_context != nullptr )
			QTimer::singleShot( 0, callback_context, callback );
	} );
}

SQL_Manager_With_Local_Cache::SQL_Manager_With_Local_Cache( QObject* parent, QFileInfo config_filename, QString unique_name, QString config_category ) :
	local_sql(  this, config_filename, unique_name + "_local", "Local_SQL_Cache" ),
	remote_sql( this, config_filename, unique_name, config_category )
{
}

void SQL_Manager_With_Local_Cache::Start_Thread()
{
	local_sql.Start_Thread();
	remote_sql.Start_Thread();
	connect( &remote_sql, &SQL_Manager::Database_Opened, this, &SQL_Manager_With_Local_Cache::Database_Opened );
}

void SQL_Manager_With_Local_Cache::Grab_SQL_XY_Blob_Data_From_Measurement_IDs( const QStringList & what_to_collect, const QString & table_name,
																		  const QStringList & measurement_ids, QObject* callback_context,
																		  std::function< void( ID_To_XY_Data ) > callback, const QString & extra_filtering )
{
	local_sql.Grab_SQL_XY_Blob_Data_From_Measurement_IDs( what_to_collect, table_name, measurement_ids, this,
		[ this, what_to_collect, table_name, measurement_ids, callback_context, callback, extra_filtering ]( ID_To_XY_Data data )
		{
			const auto measurement_ids_missing = [ &measurement_ids, &data ]
			{
				QStringList copy_of_measurement_ids = measurement_ids;
				std::vector<QString> found_ids;
				for( const auto &[ id, xy_data ] : data )
					found_ids.push_back( id );
				// Need to sort lists before use so that set_difference works
				std::sort( found_ids.begin(), found_ids.end() );
				std::sort( copy_of_measurement_ids.begin(), copy_of_measurement_ids.end() );
				QStringList measurement_ids_missing;
				std::set_difference( copy_of_measurement_ids.begin(), copy_of_measurement_ids.end(), found_ids.begin(), found_ids.end(),
									 std::back_inserter( measurement_ids_missing ) );
				return measurement_ids_missing;
			}();
			if( measurement_ids_missing.size() > 0 ) // Need to lookup data from remote source
			{
				remote_sql.Grab_SQL_XY_Blob_Data_From_Measurement_IDs( what_to_collect, table_name, measurement_ids_missing, this,
					[ this, callback_context, callback, what_to_collect, table_name, data ]( ID_To_XY_Data remaining_data )
					{
						local_sql.Write_SQL_XY_Blob_Data( remaining_data, what_to_collect, table_name );
						remaining_data.insert( data.begin(), data.end() );
						QTimer::singleShot( 0, callback_context, [ callback, remaining_data = std::move( remaining_data ) ] { callback( remaining_data ); } );
					}, extra_filtering );
			}
			else
				QTimer::singleShot( 0, callback_context, [ callback, data ]{ callback( data ); } );
		}, extra_filtering );
}

void SQL_Manager_With_Local_Cache::Grab_SQL_XY_Data_From_Measurement_IDs( const QStringList & what_to_collect, const QString & table_name,
																		  const QStringList & measurement_ids, QObject* callback_context,
																		  std::function< void( ID_To_XY_Data ) > callback, const QString & extra_filtering )
{
	local_sql.Grab_SQL_XY_Data_From_Measurement_IDs( what_to_collect, table_name, measurement_ids, this,
		[ this, what_to_collect, table_name, measurement_ids, callback_context, callback, extra_filtering ]( ID_To_XY_Data data )
		{
			const auto measurement_ids_missing = [ &measurement_ids, &data ]
			{
				QStringList copy_of_measurement_ids = measurement_ids;
				std::vector<QString> found_ids;
				for( const auto &[ id, xy_data ] : data )
					found_ids.push_back( id );
				// Need to sort lists before use so that set_difference works
				std::sort( found_ids.begin(), found_ids.end() );
				std::sort( copy_of_measurement_ids.begin(), copy_of_measurement_ids.end() );
				QStringList measurement_ids_missing;
				std::set_difference( copy_of_measurement_ids.begin(), copy_of_measurement_ids.end(), found_ids.begin(), found_ids.end(),
									 std::back_inserter( measurement_ids_missing ) );
				return measurement_ids_missing;
			}();
			if( measurement_ids_missing.size() > 0 ) // Need to lookup data from remote source
			{
				remote_sql.Grab_SQL_XY_Data_From_Measurement_IDs( what_to_collect, table_name, measurement_ids_missing, this,
					[ this, callback_context, callback, what_to_collect, table_name, data ]( ID_To_XY_Data remaining_data )
					{
						local_sql.Write_SQL_XY_Data( remaining_data, what_to_collect, table_name );
						remaining_data.insert( data.begin(), data.end() );
						QTimer::singleShot( 0, callback_context, [ callback, remaining_data = std::move( remaining_data ) ] { callback( remaining_data ); } );
					}, extra_filtering );
			}
			else
				QTimer::singleShot( 0, callback_context, [ callback, data ]{ callback( data ); } );
		}, extra_filtering );
}

void SQL_Manager_With_Local_Cache::Grab_SQL_Metadata_From_Measurement_IDs( const QStringList & what_to_collect, const QString & table_name,
																		   const QStringList & measurement_ids, QObject* callback_context,
																		   std::function< void( ID_To_Metadata ) > callback, const QString & extra_filtering )
{
	remote_sql.Grab_SQL_Metadata_From_Measurement_IDs( what_to_collect, table_name, measurement_ids, callback_context, callback, extra_filtering );

	//local_sql.Grab_SQL_Metadata_From_Measurement_IDs( what_to_collect, table_name, measurement_ids, this,
	//	[ this, what_to_collect, table_name, measurement_ids, callback_context, callback, extra_filtering ]( ID_To_Metadata data )
	//	{
	//		const auto measurement_ids_missing = [ &measurement_ids, &data ]
	//		{
	//			QStringList copy_of_measurement_ids = measurement_ids;
	//			std::vector<QString> found_ids;
	//			for( const auto &[ id, xy_data ] : data )
	//				found_ids.push_back( id );
	//			// Need to sort lists before use so that set_difference works
	//			std::sort( found_ids.begin(), found_ids.end() );
	//			std::sort( copy_of_measurement_ids.begin(), copy_of_measurement_ids.end() );
	//			std::set<QString> measurement_ids_missing;
	//			std::set_difference( found_ids.begin(), found_ids.end(), copy_of_measurement_ids.begin(), copy_of_measurement_ids.end(),
	//								 std::inserter( measurement_ids_missing, measurement_ids_missing.end() ) );
	//			return measurement_ids_missing;
	//		}();
	//		if( measurement_ids_missing.size() > 0 ) // Need to lookup data from remote source
	//		{
	//			remote_sql.Grab_SQL_Metadata_From_Measurement_IDs( what_to_collect, table_name, measurement_ids, this,
	//				[ this, callback_context, callback, what_to_collect, table_name ]( ID_To_Metadata data )
	//				{
	//					local_sql.Write_SQL_Metadata( data, what_to_collect, table_name );
	//					QTimer::singleShot( 0, callback_context, [ callback, data = std::move( data ) ]{ callback( data ); } );
	//				}, extra_filtering );
	//		}
	//		else
	//			QTimer::singleShot( 0, callback_context, [ callback, data ] { callback( data ); } );
	//}, extra_filtering );
}


void SQL_Manager_With_Local_Cache::Grab_All_SQL_Metadata( const QStringList & what_to_collect, const QString & table_name, QObject* callback_context,
														  std::function< void( Structured_Metadata ) > callback, const QString & extra_filtering )
{
	remote_sql.Grab_All_SQL_Metadata( what_to_collect, table_name, callback_context, callback, extra_filtering );

	//remote_sql.Grab_All_SQL_Metadata( what_to_collect, table_name, &local_sql, [ this, callback_context, callback, what_to_collect, table_name ]( Structured_Metadata data )
	//{
	//	QTimer::singleShot( 0, callback_context, [ callback, data ] { callback( data ); } );

	//	int measurement_id_index = data.column_names.indexOf( "measurement_id" );
	//	if( measurement_id_index == -1 )
	//		return;
	//	ID_To_Metadata converted_data;
	//	for( auto one_measurement : data.data )
	//	{
	//		const QString & measurement_id = one_measurement[ measurement_id_index ].toString();
	//		converted_data[ measurement_id ] = one_measurement;
	//	}
	//	local_sql.Write_SQL_Metadata( std::move( converted_data ), what_to_collect, table_name );
	//}, extra_filtering );
}

