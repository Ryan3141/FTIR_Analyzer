#include "IV_Plotter.h"
#include "CV_Theoretical.h"
#include "Interactive_Graph_Toolbar.h"

using namespace std;

//const QStringList header_titles{ "Sample Name", "Date", "Temperature (K)", "Location", "Device Area", "Time of Day", "Sweep Direction", "measurement_id" };
//const QStringList what_to_collect{ "sample_name", "date(time)", "temperature_in_k", "device_location", "device_area_in_um2", "time(time)", "sweep_direction", "measurement_id" };
//const QString sql_table = "cv_measurements";
//const QStringList raw_data_columns{ "measurement_id","voltage_v","capacitance_f" };
//const QString raw_data_table = "cv_raw_data";
//const int columns_to_show = 6;


namespace IV
{

double Rule_07( double temperature, double cutoff_wavelength )
{
	double J_0 = 8367.000019;
	double Pwr = 0.544071282;
	double C = -1.162972237;
	double lambda_scale = 0.200847413;
	double lambda_threshold = 4.635136423;
	double k_B = 1.3806503e-23;
	double ee = 1.60217646e-19;

	double lambda_e = ( cutoff_wavelength >= lambda_threshold ) ? cutoff_wavelength :
		cutoff_wavelength / ( 1 - std::pow( lambda_scale / cutoff_wavelength - lambda_scale / lambda_threshold, Pwr ) );

	return J_0 * std::exp( C * 1.24 * ee / ( k_B * lambda_e * temperature ) );
}

IV_Plotter::IV_Plotter( QWidget *parent )
	: QWidget( parent )
{
	this->ui.setupUi( this );
	QString config_filename = "configuration.ini";
	Initialize_SQL( config_filename );
	Initialize_Tree_Table(); // sql_manager must be initialized first
	Initialize_Graph();
	Initialize_Rule07();
	Update_Preview_Graph();
	ui.customPlot->refitGraphs();

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

void IV_Plotter::Initialize_SQL( QString config_filename )
{
	QSettings settings( config_filename, QSettings::IniFormat, this );
	ui.sqlUser_lineEdit->setText( settings.value( "SQL_Server/default_user" ).toString() );
	QMessageBox* msgBox = new QMessageBox( this );

	sql_manager = new SQL_Manager( this, config_filename, "IV" );
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

void IV_Plotter::Initialize_Tree_Table()
{
	config.header_titles    = QStringList{ "Sample Name", "Location", "Device Side Length (" + QString( QChar( 0x03BC ) ) + "m)", "Temperature (K)", "Date", "Time of Day", "measurement_id" };
	config.what_to_collect  = QStringList{ "sample_name", "device_location", "device_area_in_um2", "temperature_in_k", "date(time)", "time(time)", "measurement_id" };
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
		for( auto[ measurement_id, metadata ] : id_and_metadata )
		{
			Graph_Measurement( measurement_id, Label_Metadata( metadata, config.header_titles ) );
		}
	} );

	// setup policy and connect slot for context menu popup:
	ui.treeWidget->setContextMenuPolicy( Qt::CustomContextMenu );
	connect( ui.treeWidget, &QWidget::customContextMenuRequested, this, &IV_Plotter::treeContextMenuRequest );

	connect( ui.refresh_commandLinkButton, &QCommandLinkButton::clicked, repoll_sql );
	connect( ui.sqlUser_lineEdit, &QLineEdit::returnPressed, repoll_sql );
	connect( ui.filter_lineEdit, &QLineEdit::textEdited, ui.treeWidget, [ this ] { ui.treeWidget->Refilter( this->ui.filter_lineEdit->text() ); } );

	repoll_sql();
}

void IV_Plotter::Initialize_Graph()
{
	ui.interactiveGraphToolbar->Connect_To_Graph( ui.customPlot );

	connect( ui.customPlot, &QWidget::customContextMenuRequested, ui.customPlot, &Interactive_Graph::graphContextMenuRequest );
	connect( ui.customPlot, &Interactive_Graph::Graph_Selected, [ this ]( QCPGraph* selected_graph )
	{
		const Single_Graph & measurement = ui.customPlot->FindDataFromGraphPointer( selected_graph );
		this->ui.selectedName_lineEdit->setText(        Info_Or_Default<QString>( measurement.meta, "Sample Name", "" ) );
		this->ui.selectedTemperature_lineEdit->setText( Info_Or_Default<QString>( measurement.meta, "Temperature (K)", "" ) );
	} );
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

void IV_Plotter::Update_Preview_Graph()
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
		capacitances /= insulator.capacitance;
		//ui.customPlot->Graph( toQVec( voltages ), toQVec( capacitances ), "HF Simulation" + QString::number( this->simulated_graph_number ),
		//							QString( "HF %1, t=%2 nm, T=%3 K" ).arg( ui.insulatorType_comboBox->currentText(), QString::number( insulator_thickness * 1E9, 'f', 2 ),
		//												QString::number( temperature_in_k, 'f', 2 ) ), true );
	}
	if( ui.displayLFPreview_checkBox->isChecked() )
	{
		auto[ voltages, capacitances ] = Get_MOS_Capacitance( semiconductor, insulator, metal, temperature_in_k, ui.customPlot->xAxis->range().lower, ui.customPlot->xAxis->range().upper, 0.0 );
		capacitances /= insulator.capacitance;
		//ui.customPlot->Graph( toQVec( voltages ), toQVec( capacitances ), "LF Simulation" + QString::number( this->simulated_graph_number ),
		//							QString( "LF %1, t=%2 nm, T=%3 K" ).arg( ui.insulatorType_comboBox->currentText(), QString::number( insulator_thickness * 1E9, 'f', 2 ),
		//												QString::number( temperature_in_k, 'f', 2 ) ), true );
	}

