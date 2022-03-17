#include "IV_By_Size_Plotter.h"

#include <algorithm>
#include <armadillo>

#include "Units.h"

#include "rangeless_helper.hpp"

//static QString Unit_Names[ 3 ] = { "Wave Number (cm" + QString( QChar( 0x207B ) ) + QString( QChar( 0x00B9 ) ) + ")",
//							"Wavelength (" + QString( QChar( 0x03BC ) ) + "m)",
//							"Photon Energy (eV)" };

namespace IV
{
double Rule_07( double temperature, double cutoff_wavelength, double device_length_um );
}

namespace IV_By_Size
{

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
}

void Plotter::Initialize_SQL( QString config_filename )
{
	QSettings settings( config_filename, QSettings::IniFormat, this );
	ui.sqlUser_lineEdit->setText( settings.value( "SQL_Server/default_user" ).toString() );

	sql_manager = new SQL_Manager_With_Local_Cache( this, config_filename, "IV_By_Size" );
	sql_manager->Start_Thread();
}

void Plotter::Initialize_Tree_Table()
{
	config.header_titles = QStringList{ "Sample Name", "Temperature (K)", "Date", "Time of Day", "Location", "Device Side Length (" + QString( QChar( 0x03BC ) ) + "m)", "measurement_id" };
	config.what_to_collect = QStringList{ "sample_name", "temperature_in_k", "date(time)", "time(time)", "device_location", "device_side_length_in_um", "measurement_id" };
	config.sql_table = "iv_measurements";
	config.raw_data_columns = QStringList{ "measurement_id","voltage_v","current_a" };
	config.raw_data_table = "iv_raw_data";
	config.sorting_strategy = "ORDER BY measurement_id, voltage_v ASC";
	config.columns_to_show = 6;

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
		Graph_Measurement( id_and_metadata, config.what_to_collect );
	} );

	// setup policy and connect slot for context menu popup:
	ui.treeWidget->setContextMenuPolicy( Qt::CustomContextMenu );
	connect( ui.treeWidget, &QWidget::customContextMenuRequested, this, &Plotter::treeContextMenuRequest );

	connect( ui.refresh_commandLinkButton, &QCommandLinkButton::clicked, repoll_sql );
	connect( ui.sqlUser_lineEdit, &QLineEdit::returnPressed, repoll_sql );
	connect( ui.filter_lineEdit, &QLineEdit::textEdited, ui.treeWidget, [ this ] { ui.treeWidget->Refilter( this->ui.filter_lineEdit->text() ); } );
	connect( ui.measureAtBias_doubleSpinBox, qOverload<double>( &QDoubleSpinBox::valueChanged ), [ this ]( double new_voltage_mV )
	{
		double voltage_v = ui.measureAtBias_doubleSpinBox->value() * 1E-3;
		ui.customPlot->axes.set_voltage = voltage_v;
		ui.customPlot->RegraphAll();
	} );
	connect( ui.showLinearFits_checkBox, &QCheckBox::stateChanged, [ this ]( int new_state ) { ui.customPlot->Hide_Fit_Graphs( Qt::Checked != new_state ); } );

	repoll_sql();
}

void Plotter::Initialize_Graph()
{
	ui.interactiveGraphToolbar->Connect_To_Graph( ui.customPlot );
}

void Plotter::treeContextMenuRequest( QPoint pos )
{
	QMenu *menu = new QMenu( this );
	menu->setAttribute( Qt::WA_DeleteOnClose );
	ID_To_Metadata selected = ui.treeWidget->Selected_Data();
	bool only_one_thing_selected = selected.size() == 1;

	menu->addAction( "Graph Selected", [ this, selected ]
	{
		Graph_Measurement( selected, config.what_to_collect );
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


void Plotter::Graph_Rule07( double dark_current_a_cm2, double temperature_in_k )
{
	// "Sample Name", "Location", "Device Side Length (" + QString( QChar( 0x03BC ) ) + "m)", "Temperature (K)", "Date", "Time of Day", "measurement_id"
	double lower_bound = ui.customPlot->xAxis->range().lower;
	double upper_bound = ui.customPlot->xAxis->range().upper;
	ui.customPlot->Show_Reference_Graph( "Rule 07", QVector<double>( { lower_bound, upper_bound } ), QVector<double>( { dark_current_a_cm2, dark_current_a_cm2 } ) );
	ui.customPlot->replot();
}

void Plotter::Initialize_Rule07()
{
	// Initialize Rule 07 Box
	double rule_07 = IV::Rule_07( ui.rule07Temperature_doubleSpinBox->value(), ui.rule07Cutoff_doubleSpinBox->value(), ui.rule07DeviceLength_doubleSpinBox->value() );
	ui.rule07DarkCurrent_lineEdit->setText( QString::number( rule_07, 'E', 2 ) + QString::fromWCharArray( L" A/cm\u00B2" ) );

	auto replot_rule07 = [ this ]
	{
		double temperature_in_k = ui.rule07Temperature_doubleSpinBox->value();
		double rule_07 = IV::Rule_07( temperature_in_k, ui.rule07Cutoff_doubleSpinBox->value(), ui.rule07DeviceLength_doubleSpinBox->value() );
		ui.rule07DarkCurrent_lineEdit->setText( QString::number( rule_07, 'E', 4 ) + QString::fromWCharArray( L" A/cm\u00B2" ) );
		if( this->ui.rule07_checkBox->isChecked() )
			this->Graph_Rule07( rule_07, temperature_in_k );
		else
			ui.customPlot->Hide_Reference_Graph( "Rule 07" );
		ui.customPlot->replot();
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
//const QStringList header_titles{ "Sample Name", "Date", "Temperature (K)", "Location", "Side Length", "Time of Day", "Sweep Direction", "measurement_id" };
void Plotter::Graph_Measurement( ID_To_Metadata selected, QStringList column_names )
{
	std::vector<Metadata> data = selected
		% fn::transform( []( const auto & x ) { const auto &[ measurement_id, meta ] = x; return meta; } )
		% fn::to( std::vector<Metadata>{} );
	Graph_IV_By_Size_Raw_Data( this, this->config, ui.customPlot, { column_names, data } );
}

}