﻿#include "CV_Plotter.h"
#include "CV_Theoretical.h"
#include "Interactive_Graph_Toolbar.h"

using namespace std;

//const QStringList header_titles{ "Sample Name", "Date", "Temperature (K)", "Location", "Device Area", "Time of Day", "Sweep Direction", "measurement_id" };
//const QStringList what_to_collect{ "sample_name", "date(time)", "temperature_in_k", "device_location", "device_area_in_um2", "time(time)", "sweep_direction", "measurement_id" };
//const QString sql_table = "cv_measurements";
//const QStringList raw_data_columns{ "measurement_id","voltage_v","capacitance_f" };
//const QString raw_data_table = "cv_raw_data";
//const int columns_to_show = 6;


namespace CV
{

Plotter::Plotter( QWidget *parent )
	: QWidget( parent )
{
	this->ui.setupUi( this );
	QString config_filename = "configuration.ini";
	Initialize_SQL( config_filename );
	Initialize_Tree_Table(); // sql_manager must be initialized first
	Initialize_Graph();
	Update_Preview_Graph();
	//ui.customPlot->refitGraphs();
}

void Plotter::Initialize_SQL( QString config_filename )
{
	QSettings settings( config_filename, QSettings::IniFormat, this );
	ui.sqlUser_lineEdit->setText( settings.value( "SQL_Server/default_user" ).toString() );
	QMessageBox* msgBox = new QMessageBox( this );

	sql_manager = new SQL_Manager( this, config_filename, "CV" );
	connect( sql_manager, &SQL_Manager::Error_Connecting_To_SQL, this, [ config_filename ]( QSqlError error_message )
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
	config.header_titles    = QStringList{ "Sample Name", "Date", "Temperature (K)", "Location", "Device Side Length (" + QString( QChar( 0x03BC ) ) + "m)", "Time of Day", "AC Frequency (Hz)", "AC Amplitude (V)", "Sweep Direction", "measurement_id" };
	config.what_to_collect  = QStringList{ "sample_name", "date(time)", "temperature_in_k", "device_location", "device_side_length_in_um", "time(time)", "ac_frequency_hz", "ac_amplitude_v", "sweep_direction", "measurement_id" };
	config.sql_table        = "cv_measurements";
	config.raw_data_columns = QStringList{ "measurement_id","voltage_v","capacitance_f" };
	config.raw_data_table   = "cv_raw_data";
	config.columns_to_show  = 8;

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
		for( auto[ measurement_id, metadata ] : id_and_metadata )
		{
			Graph_Measurement( measurement_id, Label_Metadata( metadata, config.what_to_collect ) );
		}
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

	//connect( ui.customPlot, &Interactive_Graph::Graph_Selected, [ this ]( QCPGraph* selected_graph )
	//{
	//	const auto & measurement = ui.customPlot->FindDataFromGraphPointer( selected_graph );
	//	this->ui.selectedName_lineEdit->setText(        Info_Or_Default<QString>( measurement.meta, "Sample Name", "" ) );
	//	this->ui.selectedTemperature_lineEdit->setText( Info_Or_Default<QString>( measurement.meta, "Temperature (K)", "" ) );
	//} );
	connect( ui.semiconductorType_comboBox, &QComboBox::currentTextChanged, [ this ]( const QString & ) { this->Update_Preview_Graph(); } );
	connect( ui.alloyComposition_doubleSpinBox, static_cast<void ( QDoubleSpinBox::* )( double )>( &QDoubleSpinBox::valueChanged ), [ this ]( double ) { this->Update_Preview_Graph(); } );
	connect( ui.semiconductorDoping_lineEdit, &QLineEdit::editingFinished, [ this ]() { this->Update_Preview_Graph(); } );
	connect( ui.insulatorType_comboBox, &QComboBox::currentTextChanged, [ this ]( const QString & ) { this->Update_Preview_Graph(); } );
	connect( ui.insulatorThickness_doubleSpinBox, static_cast<void ( QDoubleSpinBox::* )( double )>( &QDoubleSpinBox::valueChanged ), [ this ]( double ) { this->Update_Preview_Graph(); } );
	connect( ui.interfaceCharge_lineEdit, &QLineEdit::editingFinished, [ this ]() { this->Update_Preview_Graph(); } );
	connect( ui.simulatedTemperature_doubleSpinBox, static_cast<void ( QDoubleSpinBox::* )( double )>( &QDoubleSpinBox::valueChanged ), [ this ]( double ) { this->Update_Preview_Graph(); } );
	connect( ui.metalType_comboBox, &QComboBox::currentTextChanged, [ this ]( const QString & ) { this->Update_Preview_Graph(); } );
	connect( ui.displayHFPreview_checkBox, &QCheckBox::stateChanged, [ this ]( int ) { this->Update_Preview_Graph(); } );
	connect( ui.displayLFPreview_checkBox, &QCheckBox::stateChanged, [ this ]( int ) { this->Update_Preview_Graph(); } );
	connect( ui.addGraph_pushButton, &QPushButton::pressed, [ this ]
	{
		this->simulated_graph_number++;
		this->Update_Preview_Graph();
	} );

	connect( ui.customPlot->xAxis, qOverload<const QCPRange &>( &QCPAxis::rangeChanged ), [ this ]( const QCPRange & ) { this->Update_Preview_Graph(); } );
}

void Plotter::Update_Preview_Graph()
{
	using namespace CV_Measurements;
	if( !ui.displayHFPreview_checkBox->isChecked() )
	{
		ui.customPlot->Hide_Graph( "HF Simulation" + QString::number( this->simulated_graph_number ) );
		//ui.customPlot->replot();
		//return;
	}

	if( !ui.displayLFPreview_checkBox->isChecked() )
	{
		ui.customPlot->Hide_Graph( "LF Simulation" + QString::number( this->simulated_graph_number ) );
		//ui.customPlot->replot();
		//return;
	}

	// Get data from text entries
	std::string semiconductor_selected = ui.semiconductorType_comboBox->currentText().toStdString();
	double semiconductor_alloy_composition = ui.alloyComposition_doubleSpinBox->value();
	bool properly_formatted = false;
	double semiconductor_doping = ui.semiconductorDoping_lineEdit->text().toDouble( &properly_formatted );
	if( !properly_formatted )
		return;
	std::string insulator_selected = ui.insulatorType_comboBox->currentText().toStdString();
	double insulator_thickness = 1E-9 * ui.insulatorThickness_doubleSpinBox->value(); // Convert from nm to meters
	double temperature_in_k = ui.simulatedTemperature_doubleSpinBox->value();
	double interface_charge = ui.interfaceCharge_lineEdit->text().toDouble( &properly_formatted );
	std::string metal_selected = ui.metalType_comboBox->currentText().toStdString();

	std::map<std::string, std::function< Semiconductor()> > semiconductors = {
		{ "HgCdTe", [ = ] { return Semiconductor( HgCdTe( semiconductor_alloy_composition, temperature_in_k ), semiconductor_doping, temperature_in_k ); } }
	};
	std::map<std::string, std::function< Insulator()> > insulators = {
		{ "ZnS",  [ = ] { return Insulator( ZnS,                      insulator_thickness, interface_charge ); } },
		{ "CdTe", [ = ] { return Insulator( CdTe( temperature_in_k ), insulator_thickness, interface_charge ); } },
		{ "ZnO",  [ = ] { return Insulator( ZnO,                      insulator_thickness, interface_charge ); } },
		{ "Al2O3",[ = ] { return Insulator( Al2O3,                    insulator_thickness, interface_charge ); } }
	};
	std::map<std::string, std::function< Metal()> > metals = {
		{ "Aluminum"  , [ = ] { return Aluminum; } },
		{ "Chromium"  , [ = ] { return Chromium; } },
		{ "Gold"      , [ = ] { return Gold;       } },
		{ "Indium"    , [ = ] { return Indium;     } },
		{ "Molybdenum", [ = ] { return Molybdenum; } },
		{ "Nickel"    , [ = ] { return Nickel;     } },
		{ "Platinum"  , [ = ] { return Platinum;   } },
		{ "Titanium"  , [ = ] { return Titanium;   } },
	};
	Semiconductor semiconductor = semiconductors[ semiconductor_selected ]();
	Insulator insulator = insulators[ insulator_selected ]();
	Metal metal = metals[ metal_selected ]();

	// Output calculated material values
	ui.intrinsicCarrierConcentration_lineEdit->setText( QString::number( semiconductor.n_i, 'E', 2 ) + " cm" + QString( QChar( 0x207B ) ) + QString( QChar( 0x00B3 ) ) );
	ui.dielectricConstant_lineEdit->setText( QString::number( semiconductor.eps_s, 'G', 4 ) );
	ui.bandgap_lineEdit->setText( QString::number( semiconductor.bandgap, 'G', 4 ) + " eV" );
	ui.wavelength_lineEdit->setText( QString::number( 1.23984198406 / semiconductor.bandgap, 'G', 4 ) + " " + QString( QChar( 0x03BC ) ) + "m" );
	ui.semiconductorAffinity_lineEdit->setText( QString::number( semiconductor.affinity, 'G', 4 ) + " eV" );
	ui.semiconductorWorkFunction_lineEdit->setText( QString::number( semiconductor.work_function, 'G', 4 ) + " eV" );
	ui.metalWorkFunction_lineEdit->setText( QString::number( metal.work_function, 'G', 4 ) + " eV" );

	//double lower_bound = std::max( 0.0, ui.customPlot->xAxis->range().lower );
	//double upper_bound = std::max( 0.0, ui.customPlot->xAxis->range().upper );
	if( ui.displayHFPreview_checkBox->isChecked() )
	{
		auto[ voltages, capacitances ] = Get_MOS_Capacitance( semiconductor, insulator, metal, temperature_in_k, ui.customPlot->xAxis->range().lower, ui.customPlot->xAxis->range().upper, 1000.0 );
		//capacitances /= insulator.capacitance;
		ui.customPlot->Graph<X_Units::VOLTAGE_V, Y_Units::CAPACITANCE_F>( toQVec( voltages ), toQVec( capacitances ),
																		  "HF Simulation" + QString::number( this->simulated_graph_number ),
																		  QString( "HF %1, t=%2 nm, T=%3 K" ).arg( ui.insulatorType_comboBox->currentText(),
																												   QString::number( insulator_thickness * 1E9, 'f', 2 ),
																												   QString::number( temperature_in_k, 'f', 2 ) ) );
	}
	if( ui.displayLFPreview_checkBox->isChecked() )
	{
		auto[ voltages, capacitances ] = Get_MOS_Capacitance( semiconductor, insulator, metal, temperature_in_k, ui.customPlot->xAxis->range().lower, ui.customPlot->xAxis->range().upper, 0.0 );
		//capacitances /= insulator.capacitance;
		ui.customPlot->Graph<X_Units::VOLTAGE_V, Y_Units::CAPACITANCE_F>( toQVec( voltages ), toQVec( capacitances ),
																		  "LF Simulation" + QString::number( this->simulated_graph_number ),
																		  QString( "LF %1, t=%2 nm, T=%3 K" ).arg( ui.insulatorType_comboBox->currentText(),
																												   QString::number( insulator_thickness * 1E9, 'f', 2 ),
																												   QString::number( temperature_in_k, 'f', 2 ) ) );
	}

	ui.customPlot->replot();
}

void Plotter::treeContextMenuRequest( QPoint pos )
{
	QMenu *menu = new QMenu( this );
	menu->setAttribute( Qt::WA_DeleteOnClose );
	ID_To_Metadata selected = ui.treeWidget->Selected_Data();
	bool only_one_thing_selected = selected.size() == 1;

	menu->addAction( "Graph Selected", [ this, selected ]
	{
		for( auto[ measurement_id, metadata ] : selected )
		{
			Graph_Measurement( measurement_id, Label_Metadata( metadata, config.what_to_collect ) );
		}
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

	//menu->addAction( "Save to csv file", [this, selected]
	//{
	//	this->Save_To_CSV( selected );
	//} );

	menu->popup( ui.treeWidget->mapToGlobal( pos ) );
}

//const QStringList header_titles{ "Sample Name", "Date", "Temperature (K)", "Location", "Side Length", "Time of Day", "Sweep Direction", "measurement_id" };
void Plotter::Graph_Measurement( QString measurement_id, Labeled_Metadata metadata )
{
	sql_manager->Grab_SQL_XY_Data_From_Measurement_IDs( config.raw_data_columns, config.raw_data_table, { measurement_id }, this, [ this, measurement_id, metadata ]( ID_To_XY_Data data )
	{
		auto[ x_data, y_data ] = data[ measurement_id ];
		const auto q = [ &metadata ]( const auto & i ) { return metadata.find( i )->second.toString(); };
		ui.customPlot->Graph<X_Units::VOLTAGE_V, Y_Units::CAPACITANCE_F>(
			x_data, y_data, measurement_id,
			//QString("%1 %2 K %4" + QString( QChar( 0x03BC ) ) + "m: %3").arg( row[0].toString(), row[2].toString(), row[3].toString(), QString::number(int(std::sqrt(row[4].toInt()))) ) );
			QString( "%1 %2 K %4" + QString( QChar( 0x03BC ) ) + "m: %3" ).arg(
				q( "Sample Name" ),
				q( "Temperature (K)" ),
				q( "Location" ),
				q( "Device Side Length (" + QString( QChar( 0x03BC ) ) + "m)" ) ),
			metadata );
		ui.customPlot->replot();
	}, config.sorting_strategy );
}

}