	ui.customPlot->replot();
}

void IV_Plotter::treeContextMenuRequest( QPoint pos )
{
	QMenu *menu = new QMenu( this );
	menu->setAttribute( Qt::WA_DeleteOnClose );
	ID_To_Metadata selected = ui.treeWidget->Selected_Data();
	bool only_one_thing_selected = selected.size() == 1;

	menu->addAction( "Graph Selected", [this, selected]
	{
		for( auto[ measurement_id, metadata ] : selected )
		{
			Graph_Measurement( measurement_id, Label_Metadata( metadata, config.header_titles ) );
		}
	} );

	//menu->addAction( "Save to csv file", [this, selected]
	//{
	//	this->Save_To_CSV( selected );
	//} );

	menu->popup( ui.treeWidget->mapToGlobal( pos ) );
}


void IV_Plotter::Graph_Rule07( double temperature_in_k, double cutoff_wavelength )
{
	// "Sample Name", "Location", "Device Side Length (" + QString( QChar( 0x03BC ) ) + "m)", "Temperature (K)", "Date", "Time of Day", "measurement_id"
	double lower_bound = ui.customPlot->xAxis->range().lower;
	double upper_bound = ui.customPlot->xAxis->range().upper;
	double rule_07 = Rule_07( temperature_in_k, cutoff_wavelength );
	ui.customPlot->Graph( QVector<double>( { lower_bound, upper_bound } ), QVector<double>( { rule_07, rule_07 } ), "Rule 07", Label_Metadata( { "Rule 07", "", 1E4, temperature_in_k, "", "", "" }, config.header_titles ), false );
	ui.customPlot->replot();
}

void IV_Plotter::Initialize_Rule07()
{
	auto replot_rule07 = [ this ]
	{
		if( this->ui.rule07_checkBox->isChecked() )
			this->Graph_Rule07( ui.rule07Temperature_doubleSpinBox->value(), ui.rule07Cutoff_doubleSpinBox->value() );
		else
			ui.customPlot->Hide_Graph( "Rule 07" );
	};
	connect( ui.customPlot->xAxis, qOverload<const QCPRange &>( &QCPAxis::rangeChanged ), [ replot_rule07 ]( const QCPRange & ) { replot_rule07(); } );
	connect( ui.rule07Temperature_doubleSpinBox, qOverload<double>( &QDoubleSpinBox::valueChanged ), [ this, replot_rule07 ]( double )
	{
		replot_rule07();
	} );
	connect( ui.rule07Cutoff_doubleSpinBox, qOverload<double>( &QDoubleSpinBox::valueChanged ), [ this, replot_rule07 ]( double )
	{
		replot_rule07();
	} );
	connect( ui.rule07_checkBox, &QCheckBox::stateChanged, [ replot_rule07 ]( int ) { replot_rule07(); } );

}
//const QStringList header_titles{ "Sample Name", "Date", "Temperature (K)", "Location", "Side Length", "Time of Day", "Sweep Direction", "measurement_id" };
void IV_Plotter::Graph_Measurement( QString measurement_id, Labeled_Metadata metadata )
{
	sql_manager->Grab_SQL_XY_Data_From_Measurement_IDs( config.raw_data_columns, config.raw_data_table, { measurement_id }, this, [ this, measurement_id, metadata ]( ID_To_XY_Data data )
	{
		auto[ x_data, y_data ] = data[ measurement_id ];
		const auto q = [ &metadata ]( const auto & i ) { return metadata.find( i )->second.toString(); };
		ui.customPlot->Graph( x_data, y_data, measurement_id, metadata,
					 //QString("%1 %2 K %4" + QString( QChar( 0x03BC ) ) + "m: %3").arg( row[0].toString(), row[2].toString(), row[3].toString(), QString::number(int(std::sqrt(row[4].toInt()))) ) );
					 QString( "%1 %2 K %4" + QString( QChar( 0x03BC ) ) + "m: %3" ).arg( q( "Sample Name" ),
																						 q( "Temperature (K)" ),
																						 q( "Location" ),
																						 q( "Device Side Length (" + QString( QChar( 0x03BC ) ) + "m)" ) ) );
		ui.customPlot->replot();
	}, config.sorting_strategy );
}

}