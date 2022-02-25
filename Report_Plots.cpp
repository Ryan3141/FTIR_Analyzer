#include "Report_Plots.h"

#include "Interactive_Graph_Toolbar.h"

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

	sql_manager = new SQL_Manager( this, config_filename, "Report" );
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

//struct Structured_Metadata
//{
//	QStringList column_names;
//	std::vector<Metadata> data;
//};
// using ID_To_XY_Data = std::map<QString, XY_Data>;

//const QStringList header_titles{ "Sample Name", "Date", "Temperature (K)", "Location", "Side Length", "Time of Day", "Sweep Direction", "measurement_id" };
void Report_Plots::Load_Report( QFileInfo file )
{
	std::ifstream in( file.absoluteFilePath().toStdString() );
	if( !in.is_open() )
		return;

	ui.reportIVSize_customPlot->removeAllGraphs();

	std::ifstream data_file( file.filePath().toStdString() );
	std::string whole_file( ( std::istreambuf_iterator<char>( data_file ) ),
							std::istreambuf_iterator<char>() );
	const auto csv_data = Load_CSV_Data( whole_file );

	for( const auto & one_row : csv_data )
	{
		const QStringList ids_in_report = QStringList::fromVector( QVector<QString>::fromStdVector(
			one_row % fn::where( []( const auto & std_s ) { return !std_s.empty(); } )
				% fn::transform( []( const auto & std_s ) { return QString::fromStdString( std_s ); } )
		) );

		sql_manager->Grab_All_SQL_Metadata( config.what_to_collect, config.sql_table, this, [ this ]( Structured_Metadata metadata )
		{
			const auto & column_names = metadata.column_names;

			int side_length_i = column_names.indexOf( "device_side_length_in_um" );
			const std::vector<Metadata> metadata_sorted_by_device_sizes = metadata.data
				% fn::sort_by( [ side_length_i ]( const auto & row_of_metadata ) { return row_of_metadata[ side_length_i ]; } );

			int measurement_id_i = column_names.indexOf( "measurement_id" );
			const std::vector<QString> measurement_ids_as_vec = metadata_sorted_by_device_sizes
				% fn::transform( [ measurement_id_i ]( const auto & row_of_metadata ) { return row_of_metadata[ measurement_id_i ].toString(); } );
			//% fn::to( QStringList{} );
			const QStringList measurement_ids = QStringList::fromVector( QVector<QString>::fromStdVector( measurement_ids_as_vec ) );

			double voltage_to_use = -0.1;
			sql_manager->Grab_SQL_XY_Data_From_Measurement_IDs( config.raw_data_columns, config.raw_data_table, measurement_ids, this,
																[ this, column_names, metadata_sorted_by_device_sizes, voltage_to_use ]( ID_To_XY_Data data )
			{
				int measurement_id_i = column_names.indexOf( "measurement_id" );
				int device_side_length_in_um_i = column_names.indexOf( "device_side_length_in_um" );
				QVector< double > side_lengths_um;
				QVector< double > currents_per_side_a;
				for( const auto & one_metadata : metadata_sorted_by_device_sizes )
				{
					QString measurement_id = one_metadata[ measurement_id_i ].toString();
					if( data.find( measurement_id ) == data.end() )
						continue;

					const auto & xy_data = data[ measurement_id ];
					const auto &[ voltages_v, currents_a ] = xy_data;
					double side_length_um = one_metadata[ device_side_length_in_um_i ].toDouble();

					int i = std::distance( voltages_v.begin(), std::lower_bound( voltages_v.begin(), voltages_v.end(), voltage_to_use ) );
					i = std::min( voltages_v.size() - 1, i );
					side_lengths_um.push_back( 4 / side_length_um ); // Area / perimeter = side * side / (4 * side)
					currents_per_side_a.push_back( currents_a[ i ] );
				}
				//Label_Metadata( Metadata meta, QStringList labels )
				int sample_name_i = column_names.indexOf( "sample_name" );
				int temperature_in_k_i = column_names.indexOf( "temperature_in_k" );
				QString name = QString( "%1 %2 K" ).arg( metadata_sorted_by_device_sizes[ 0 ][ sample_name_i ].toString(), metadata_sorted_by_device_sizes[ 0 ][ temperature_in_k_i ].toString() );
				ui.reportIVSize_customPlot->Graph<X_Units::AREA_OVER_PERIMETER_M, Y_Units::CURRENT_A>(
					side_lengths_um, currents_per_side_a, name, name, {} );
				ui.reportIVSize_customPlot->replot();
			}, config.sorting_strategy );

		}, QString( " WHERE measurement_id in (%1)" ).arg( ids_in_report.join( ',' ) ) );
	}
}

}