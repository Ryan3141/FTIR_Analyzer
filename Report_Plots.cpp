#include "Report_Plots.h"

#include "Interactive_Graph_Toolbar.h"
#include "yaml-cpp/yaml.h"
#include "qtyaml.h"

#include "IV_By_Size_Plotter.h"
#include "FTIR_Analyzer.h"
#include "IV_Plotter.h"
#include "Lifetime_Plotter.h"

namespace Report
{

Report_Plots::Report_Plots( QWidget *parent, QString initial_configuration )
	: QWidget( parent )
{
	this->ui.setupUi( this );
	QString config_filename = "configuration.ini";
	Initialize_SQL( config_filename );
	Initialize_Graph();
	//ui.customPlot->refitGraphs();

	if( initial_configuration != "" )
	{
		this->temporary_connection = connect( sql_manager, &SQL_Manager_With_Local_Cache::Database_Opened, [this, initial_configuration]
		{
			QObject::disconnect( this->temporary_connection );
			QFileInfo file = "../Reports/" + initial_configuration;
			std::string debug = file.absoluteFilePath().toStdString();
			Load_Report( file );
		} );
	}
}

void Report_Plots::Initialize_SQL( QString config_filename )
{
	{
		auto & config = this->iv_by_size_sql_config;
		config.header_titles = QStringList{ "Sample Name", "Location", "Device Side Length (" + QString( QChar( 0x03BC ) ) + "m)", "Temperature (K)", "Date", "Time of Day", "measurement_id" };
		config.what_to_collect = QStringList{ "sample_name", "device_location", "device_side_length_in_um", "temperature_in_k", "date(time)", "time(time)", "measurement_id" };
		config.sql_table = "iv_measurements";
		config.raw_data_columns = QStringList{ "measurement_id","voltage_v","current_a" };
		config.raw_data_table = "iv_raw_data";
		config.sorting_strategy = "ORDER BY measurement_id, voltage_v ASC";
		config.columns_to_show = 6;
		this->iv_sql_config = this->iv_by_size_sql_config;
	}
	{
		auto & config = this->spectral_response_sql_config;
		config.header_titles = QStringList{ "Date", "Sample Name", "Temperature (K)", "Device Side Length (" + QString( QChar( 0x03BC ) ) + "m)", "Location", "Bias (V)", "Dewar Temp (C)", "Time of Day", "Gain", "measurement_id" };
		config.what_to_collect = QStringList{ "date(time)", "sample_name", "temperature_in_k", "device_side_length_in_um", "device_location", "bias_in_v", "dewar_temp_in_c", "time(time)", "gain", "measurement_id" }; // DATE_FORMAT(time, '%b %e %Y') DATE_FORMAT(time, '%H:%i:%s')
		config.sql_table = "ftir_measurements";
		config.raw_data_columns = QStringList{ "measurement_id","wavenumber","intensity" };
		config.raw_data_table = "ftir_raw_data";
		config.sorting_strategy = "ORDER BY measurement_id, wavenumber ASC";
		config.columns_to_show = 8;
		this->ftir_sql_config = this->spectral_response_sql_config;
	}

	{
		auto& config = this->lifetime_sql_config;
		config.header_titles = QStringList{ "Sample Name", "T Gain", "Location", "Device Side Length (" + QString( QChar( 0x03BC ) ) + "m)", "Bias (V)", "Temperature (K)", "Date", "Time of Day", "measurement_id" };
		config.what_to_collect = QStringList{ "sample_name", "transimpedance_gain", "device_location", "device_side_length_um", "bias_v", "temperature_in_k", "date(time)", "time(time)", "measurement_id" }; // DATE_FORMAT(time, '%b %e %Y') DATE_FORMAT(time, '%H:%i:%s')
		config.sql_table = "lifetime_measurements";
		config.raw_data_columns = QStringList{ "measurement_id","time_s","voltage_v" };
		config.raw_data_table = "lifetime_raw_data";
		config.sorting_strategy = "";
		config.columns_to_show = 8;
	}

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
		QFileDialog dialog( this, tr( "Load Report File" ), QString(), tr( "Report File (*.yaml *.yml);;All Files (*)" ) );
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

template< typename ReturnType >
ReturnType lookup( const YAML::Node & thing, ReturnType the_default )
{
	if( thing )
		return thing.as< ReturnType >();
	else
		return the_default;
}

template< typename Plot_Type >
void set_graph_limits( const YAML::Node & node, Plot_Type* plot )
{
	if( node[ "x_units" ] )
		plot->Change_X_Axis( node[ "x_units" ].as< int >() );
	if( node[ "y_units" ] )
		plot->Change_Y_Axis( node[ "y_units" ].as< int >() );
	plot->rescaleAxes();

	if( node[ "x_min" ] )
		plot->xAxis->setRangeLower( node[ "x_min" ].as<double>() );
	if( node[ "x_max" ] )
		plot->xAxis->setRangeUpper( node[ "x_max" ].as<double>() );
	if( node[ "y_min" ] )
		plot->yAxis->setRangeLower( node[ "y_min" ].as<double>() );
	if( node[ "y_max" ] )
		plot->yAxis->setRangeUpper( node[ "y_max" ].as<double>() );
	plot->replot();
}

template< typename Plot_Type >
void set_graph_title( const YAML::Node & node, Plot_Type* plot )
{
	if( !node[ "title" ] )
		return;
	QString title = node[ "title" ].as< QString >();
	plot->Set_Title( title );
}

QWidget* By_Device_Size( YAML::Node main_plot_node, Request_Data_Func get_the_data )
{
	auto interactive_graph = new IV_By_Size::Interactive_Graph();

	set_graph_title( main_plot_node, interactive_graph );
	QString save_file_name = lookup( main_plot_node[ "save_file_name" ], QString{} );
	if( main_plot_node[ "voltage_v" ] )
		interactive_graph->axes.set_voltage = main_plot_node[ "voltage_v" ].as< double >();

	auto plots = main_plot_node[ "plots" ];
	if( !plots || !plots.IsSequence() )
		return interactive_graph;
	auto data_remaining = plots.size(); // Workaround to async sequencing

	bool legend_on = lookup(main_plot_node["legend_on"], false);
	interactive_graph->legend->setVisible(legend_on);

	for( const auto & one_plot : plots )
	{
		data_remaining--;
		if( !one_plot[ "measurement_ids" ] )
			continue;
		QString title = lookup( one_plot[ "title" ], QString{} );
		
		if( !one_plot[ "measurement_ids" ] || !one_plot[ "measurement_ids" ].IsSequence() )
			continue;
		QStringList measurement_ids = one_plot[ "measurement_ids" ].as< QStringList >();

		get_the_data( measurement_ids,
						[ interactive_graph, title, main_plot_node, save_file_name, data_remaining ]( Structured_Metadata metadata, ID_To_XY_Data data ) mutable
		{
			interactive_graph->Graph( metadata, data, title );
			interactive_graph->axisRect()->insetLayout()->setInsetAlignment( 0, Qt::AlignTop | Qt::AlignLeft );
			set_graph_limits( main_plot_node, interactive_graph );
			interactive_graph->Recolor_Graphs( QCPColorGradient::gpSpectrum );
			interactive_graph->replot();
			if( data_remaining == 0 && !save_file_name.isEmpty() )
				interactive_graph->saveAsStandardPdf( save_file_name );
		} );
	}
	return interactive_graph;
}

QWidget* Dark_Current( YAML::Node main_plot_node, Request_Data_Func get_the_data )
{
	auto interactive_graph = new IV::Interactive_Graph();

	set_graph_title( main_plot_node, interactive_graph );
	QString save_file_name = lookup( main_plot_node[ "save_file_name" ], QString{} );

	auto plots = main_plot_node[ "plots" ];
	if( !plots || !plots.IsSequence() )
		return interactive_graph;

	bool legend_on = lookup(main_plot_node["legend_on"], false);
	interactive_graph->legend->setVisible(legend_on);

	auto data_remaining = plots.size(); // Workaround to async sequencing
	for( const auto & one_plot : plots )
	{
		data_remaining--;
		if( !one_plot[ "measurement_id" ] )
			continue;
		QString legend_label = lookup( one_plot[ "legend_label" ], QString{} );
		QString measurement_id = one_plot[ "measurement_id" ].as<QString>();

		get_the_data( { measurement_id },
					  [ interactive_graph, measurement_id, legend_label, main_plot_node, save_file_name, data_remaining ]( Structured_Metadata metadata, ID_To_XY_Data data ) mutable
		{
			IV::Graph_Measurement( std::move( data ), interactive_graph, measurement_id, Label_Metadata( metadata.data[ 0 ], metadata.column_names ), legend_label );
			interactive_graph->axisRect()->insetLayout()->setInsetAlignment( 0, Qt::AlignBottom | Qt::AlignRight );
			set_graph_limits( main_plot_node, interactive_graph );
			interactive_graph->Recolor_Graphs( QCPColorGradient::gpSpectrum );
			interactive_graph->replot();
			if( data_remaining == 0 && !save_file_name.isEmpty() )
				interactive_graph->saveAsStandardPdf( save_file_name );
		} );
	}
	return interactive_graph;
}

QWidget* Spectral_Response( YAML::Node main_plot_node, Request_Data_Func get_the_data )
{
	auto interactive_graph = new FTIR::Interactive_Graph();

	set_graph_title( main_plot_node, interactive_graph );
	QString save_file_name = lookup( main_plot_node[ "save_file_name" ], QString{} );

	auto plots = main_plot_node[ "plots" ];
	if( !plots || !plots.IsSequence() )
		return interactive_graph;

	bool legend_on = lookup(main_plot_node["legend_on"], false);
	interactive_graph->legend->setVisible( legend_on );

	auto data_remaining = plots.size(); // Workaround to async sequencing
	for( const auto & one_plot : plots )
	{
		data_remaining--;
		if( !one_plot[ "measurement_id" ] )
			continue;
		QString legend_label = lookup( one_plot[ "legend_label" ], QString{} );
		QString measurement_id = one_plot[ "measurement_id" ].as<QString>();
		QString background_id = lookup( one_plot[ "background_id" ], QString{} );
		QStringList ids = { measurement_id };
		if( !background_id.isEmpty() )
			ids.append( background_id );
		get_the_data( ids,
		[ interactive_graph, measurement_id, background_id, legend_label, main_plot_node, save_file_name, data_remaining ]( Structured_Metadata metadata, ID_To_XY_Data data ) mutable
		{
			FTIR::Graph_Measurement( std::move( data ), interactive_graph, measurement_id, Label_Metadata( metadata.data[ 0 ], metadata.column_names ), legend_label, background_id );

			interactive_graph->axisRect()->insetLayout()->setInsetAlignment( 0, Qt::AlignTop | Qt::AlignLeft );
			set_graph_limits( main_plot_node, interactive_graph );
			interactive_graph->Recolor_Graphs( QCPColorGradient::gpSpectrum );
			interactive_graph->replot();
			if( data_remaining == 0 && !save_file_name.isEmpty() )
				interactive_graph->saveAsStandardPdf( save_file_name );
		} );
	}
	return interactive_graph;
}

QWidget* Lifetime( YAML::Node main_plot_node, Request_Data_Func get_the_data )
{
	auto interactive_graph = new Lifetime::Interactive_Graph();

	set_graph_title( main_plot_node, interactive_graph );
	QString save_file_name = lookup( main_plot_node[ "save_file_name" ], QString{} );

	auto plots = main_plot_node[ "plots" ];
	if( !plots || !plots.IsSequence() )
		return interactive_graph;

	auto data_remaining = plots.size(); // Workaround to async sequencing
	for( const auto& one_plot : plots )
	{
		//data_remaining--;
		//if( !one_plot[ "measurement_id" ] )
		//	continue;
		//QString legend_label = lookup( one_plot[ "title" ], QString{} );
		//QString measurement_id = one_plot[ "measurement_id" ].as<QString>();

		//get_the_data( { measurement_id },
		//	[interactive_graph, measurement_id, legend_label, main_plot_node, save_file_name, data_remaining]( Structured_Metadata metadata, ID_To_XY_Data data ) mutable
		//	{
		//		Lifetime::Graph_Measurement( std::move( data ), interactive_graph, measurement_id, Label_Metadata( metadata.data[ 0 ], metadata.column_names ), legend_label );
		//		interactive_graph->axisRect()->insetLayout()->setInsetAlignment( 0, Qt::AlignTop | Qt::AlignLeft );
		//		set_graph_limits( main_plot_node, interactive_graph );
		//		interactive_graph->Recolor_Graphs( QCPColorGradient::gpSpectrum );
		//		interactive_graph->replot();
		//		if( data_remaining == 0 && !save_file_name.isEmpty() )
		//			interactive_graph->saveAsStandardPdf( save_file_name );
		//	} );
	}
	return interactive_graph;
}

void Report_Plots::Get_SQL_Data( const SQL_Configuration & config, QStringList measurement_ids, Data_Is_Returned_Func run_on_data )
{
	auto grab_raw_data = [ this, measurement_ids, config, run_on_data ]( Structured_Metadata metadata )
	{
		sql_manager->Grab_SQL_XY_Data_From_Measurement_IDs( config.raw_data_columns, config.raw_data_table, measurement_ids, this,
															[ run_on_data, metadata ]( ID_To_XY_Data xy_data ) { run_on_data( metadata, xy_data ); }, config.sorting_strategy );
	};

	sql_manager->Grab_All_SQL_Metadata( config.what_to_collect, config.sql_table, this, grab_raw_data, QString( " WHERE measurement_id in (%1)" ).arg( measurement_ids.join( "," ) ) );
}

void Report_Plots::Load_Report( QFileInfo file )
{
	std::ifstream in( file.absoluteFilePath().toStdString() );
	if( !in.is_open() )
		return;

	YAML::Node report_outline = YAML::Load( in );

	if( report_outline[ "title" ] )
		ui.report_label->setText( QString::fromStdString( report_outline[ "title" ].as<std::string>() ) );

	if( report_outline[ "tab_title" ] )
		emit Change_Tab_Name( QString::fromStdString( report_outline[ "tab_title" ].as<std::string>() ) );

	YAML::Node list_of_devices = report_outline[ "windows" ];
	if( !list_of_devices || !list_of_devices.IsSequence() )
		return;

	std::array< std::function<QWidget*( QWidget* )>, 4 > replace_functions = {
		[ this ]( QWidget* replacement ) { return ui.left_splitter->replaceWidget( 0, replacement ); },
		[ this ]( QWidget* replacement ) { return ui.right_splitter->replaceWidget( 0, replacement ); },
		[ this ]( QWidget* replacement ) { return ui.left_splitter->replaceWidget( 1, replacement ); },
		[ this ]( QWidget* replacement ) { return ui.right_splitter->replaceWidget( 1, replacement ); }
	};
	struct Function_And_Config
	{
		std::function<QWidget*( YAML::Node, Request_Data_Func )> func;
		SQL_Configuration config;
	};

	std::map< QString, Function_And_Config > function_for_graph_type = {
		{ "current_voltage"  , { &Dark_Current     , iv_sql_config } },
		{ "by_device_size"   , { &By_Device_Size   , iv_by_size_sql_config } },
		{ "spectral_response", { &Spectral_Response, ftir_sql_config } },
		{ "lifetime"         , { &Lifetime         , lifetime_sql_config } },
	};

	for( auto replace_widget : replace_functions )
	{
		QWidget* old_widget = replace_widget( new QWidget() );
		old_widget->close();
	}

	for( int i = 0; const auto & this_plot_node : list_of_devices )
	{
		auto replace_widget = replace_functions[ i ];
		i = (i + 1) % 4;

		if( !this_plot_node[ "plot_type" ] )
			continue;
		QString plot_type = this_plot_node[ "plot_type" ].as<QString>();
		auto find_plot_type = function_for_graph_type.find( plot_type );
		if( find_plot_type == function_for_graph_type.end() )
			continue;

		auto &[ func, config2 ] = find_plot_type->second;
		auto config = config2;
		QWidget* new_plot = func( this_plot_node, [ this, config ]( QStringList measurement_ids, Data_Is_Returned_Func run_on_data )
		{
			Get_SQL_Data( config, measurement_ids, run_on_data );
		} );
		QWidget* old_widget = replace_widget( new_plot );
		old_widget->close();
	}
}


}