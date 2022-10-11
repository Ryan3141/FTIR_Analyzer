#include "FTIR_Analyzer.h"

#include <QSettings>
#include <QFileDialog>
#include <QVariant>
#include <vector>
#include <math.h>

#include "SQL_Tree_Widget.h"
#include "Thin_Film_Interference.h"
#include "Interactive_Graph.h"
#include "SPA_File.h"
#include "Blackbody_Radiation.h"
//#include "Optimize.h"
#include "FTIR_Interactive_Graph.h"
#include "Ceres_Curve_Fitting.h"

#include "rangeless_helper.hpp"

namespace FTIR
{

static QVector<double> Find_Zero_Crossings( QVector<double> x_data, QVector<double> y_data, double offset )
{
	QVector<double> output;
	for( int i = 1; i < y_data.size(); i++ )
	{
		if( y_data[ i - 1 ] == offset )
			output.push_back( x_data[ i - 1 ] );
		else if( signbit( y_data[ i ] - offset ) != signbit( y_data[ i - 1 ] - offset ) )
			output.push_back( (x_data[ i ] + x_data[ i - 1 ]) / 2 );
	}

	return output;
}

FTIR_Analyzer::FTIR_Analyzer( QWidget *parent )
	: QWidget( parent )
{
	ui.setupUi( this );
	QString config_filename = "configuration.ini";

	//connect( ui.fitGraph_pushButton, &QPushButton::clicked, [this]( bool )
	//{
	//	QString file_name = QFileDialog::getOpenFileName( this, tr( "Load Data" ), QString(), tr( "CSV File (*.spa)" ) );
	//	if( file_name.isNull() )
	//	{
	//		qDebug() << "Error reading " + file_name + "\n";
	//		return;
	//	}
	//	Load_From_SPA( file_name.toStdString() );
	//} );

	Initialize_SQL( config_filename );
	Initialize_Tree_Table();
	Initialize_Graph();
	Initialize_Simulation();
	for( auto[ name, material ] : name_to_material )
	{
		ui.plotMaterialIndex_comboBox->addItem( QString::fromStdString( name ) );
		ui.backsideMaterial_comboBox->addItem( QString::fromStdString( name ) );
	}
}

void FTIR_Analyzer::Initialize_SQL( QString config_filename )
{
	QSettings settings( config_filename, QSettings::IniFormat, this );
	ui.sqlUser_lineEdit->setText( settings.value( "SQL_Server/default_user" ).toString() );
	QMessageBox* msgBox = new QMessageBox( this );

	sql_manager = new SQL_Manager_With_Local_Cache( this, config_filename, "FTIR" );
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

void FTIR_Analyzer::Graph_Data( const ID_To_Metadata & selected_data )
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

void FTIR_Analyzer::Graph_Blackbody( double temperature_in_k, double amplitude )
{
	double lower_bound = std::max( 0.0, ui.customPlot->xAxis->range().lower );
	double upper_bound = std::max( 0.0, ui.customPlot->xAxis->range().upper );
	arma::vec x_data_meters = arma::linspace( lower_bound, upper_bound, 2049 ).tail( 2048 );
	x_data_meters.transform( [=]( double x ) { return Convert_Units( ui.customPlot->axes.x_units, FTIR::X_Units::WAVELENGTH_METERS, x ); } );
	arma::vec y_blackbody = Blackbody_Radiation( x_data_meters, temperature_in_k, amplitude );

	ui.customPlot->Graph<FTIR::X_Units::WAVELENGTH_METERS, FTIR::Y_Units::DONT_CHANGE>( toQVec( x_data_meters ), toQVec( y_blackbody ), "Black Body", "Black Body" );
	ui.customPlot->replot();
}

void FTIR_Analyzer::Graph_Refractive_Index( std::string material_name, Optional_Material_Parameters parameters )
{
	double lower_bound = std::max( 0.0, ui.customPlot->xAxis->range().lower );
	double upper_bound = std::max( 0.00000001, ui.customPlot->xAxis->range().upper );
	arma::vec x_data_meters = arma::linspace( lower_bound, upper_bound, 2049 ).tail( 2048 );
	x_data_meters.transform( [=]( double x ) { return Convert_Units( ui.customPlot->axes.x_units, FTIR::X_Units::WAVELENGTH_METERS, x ); } );

	arma::cx_vec refractive_index = Get_Refraction_Index( name_to_material[ material_name ], x_data_meters, parameters );
	ui.customPlot->Graph<FTIR::X_Units::WAVELENGTH_METERS, FTIR::Y_Units::DONT_CHANGE>( toQVec( x_data_meters ), toQVec( arma::real( refractive_index ) ), "Index (n)", "Index (n)" );
	ui.customPlot->Graph<FTIR::X_Units::WAVELENGTH_METERS, FTIR::Y_Units::DONT_CHANGE>( toQVec( x_data_meters ), toQVec( arma::imag( refractive_index ) ), "Index (k)", "Index (k)" );

	ui.customPlot->replot();

}

void FTIR_Analyzer::Graph_Simulation( std::vector<Material_Layer> layers, std::tuple<bool,bool,bool> what_to_plot, double largest_transmission, Material_Layer backside_material )
{
	double lower_bound = std::max( 0.0, ui.customPlot->xAxis->range().lower );
	double upper_bound = std::max( 0.00000001, ui.customPlot->xAxis->range().upper );
	arma::vec x_data_meters = arma::linspace( lower_bound, upper_bound, 2049 ).tail( 2048 ); // Remove zero x value (to avoid divide by zero type errors)
	x_data_meters.transform( [=]( double x ) { return Convert_Units( ui.customPlot->axes.x_units, FTIR::X_Units::WAVELENGTH_MICRONS, x ) * 1E-6; } );

	Thin_Film_Interference tfi;
	try
	{
		constexpr auto X = FTIR::X_Units::WAVELENGTH_METERS;
		constexpr auto Y = FTIR::Y_Units::DONT_CHANGE;
		double transmission_scale = ui.transmissionAmplitude_doubleSpinBox->value();
		double transmission_subtractor = ui.transmissionSubtractor_doubleSpinBox->value();
		double reflection_scale   = ui.reflectionAmplitude_doubleSpinBox->value();
		double absorption_scale   = ui.absorptionAmplitude_doubleSpinBox->value();
		auto[ transmission, reflection ] = Get_Expected_Transmission( layers, x_data_meters, backside_material );
		transmission += transmission_subtractor;
		transmission *= largest_transmission;
		reflection *= largest_transmission;
		if( std::get<0>( what_to_plot ) )
			ui.customPlot->Graph<X, Y>( toQVec( x_data_meters ), toQVec( transmission_scale * transmission ), "Transmission Simulation", "Transmission Simulation" );
		if( std::get<1>( what_to_plot ) )
			ui.customPlot->Graph<X, Y>( toQVec( x_data_meters ), toQVec( reflection_scale * reflection ), "Reflection Simulation", "Reflection Simulation" );
		if( std::get<2>( what_to_plot ) )
			ui.customPlot->Graph<X, Y>( toQVec( x_data_meters ), toQVec( 100.0 * absorption_scale - transmission * transmission_scale - reflection * reflection_scale ), "Absorption Simulation", "Absorption Simulation" );
		ui.customPlot->replot();
	}
	catch( ... )
	{
		return;
	}
}

Material_Layer FTIR_Analyzer::Get_Backside_Material( double temperature_in_k )
{
	std::string material_name = ui.backsideMaterial_comboBox->currentText().toStdString();
	Optional_Material_Parameters backside_parameters( material_name, temperature_in_k );
	Material_Layer backside_material( name_to_material[ material_name ], backside_parameters );

	return backside_material;
}

void FTIR_Analyzer::Run_Fit()
{
	const auto & selected_graph = ui.customPlot->GetSelectedGraphData();
	if( selected_graph.graph_pointer == nullptr )
		return;

	std::array<double, 2> bounds = { ui.customPlot->xAxis->range().lower, ui.customPlot->xAxis->range().upper };
	for( double & x : bounds )
		x = std::max( 0.0, Convert_Units( ui.customPlot->axes.x_units, FTIR::X_Units::WAVELENGTH_MICRONS, x ) * 1E-6 );
	if( bounds[ 0 ] > bounds[ 1 ] )
		std::swap( bounds[ 0 ], bounds[ 1 ] );

	auto[ x_data, y_data ] = ui.customPlot->axes.Prepare_XY_Data( selected_graph );
	arma::vec wavelength_data   = arma::conv_to< arma::vec >::from( x_data.toStdVector() );
	arma::vec transmission_data = arma::conv_to< arma::vec >::from( y_data.toStdVector() );
	wavelength_data = Convert_Units( ui.customPlot->axes.x_units, FTIR::X_Units::WAVELENGTH_MICRONS, wavelength_data ) * 1E-6;
	double temperature_in_k = Info_Or_Default( selected_graph.meta, "Temperature (K)", 300.0 );
	if( temperature_in_k == 0.0 )
		temperature_in_k = 300.0;
	//double temperature = ui.simulationTemperature_doubleSpinBox->value();
	Material_Layer backside_material = Get_Backside_Material( temperature_in_k );
	std::vector<Material_Layer> copy_layers = ui.simulated_listWidget->Build_Material_List( temperature_in_k );

	double transmission_scale = ui.transmissionAmplitude_doubleSpinBox->value();
	//double subtractor = ui.transmissionSubtractor_doubleSpinBox->value();
	//transmission_data -= subtractor;
	//transmission_data /= transmission_scale;
	arma::uvec filter_bounds = arma::find( wavelength_data >= bounds[ 0 ] && wavelength_data <= bounds[ 1 ] );

	if( !arma::any( filter_bounds ) )
		return; // Something went wrong and none of the data is in these bounds
	arma::vec filtered_wavelength_data = wavelength_data( filter_bounds );
	arma::vec filtered_transmission_data = transmission_data( filter_bounds );
	filtered_transmission_data /= 100;
	//double transmission_max = arma::max( filtered_transmission_data );
	//double transmission_max = 100.0;
	std::optional< double > scaled_height = transmission_scale;
	auto fit_parameters = Get_Things_To_Fit( copy_layers );
	fit_parameters.push_back( &scaled_height );
	arma::vec solution = Ceres_Thin_Film_Fit( copy_layers, filtered_wavelength_data, filtered_transmission_data, backside_material );
	for( int i = 0; std::optional< double >*parameter : fit_parameters )
		*parameter = solution[ i++ ];
	ui.simulated_listWidget->Make_From_Material_List( copy_layers );
	ui.transmissionAmplitude_doubleSpinBox->setValue( scaled_height.value() );
	this->Graph_Simulation( copy_layers, { true, false, false }, 100.0, backside_material);
	if constexpr( false )
	{ // Start the thread
		QThread* thread = new QThread;
		this->thin_film_manager = new Thin_Film_Interference();
		//connect( this->thin_film_manager, &Thin_Film_Interference::Final_Guess, thread, &QThread::quit );
		//automatically delete thread and task object when work is done:
		connect( thread, &QThread::finished, this->thin_film_manager, &QObject::deleteLater );
		connect( thread, &QThread::finished, thread, &QObject::deleteLater );
		connect( ui.stopFitting_pushButton, &QPushButton::pressed, this->thin_film_manager, &Thin_Film_Interference::Quit_Early );
		//qRegisterMetaType< std::tuple<bool,bool,bool> >();
		auto test = connect( this->thin_film_manager, &Thin_Film_Interference::Updated_Guess, this, [ = ]( std::vector<Material_Layer> updated_layers )
		{
			ui.simulated_listWidget->Make_From_Material_List( updated_layers );
			this->Graph_Simulation( updated_layers, { true, false, false }, 100.0, backside_material );
		} );
		auto test2 = connect( thread, &QThread::started, this->thin_film_manager, [ = ] { this->thin_film_manager->Get_Best_Fit( copy_layers, filtered_wavelength_data, filtered_transmission_data, backside_material ); } );
		auto test3 = connect( this->thin_film_manager, &Thin_Film_Interference::Debug_Plot, [ this ]( arma::vec wavelengths_m, arma::vec y_data )
		{
			ui.customPlot->Graph<FTIR::X_Units::WAVELENGTH_METERS, FTIR::Y_Units::DONT_CHANGE>( toQVec( std::move( wavelengths_m ) ), toQVec( std::move( y_data ) ), "Debug", "Debug" );
		} );
		this->thin_film_manager->moveToThread( thread );
		thread->start();
	}

	//QMetaObject::invokeMethod( this->thin_film_manager, [=] { this->thin_film_manager->Get_Best_Fit( temperature, copy_layers, wavelength_data, transmission_data, backside_material ); } );
}

void FTIR_Analyzer::Initialize_Tree_Table()
{
	config.header_titles    = QStringList{ "Date", "Sample Name", "Temperature (K)", "Device Side Length (" + QString( QChar( 0x03BC ) ) + "m)", "Location", "Bias (V)", "Time of Day", "T Gain", "Gain", "Dewar Temp (C)", "measurement_id" };
	config.what_to_collect  = QStringList{ "date(time)", "sample_name", "temperature_in_k", "device_side_length_in_um", "device_location", "bias_in_v", "time(time)", "transimpedance_gain", "gain", "dewar_temp_in_c", "measurement_id" }; // DATE_FORMAT(time, '%b %e %Y') DATE_FORMAT(time, '%H:%i:%s')
	config.sql_table        = "ftir_measurements";
	config.raw_data_columns = QStringList{ "measurement_id","wavenumber","intensity" };
	config.raw_data_table   = "ftir_raw_data";
	config.sorting_strategy = "ORDER BY measurement_id, wavenumber ASC";
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

	connect( ui.treeWidget, &SQL_Tree_Widget::Data_Double_Clicked, [this]( ID_To_Metadata id_and_metadata )
	{
		Graph_Data( id_and_metadata );
	} );

	// setup policy and connect slot for context menu popup:
	ui.treeWidget->setContextMenuPolicy( Qt::CustomContextMenu );
	connect( ui.treeWidget, &QWidget::customContextMenuRequested, this, &FTIR_Analyzer::treeContextMenuRequest );

	connect( ui.refresh_commandLinkButton, &QCommandLinkButton::clicked, repoll_sql );
	connect( ui.sqlUser_lineEdit, &QLineEdit::returnPressed, repoll_sql );
	connect( ui.filter_lineEdit, &QLineEdit::textEdited, ui.treeWidget, [this] { ui.treeWidget->Refilter( this->ui.filter_lineEdit->text() ); } );

	repoll_sql();
}

void FTIR_Analyzer::Initialize_Graph()
{
	ui.interactiveGraphToolbar->Connect_To_Graph( ui.customPlot );
	connect( ui.customPlot, &FTIR::Interactive_Graph::Graph_Selected, [this]( QCPGraph* selected_graph )
	{
		const auto & measurement = ui.customPlot->FindDataFromGraphPointer( selected_graph );
		this->ui.selectedName_lineEdit->setText(        Info_Or_Default<QString>( measurement.meta, "sample_name", "" ) );
		this->ui.selectedTemperature_lineEdit->setText( Info_Or_Default<QString>( measurement.meta, "temperature_in_k", "" ) );
		this->ui.selectedCutoff_lineEdit->setText(      Info_Or_Default<QString>( measurement.meta, "gain", "" ) );

		//	QVector<double> cutoffs = Find_Zero_Crossings( x_data, y_data, *std::max_element( y_data.constBegin(), y_data.constEnd() ) / 2 );
		//	if( !cutoffs.isEmpty() )
		//	{
		//		QStringList all_cutoffs;
		//		for( double cutoff : cutoffs )
		//			all_cutoffs.push_back( QString::number( cutoff ) );
		//		this->ui.selectedCutoff_lineEdit->setText( all_cutoffs.join( ", " ) );
		//	}
		//}
	} );
}

void FTIR_Analyzer::Initialize_Simulation()
{
	connect( ui.fitGraph_pushButton, &QPushButton::clicked, this, &FTIR_Analyzer::Run_Fit );

	ui.simulated_listWidget->Set_Material_List( name_to_material );

	auto Avoid_Resignalling_setValue = []( auto going_to_change, auto value )
	{
		bool oldState = going_to_change->blockSignals( true ); // Prevent remove from triggering another changed signal
		going_to_change->setValue( value );
		going_to_change->blockSignals( oldState );
	};

	auto replot_simulation = [this]
	{
		bool plot_T = ui.simulationTransmissionOn_checkBox->isChecked();
		bool plot_R = ui.simulationReflectionOn_checkBox->isChecked();
		bool plot_A = ui.simulationAbsorptionOn_checkBox->isChecked();
		if( plot_T || plot_R || plot_A )
		{
			std::string material_name = ui.backsideMaterial_comboBox->currentText().toStdString();
			double temperature_in_k = ui.simulationTemperature_doubleSpinBox->value();
			this->Graph_Simulation( ui.simulated_listWidget->Build_Material_List( temperature_in_k ),
									{ plot_T, plot_R, plot_A },
									100.0, Get_Backside_Material( temperature_in_k ) );
		}
		if( !plot_T )
			ui.customPlot->Hide_Graph( "Transmission Simulation" );
		if( !plot_R )
			ui.customPlot->Hide_Graph( "Reflection Simulation" );
		if( !plot_A )
			ui.customPlot->Hide_Graph( "Absorption Simulation" );
	};
	auto replot_blackbody = [this]
	{
		if( this->ui.blackbodyOn_checkBox->isChecked() )
			this->Graph_Blackbody( ui.blackbodyTemperature_doubleSpinBox->value(), ui.blackbodyAmplitude_doubleSpinBox->value() );
		else
			ui.customPlot->Hide_Graph( "Black Body" );
	};
	auto replot_refractive_index = [this]
	{
		if( this->ui.plotMaterialIndex_checkBox->isChecked() )
		{
			std::string material_name = ui.plotMaterialIndex_comboBox->currentText().toStdString();
			double temperature_in_k = ui.simulationTemperature_doubleSpinBox->value();
			Optional_Material_Parameters parameters( material_name, temperature_in_k, std::nullopt, 0.221 );

			this->Graph_Refractive_Index( material_name, parameters );
		}
		else
		{
			ui.customPlot->Hide_Graph( "Index (n)" );
			ui.customPlot->Hide_Graph( "Index (k)" );
		}
	};

	connect( ui.customPlot->xAxis, qOverload<const QCPRange &>( &QCPAxis::rangeChanged ), [replot_simulation, replot_blackbody, replot_refractive_index]( const QCPRange & ) { replot_simulation();  replot_blackbody(); replot_refractive_index(); } );
	connect( ui.backsideMaterial_comboBox, qOverload<int>( &QComboBox::currentIndexChanged ), [replot_simulation](int){ replot_simulation(); } );
	connect( ui.simulated_listWidget, &Layer_Builder::Materials_List_Changed, [replot_simulation] { replot_simulation(); } );
	connect( ui.simulationTemperature_doubleSpinBox, qOverload<double>( &QDoubleSpinBox::valueChanged ), [replot_simulation, replot_refractive_index]( double ) { replot_simulation(); replot_refractive_index(); } );
	connect( ui.simulationTransmissionOn_checkBox, &QCheckBox::stateChanged, [replot_simulation]( int ) { replot_simulation(); } );
	connect( ui.simulationReflectionOn_checkBox, &QCheckBox::stateChanged, [replot_simulation]( int ) { replot_simulation(); } );
	connect( ui.simulationAbsorptionOn_checkBox, &QCheckBox::stateChanged, [replot_simulation]( int ) { replot_simulation(); } );

	connect( ui.transmissionAmplitude_doubleSpinBox, qOverload<double>( &QDoubleSpinBox::valueChanged ), [ replot_simulation ]( double ) { replot_simulation(); } );
	connect( ui.transmissionSubtractor_doubleSpinBox, qOverload<double>( &QDoubleSpinBox::valueChanged ), [ replot_simulation ]( double ) { replot_simulation(); } );
	connect( ui.reflectionAmplitude_doubleSpinBox, qOverload<double>( &QDoubleSpinBox::valueChanged ), [ replot_simulation ]( double ) { replot_simulation(); } );
	connect( ui.absorptionAmplitude_doubleSpinBox, qOverload<double>( &QDoubleSpinBox::valueChanged ), [ replot_simulation ]( double ) { replot_simulation(); } );

	connect( ui.plotMaterialIndex_comboBox, qOverload<int>( &QComboBox::currentIndexChanged ), [replot_refractive_index]( int ) { replot_refractive_index(); } );
	connect( ui.plotMaterialIndex_checkBox, &QCheckBox::stateChanged, [replot_refractive_index]( int ) { replot_refractive_index(); } );

	connect( ui.blackbodyAmplitude_horizontalSlider, &QSlider::valueChanged, [this, Avoid_Resignalling_setValue, replot_blackbody]( int )
	{
		int one_to_1E5 = ui.blackbodyAmplitude_horizontalSlider->value();
		const int max = 100000;
		const double curvature = 8.0;
		const double max_y = 200.0;
		double scaled = max_y * std::exp( curvature * ( one_to_1E5 - max ) / max );
		Avoid_Resignalling_setValue( ui.blackbodyAmplitude_doubleSpinBox, scaled );
		replot_blackbody();
	} );
	connect( ui.blackbodyTemperature_horizontalSlider, &QSlider::valueChanged, [this, Avoid_Resignalling_setValue, replot_blackbody]( int )
	{
		int one_to_1E6 = ui.blackbodyTemperature_horizontalSlider->value();
		const int max = 1000000;
		const double curvature = 4.0;
		const double max_y = 6000.0;
		double scaled = max_y * std::exp( curvature * ( one_to_1E6 - max ) / max );
		Avoid_Resignalling_setValue( ui.blackbodyTemperature_doubleSpinBox, scaled );
		replot_blackbody();
	} );
	connect( ui.blackbodyTemperature_doubleSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), [this, Avoid_Resignalling_setValue, replot_blackbody]( double )
	{
		const int max = 1000000;
		const double curvature = 4.0;
		const double max_y = 6000.0;
		double box = ui.blackbodyTemperature_doubleSpinBox->value();
		int inverse = std::log( box / max_y ) * max / curvature + max;
		Avoid_Resignalling_setValue( ui.blackbodyTemperature_horizontalSlider, std::max( 1, std::min( max, inverse ) ) );
		replot_blackbody();
	} );
	connect( ui.blackbodyAmplitude_doubleSpinBox,   qOverload<double>(&QDoubleSpinBox::valueChanged), [this, Avoid_Resignalling_setValue, replot_blackbody]( double )
	{
		const int max = 100000;
		const double curvature = 8.0;
		const double max_y = 200.0;
		double box = ui.blackbodyAmplitude_doubleSpinBox->value();
		int inverse = std::log( box / max_y ) * max / curvature + max;
		Avoid_Resignalling_setValue( ui.blackbodyAmplitude_horizontalSlider, std::max( 1, std::min( max, inverse ) ) );
		replot_blackbody();
	} );
	connect( ui.blackbodyOn_checkBox, &QCheckBox::stateChanged, [replot_blackbody]( int ) { replot_blackbody(); } );

	connect( ui.loadLayersFile_pushButton, &QPushButton::pressed, [this]
	{
		QFileDialog dialog( this, tr( "Load Layers File" ), QString(), tr( "Comma Separated File (*.csv)" ) );
		dialog.setAcceptMode( QFileDialog::AcceptOpen );
		QString current_text = ui.layersFile_lineEdit->text();
		if( current_text == "" )
			dialog.selectFile( "." );
		else
			dialog.selectFile( QFileInfo( current_text ).baseName() );

		auto result = dialog.exec();
		if( result != QDialog::Accepted )
			return;

		QString file_path_string = dialog.selectedFiles()[ 0 ];
		QFileInfo full_file_path( file_path_string );
		ui.simulated_listWidget->Load_From_File( full_file_path );
		ui.layersFile_lineEdit->setText( file_path_string );
	} );
	connect( ui.saveLayersFile_pushButton, &QPushButton::pressed, [this]
	{
		QFileDialog dialog( this, tr( "Load Layers File" ), QString(), tr( "Comma Separated File (*.csv)" ) );
		dialog.setAcceptMode( QFileDialog::AcceptSave );
		QString current_text = ui.layersFile_lineEdit->text();
		if( current_text == "" )
			dialog.selectFile( "." );
		else
			dialog.selectFile( QFileInfo( current_text ).baseName() );

		int result = dialog.exec();
		if( result != QDialog::Accepted )
			return;

		QString file_path_string = dialog.selectedFiles()[ 0 ];
		QFileInfo full_file_path( file_path_string );
		ui.simulated_listWidget->Save_To_File( full_file_path );
		ui.layersFile_lineEdit->setText( file_path_string );
	} );
}

void FTIR_Analyzer::treeContextMenuRequest( QPoint pos )
{
	QMenu *menu = new QMenu( this );
	menu->setAttribute( Qt::WA_DeleteOnClose );
	ID_To_Metadata selected = ui.treeWidget->Selected_Data();

	if( !selected.empty() )
		menu->addAction( "Graph Selected", [ this, selected ] { Graph_Data( selected ); } );

	bool only_one_thing_selected = selected.size() == 1;
	if( only_one_thing_selected ) // Only show up if only 1 thing is selected
	{
		menu->addAction( "Set As Background", [this, selected]
		{
			auto [measurement_id, metadata] = *selected.begin();
			sql_manager->Grab_SQL_XY_Data_From_Measurement_IDs( config.raw_data_columns, config.raw_data_table, { measurement_id }, this, [ this, measurement_id=measurement_id, metadata=metadata ]( ID_To_XY_Data data )
			{
				XY_Data background_data = data[ measurement_id ];
				ui.customPlot->axes.Set_As_Background( std::move( background_data ) );
			}, config.sorting_strategy );
		} );
	}

	menu->addAction( "Clear Background", ui.customPlot, [ this ] { ui.customPlot->axes.Set_Y_Units( FTIR::Y_Units::RAW_SENSOR ); } );

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

void FTIR_Analyzer::Save_To_CSV( const ID_To_Metadata & things_to_save )
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
			for( const auto [ measurement_id, metadata ] : fn::zip( measurement_ids_to_graph, measurements_meta_data ) )
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
					current_line.push_back( "Wavenumber" );
					for( double x : x_data )
						current_line.push_back( std::to_string( x ) );
				}

				{
					data_before_transpose.resize( data_before_transpose.size() + 1 );
					std::vector<std::string> & current_line = data_before_transpose.back();
					QString info = metadata[ 0 ].toString() + " " + metadata[ 2 ].toString() + "K " + metadata[ 6 ].toString() + "V";
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

void Graph_Measurement( ID_To_XY_Data data, Graph_Base* graph, QString measurement_id, Labeled_Metadata metadata, QString legend_label )
{
	const auto &[ x_data, y_data ] = data[ measurement_id ];
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
	graph->Graph<FTIR::X_Units::WAVE_NUMBER, FTIR::Y_Units::RAW_SENSOR>( x_data, y_data, measurement_id, used_legend_label, metadata );
	graph->replot();
}


}