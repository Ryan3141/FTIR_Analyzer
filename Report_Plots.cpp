#include "Report_Plots.h"

#include "Interactive_Graph_Toolbar.h"

#include "IV_By_Size_Plotter.h"

namespace Report
{

Report_Plots::Report_Plots( QWidget *parent )
	: QWidget( parent )
{
	this->ui.setupUi( this );
	QString config_filename = "configuration.ini";
	Initialize_SQL( config_filename );
	Initialize_Graph();
	//ui.customPlot->refitGraphs();
}

void Report_Plots::Initialize_SQL( QString config_filename )
{
	config.header_titles = QStringList{ "Sample Name", "Location", "Device Side Length (" + QString( QChar( 0x03BC ) ) + "m)", "Temperature (K)", "Date", "Time of Day", "measurement_id" };
	config.what_to_collect = QStringList{ "sample_name", "device_location", "device_side_length_in_um", "temperature_in_k", "date(time)", "time(time)", "measurement_id" };
	config.sql_table = "iv_measurements";
	config.raw_data_columns = QStringList{ "measurement_id","voltage_v","current_a" };
	config.raw_data_table = "iv_raw_data";
	config.sorting_strategy = "ORDER BY measurement_id, voltage_v ASC";
	config.columns_to_show = 6;

	sql_manager = new SQL_Manager_With_Local_Cache( this, config_filename, "Report" );
	sql_manager->Start_Thread();
}
	//auto repoll_sql = [ this ]
	//{
	//	QString user = ui.sqlUser_lineEdit->text();
	//	QString filter = ui.filter_lineEdit->text();

	//	sql_manager->Grab_All_SQL_Metadata( config.what_to_collect, config.sql_table, this, [ this, filter ]( Structured_Metadata data )
	//	{
	//		ui.treeWidget->current_meta_data = std::move( data );
	//		ui.treeWidget->Refilter( filter );
	//	}, QString( " WHERE user=\"%1\"" ).arg( user ) );
	//};

	//connect( ui.refresh_commandLinkButton, &QCommandLinkButton::clicked, repoll_sql );

void Report_Plots::Initialize_Graph()
{
	//ui.interactiveGraphToolbar->Connect_To_Graph( ui.customPlot );

	//connect( ui.customPlot->xAxis, qOverload<const QCPRange &>( &QCPAxis::rangeChanged ), [ this ]( const QCPRange & ) { this->Update_Preview_Graph(); } );
	connect( ui.loadReport_pushButton, &QPushButton::pressed, [ this ]
	{
		QFileDialog dialog( this, tr( "Load Report File" ), QString(), tr( "Comma Separated File (*.csv)" ) );
		dialog.setAcceptMode( QFileDialog::AcceptOpen );
		//QString current_text = ui.layersFile_lineEdit->text();
		//if( current_text == "" )
		//	dialog.selectFile( "." );
		//else
		//	dialog.selectFile( QFileInfo( current_text ).baseName() );

		auto result = dialog.exec();
		if( result != QDialog::Accepted )
			return;

		QString file_path_string = dialog.selectedFiles()[ 0 ];
		QFileInfo full_file_path( file_path_string );
		this->Load_Report( full_file_path );
	} );

}

//const QStringList header_titles{ "Sample Name", "Date", "Temperature (K)", "Location", "Side Length", "Time of Day", "Sweep Direction", "measurement_id" };
void Report_Plots::Load_Report( QFileInfo file )
{
	std::ifstream in( file.absoluteFilePath().toStdString() );
	if( !in.is_open() )
		return;

	ui.reportIVSize_customPlot->removeAllGraphs();
	const auto csv_data = [ file ]
	{
		std::ifstream data_file( file.filePath().toStdString() );
		std::string whole_file( ( std::istreambuf_iterator<char>( data_file ) ),
								std::istreambuf_iterator<char>() );
		return Load_CSV_Data( whole_file );
	}();

	for( const auto & one_row : csv_data )
	{
		const QStringList ids_in_report = QStringList::fromVector( QVector<QString>::fromStdVector(
			one_row % fn::where( []( const auto & std_s ) { return !std_s.empty(); } )
			% fn::transform( []( const auto & std_s ) { return QString::fromStdString( std_s ); } )
		) );

		auto grab_raw_data = [ this ]( Structured_Metadata metadata )
		{
			Graph_IV_By_Size_Raw_Data( this, ui.reportIVSize_customPlot, metadata );
		};

		sql_manager->Grab_All_SQL_Metadata( config.what_to_collect, config.sql_table, this, grab_raw_data, QString( " WHERE measurement_id in (%1)" ).arg( ids_in_report.join( ',' ) ) );
	}
}

}