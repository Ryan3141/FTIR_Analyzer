﻿#include "IV_Plotter.h"
#include "Interactive_Graph_Toolbar.h"

using namespace std;


namespace IV
{

double Rule_07( double temperature, double cutoff_wavelength, double device_length_um )
{
	double J_0 = 8367.000019;
	double Pwr = 0.544071282;
	double C = -1.162972237;
	double lambda_scale = 0.200847413;
	double lambda_threshold = 4.635136423;
	double k_B = 1.3806503e-23;
	double ee = 1.60217646e-19;

	double scale_for_device_length = device_length_um / cutoff_wavelength;

	double lambda_e = ( cutoff_wavelength >= lambda_threshold ) ? cutoff_wavelength :
		cutoff_wavelength / ( 1 - std::pow( lambda_scale / cutoff_wavelength - lambda_scale / lambda_threshold, Pwr ) );

	return scale_for_device_length * J_0 * std::exp( C * 1.24 * ee / ( k_B * lambda_e * temperature ) );
}

Plotter::Plotter( QWidget *parent )
	: QWidget( parent )
{
	this->ui.setupUi( this );
	QString config_filename = "configuration.ini";
	Initialize_SQL( config_filename );
	Initialize_Tree_Table(); // sql_manager must be initialized first
	Initialize_Graph();
	Initialize_Rule07();
	//ui.customPlot->refitGraphs();

	//auto test = u8"This is a Unicode Character: μ\u2018.";
	//std::cerr << test;
	//const QString pageSource = R"(
	//	<html><head>
	//	<script type = "text/javascript" src = "https://cdnjs.cloudflare.com/ajax/libs/mathjax/2.7.5/MathJax.js?config=TeX-AMS-MML_HTMLorMML">
	//	</script></head>
	//	<body>
	//	<p><mathjax style = "font-size:2.3em">$$u = \int_{ -\infty }^{\infty}(awesome)\cdot du$$</mathjax></p>
	//	</body></html>
	//)";
	//ui.webEngineView->setHtml( pageSource );
	//ui.webEngineView->show();
}

void Plotter::Initialize_SQL( QString config_filename )
{
	QSettings settings( config_filename, QSettings::IniFormat, this );
	ui.sqlUser_lineEdit->setText( settings.value( "SQL_Server/default_user" ).toString() );
	QMessageBox* msgBox = new QMessageBox( this );

	sql_manager = new SQL_Manager_With_Local_Cache( this, config_filename, "IV" );
	connect( sql_manager, &SQL_Manager_With_Local_Cache::Error_Connecting_To_SQL, this, [ config_filename ]( QSqlError error_message )
	{
		QMessageBox msgBox;
		msgBox.setText( "Error Opening SQL" );
		msgBox.setInformativeText( error_message.text() + "\nDo you want to open your config file?\n(Restart program to try SQL again)" );
		msgBox.setStandardButtons( QMessageBox::Yes | QMessageBox::No );
		msgBox.setDefaultButton( QMessageBox::Save );
		int ret = msgBox.exec();
		if( ret == QMessageBox::Yes )
		{
			QDesktopServices::openUrl( QUrl( config_filename ) ); // Open config file in default program
		}
	} );

	sql_manager->Start_Thread();
}

void Plotter::Initialize_Tree_Table()
{
	config.header_titles    = QStringList{ "Sample Name", "Location", "Device Side Length (" + QString( QChar( 0x03BC ) ) + "m)", "Temperature (K)", "Date", "Time of Day", "measurement_id" };
	config.what_to_collect  = QStringList{ "sample_name", "device_location", "device_side_length_in_um", "temperature_in_k", "date(time)", "time(time)", "measurement_id" };
	config.sql_table        = "iv_measurements";
	config.raw_data_columns = QStringList{ "measurement_id","voltage_v","current_a" };
	config.raw_data_table   = "iv_raw_data";
	config.sorting_strategy = "ORDER BY measurement_id, voltage_v ASC";
	config.columns_to_show  = 6;

	ui.treeWidget->Set_Column_Info( config.header_titles, config.what_to_collect, config.columns_to_show );

	auto repoll_sql = [ this ]
	{
		QString user = ui.sqlUser_lineEdit->text();
		QString filter = ui.filter_lineEdit->text();

		sql_manager->Grab_All_SQL_Metadata( config.what_to_collect, config.sql_table, this, [ this, filter ]( Structured_Metadata data )
		{
			ui.treeWidget->current_meta_data = std::move( data );
			ui.treeWidget->Refilter( filter );
		}, QString( " WHERE user=\"%1\"" ).arg( user ) );
	};

	connect( ui.treeWidget, &SQL_Tree_Widget::Data_Double_Clicked, [ this ]( ID_To_Metadata id_and_metadata )
	{
		Graph_Data( id_and_metadata );
	} );

	// setup policy and connect slot for context menu popup:
	ui.treeWidget->setContextMenuPolicy( Qt::CustomContextMenu );
	connect( ui.treeWidget, &QWidget::customContextMenuRequested, this, &Plotter::treeContextMenuRequest );

	connect( ui.refresh_commandLinkButton, &QCommandLinkButton::clicked, repoll_sql );
	connect( ui.sqlUser_lineEdit, &QLineEdit::returnPressed, repoll_sql );
	connect( ui.filter_lineEdit, &QLineEdit::textEdited, ui.treeWidget, [ this ] { ui.treeWidget->Refilter( this->ui.filter_lineEdit->text() ); } );

	repoll_sql();
}

void Plotter::Initialize_Graph()
{
	ui.interactiveGraphToolbar->Connect_To_Graph( ui.customPlot );

	connect( ui.customPlot, &Interactive_Graph::Graph_Selected, [ this ]( QCPGraph* selected_graph )
	{
		const auto & measurement = ui.customPlot->FindDataFromGraphPointer( selected_graph );
		this->ui.selectedName_lineEdit->setText(        Info_Or_Default<QString>( measurement.meta, "Sample Name", "" ) );
		this->ui.selectedTemperature_lineEdit->setText( Info_Or_Default<QString>( measurement.meta, "Temperature (K)", "" ) );
	} );
}

void Plotter::treeContextMenuRequest( QPoint pos )
{
	QMenu *menu = new QMenu( this );
	menu->setAttribute( Qt::WA_DeleteOnClose );
	ID_To_Metadata selected = ui.treeWidget->Selected_Data();
	bool only_one_thing_selected = selected.size() == 1;

	menu->addAction( "Graph Selected", [this, selected]
	{
		Graph_Data( selected );
	} );

	menu->addAction( "Save to csv file", [this, selected]
	{
		this->Save_To_CSV( selected );
	} );

	menu->addAction( "Copy Measurement IDs", [ this, selected ]
	{
		const auto measurement_ids = selected
			% fn::transform( []( const auto & x ) { const auto &[ measurement_id, metadata ] = x; return measurement_id; } )
			% fn::to( QStringList{} );

		QClipboard *clipboard = QApplication::clipboard();
		clipboard->setText( measurement_ids.join( '\n' ) );
	} );

	menu->addAction( "Copy Measurement IDs (YAML)", [ this, selected ]
	{
		const auto measurement_ids = selected
			% fn::transform( []( const auto & x ) { const auto &[ measurement_id, metadata ] = x; return measurement_id; } )
			% fn::to( QStringList{} );

		QClipboard *clipboard = QApplication::clipboard();
		clipboard->setText( "[" + measurement_ids.join( ',' ) + "]" );
	} );

	menu->popup( ui.treeWidget->mapToGlobal( pos ) );
}

void Plotter::Graph_Data( const ID_To_Metadata & selected_data )
{
	for( auto[ measurement_id, metadata ] : selected_data )
	{
		auto labeled_metadata = Label_Metadata( metadata, config.what_to_collect );
		sql_manager->Grab_SQL_XY_Data_From_Measurement_IDs( config.raw_data_columns, config.raw_data_table, { measurement_id }, this,
															[ this, measurement_id, labeled_metadata ]( ID_To_XY_Data data )
		{
			Graph_Measurement( std::move( data ), ui.customPlot, measurement_id, labeled_metadata );
		}, config.sorting_strategy );
	}
}

void Plotter::Graph_Rule07( double dark_current_a_cm2, double temperature_in_k )
{
	// "Sample Name", "Location", "Device Side Length (" + QString( QChar( 0x03BC ) ) + "m)", "Temperature (K)", "Date", "Time of Day", "measurement_id"
	double lower_bound = ui.customPlot->xAxis->range().lower;
	double upper_bound = ui.customPlot->xAxis->range().upper;
	double side_length_um = 1E4; // Set it as a pretend device of area 1 cm^2 to make units correct
	ui.customPlot->Graph<X_Units::VOLTAGE_V, Y_Units::CURRENT_A>( QVector<double>( { lower_bound, upper_bound } ), QVector<double>( { dark_current_a_cm2, dark_current_a_cm2 } ),
																  "Rule 07", "Rule 07", Label_Metadata( { "Rule 07", "", side_length_um, temperature_in_k, "", "", "" }, config.what_to_collect ) );
	ui.customPlot->replot();
}

void Plotter::Initialize_Rule07()
{
	// Initialize Rule 07 Box
	double rule_07 = Rule_07( ui.rule07Temperature_doubleSpinBox->value(), ui.rule07Cutoff_doubleSpinBox->value(), ui.rule07DeviceLength_doubleSpinBox->value() );
	ui.rule07DarkCurrent_lineEdit->setText( QString::number( rule_07, 'E', 2 ) + QString::fromWCharArray( L" A/cm\u00B2" ) );

	auto replot_rule07 = [ this ]
	{
		double temperature_in_k = ui.rule07Temperature_doubleSpinBox->value();
		double rule_07 = Rule_07( temperature_in_k, ui.rule07Cutoff_doubleSpinBox->value(), ui.rule07DeviceLength_doubleSpinBox->value() );
		ui.rule07DarkCurrent_lineEdit->setText( QString::number( rule_07, 'E', 4 ) + QString::fromWCharArray( L" A/cm\u00B2" ) );
		if( this->ui.rule07_checkBox->isChecked() )
			this->Graph_Rule07( rule_07, temperature_in_k );
		else
			ui.customPlot->Hide_Graph( "Rule 07" );
	};
	connect( ui.customPlot->xAxis, qOverload<const QCPRange &>( &QCPAxis::rangeChanged ), [ replot_rule07 ]( const QCPRange & ) { replot_rule07(); } );
	connect( ui.rule07Temperature_doubleSpinBox, qOverload<double>( &QDoubleSpinBox::valueChanged ), [ this, replot_rule07 ]( double )
	{
		replot_rule07();
	} );
	connect( ui.rule07Cutoff_doubleSpinBox, qOverload<double>( &QDoubleSpinBox::valueChanged ), [ this, replot_rule07 ]( double value )
	{
		ui.rule07DeviceLength_doubleSpinBox->setValue( value );
		replot_rule07();
	} );
	connect( ui.rule07DeviceLength_doubleSpinBox, qOverload<double>( &QDoubleSpinBox::valueChanged ), [ this, replot_rule07 ]( double )
	{
		replot_rule07();
	} );
	connect( ui.rule07_checkBox, &QCheckBox::stateChanged, [ replot_rule07 ]( int ) { replot_rule07(); } );

}

void Plotter::Save_To_CSV( const ID_To_Metadata & things_to_save )
{
	QFileInfo file_name = QFileDialog::getSaveFileName( this, tr( "Save Data" ), QString(), tr( "CSV File (*.csv)" ) );//;; JPG File (*.jpg);; BMP File (*.bmp);; PDF File (*.pdf)" ) );

	//if( file_name.suffix().toLower() == "csv" )
	{
		//const auto[ measurement_ids_to_graph, measurements_meta_data ] = fn::cfrom( things_to_save )
		//const auto test = fn::cfrom( things_to_save )
		//	% fn::transform( []( const auto[ key, value ] ) -> std::tuple<QString, Metadata> { return {key, value}; } );
		//% fn::unzip( std::tuple< QStringList, std::vector<Metadata> >{} );
		QStringList measurement_ids_to_graph;
		std::vector<Metadata> measurements_meta_data;
		for( auto[ measurement_id, metadata ] : things_to_save )
		{
			measurement_ids_to_graph.push_back( measurement_id );
			measurements_meta_data.push_back( metadata );
		}

		sql_manager->Grab_SQL_XY_Data_From_Measurement_IDs( config.raw_data_columns, config.raw_data_table, measurement_ids_to_graph, this,
															[ this, file_name, measurement_ids_to_graph, measurements_meta_data ]( ID_To_XY_Data id_to_data )
		{
			QVector<double> repeating_x_data;
			int longest_y_data = 0;
			std::vector< std::vector<std::string> > data_before_transpose;
			//for( const auto &[ measurement_id, metadata ] : measurements_to_graph % fn::zip_with( measurements_meta_data, []( const auto & x, const auto & y ) { return std::tuple<QString, Metadata>{ x, y }; } ) )
			for( const auto[ measurement_id, metadata ] : fn::zip( measurement_ids_to_graph, measurements_meta_data ) )
			{
				if( id_to_data.find( measurement_id ) == id_to_data.end() )
					continue;

				const XY_Data & data = id_to_data[ measurement_id ];
				auto[ x_data, y_data ] = data; // Output the raw data without adjusting for gain
				//auto[ x_data, y_data ] = Scale_FTIR_XY_Data( meta_data, data );

				bool x_data_is_repeating = repeating_x_data.size() == x_data.size();
				if( x_data_is_repeating )
				{
					for( int i = 0; i < x_data.size(); i++ )
					{
						if( repeating_x_data[ i ] != x_data[ i ] )
						{
							x_data_is_repeating = false;
							repeating_x_data = x_data;
							break;
						}
					}
				}
				else
					repeating_x_data = x_data;


				if( !x_data_is_repeating )
				{
					data_before_transpose.resize( data_before_transpose.size() + 1 );
					std::vector<std::string> & current_line = data_before_transpose.back();
					current_line.push_back( "Voltage (V)" );
					for( double x : x_data )
						current_line.push_back( std::to_string( x ) );
				}

				{
					data_before_transpose.resize( data_before_transpose.size() + 1 );
					std::vector<std::string> & current_line = data_before_transpose.back();
					// "Sample Name", "Location", "Device Side Length (" + QString( QChar( 0x03BC ) ) + "m)", "Temperature (K)", "Date", "Time of Day", "measurement_id"
					QString info = metadata[ 0 ].toString() + " " + metadata[ 1 ].toString() + " " + metadata[ 3 ].toString() + "K";
					current_line.push_back( info.toStdString() );
					longest_y_data = std::max( longest_y_data, y_data.size() );
					for( int i = 0; i < y_data.size(); i++ )
						current_line.push_back( std::to_string( y_data[ i ] ) );
				}

			}

			std::ofstream out_file( file_name.absoluteFilePath().toStdString() );
			//data_before_transpose % fn::transpose2D();
			for( int j = 0; j < longest_y_data; j++ )
			{
				for( const auto & data_group : data_before_transpose )
				{
					if( data_group.size() > j )
						out_file << data_group[ j ];
					out_file << ",";
				}
				out_file << "\n";
			}
		}, config.sorting_strategy );
	}

}

template< typename Func >
QString lookup( const Labeled_Metadata & metadata, const QString & lookup_name, QString the_default, Func run_if_exists )
{
	auto lookup = metadata.find( lookup_name );
	if( lookup == metadata.end() || lookup->second.isNull() )
		return the_default;
	else
		return run_if_exists( lookup->second );
}

//const QStringList header_titles{ "Sample Name", "Date", "Temperature (K)", "Location", "Side Length", "Time of Day", "Sweep Direction", "measurement_id" };
void Graph_Measurement( ID_To_XY_Data data, Graph_Base* graph, QString measurement_id, Labeled_Metadata metadata, QString legend_label )
{
	auto[ x_data, y_data ] = data[ measurement_id ];
	QString used_legend_label = legend_label;
	if( used_legend_label == "" )
	{
		auto to_string = []( QVariant value ) { return value.toString(); };
		QString sample_name = lookup( metadata, "sample_name", "", to_string );
		QString temperature_label = lookup( metadata, "temperature_in_k", "", []( QVariant value ) { return value.toString() + " K"; } );
		QString bias_label = lookup( metadata, "bias_in_v", "", []( QVariant value ) { return QString::number( std::round( value.toFloat() * 1.0E3 ) ) + " mV ";  } );
		QString side_length_label = lookup( metadata, "device_side_length_in_um", "",
											[]( QVariant value ) { return QString::number( std::round( value.toFloat() ) ) + " " + QString( QChar( 0x03BC ) ) + "m";  } );
		QString device_location = lookup( metadata, "device_location", "", to_string );
		used_legend_label = QStringList{ sample_name, device_location, side_length_label, temperature_label, bias_label }.join( " " );
	}
	graph->Graph<X_Units::VOLTAGE_V, Y_Units::CURRENT_A>( x_data, y_data, measurement_id, used_legend_label, metadata );
	graph->replot();
}

}