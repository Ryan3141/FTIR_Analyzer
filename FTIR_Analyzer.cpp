#include "FTIR_Analyzer.h"

#include <QSettings>
#include <QFileDialog>
#include <vector>
#include <math.h>

#include "SQL_Tree_Widget.h"
#include "Thin_Film_Interference.h"
#include "Interactive_Graph.h"
#include "SPA_File.h"
#include "Blackbody_Radiation.h"
//#include "Optimize.h"

namespace FTIR
{
const QStringList header_titles{ "Sample Name", "Date", "Temperature (K)", "Dewar Temp (C)", "Time of Day", "Gain", "measurement_id" };
const QStringList what_to_collect{ "sample_name", "date(time)", "temperature_in_k", "dewar_temp_in_c", "time(time)", "gain", "measurement_id" }; // DATE_FORMAT(time, '%b %e %Y') DATE_FORMAT(time, '%H:%i:%s')

const QString sql_table = "ftir_measurements";
const QStringList raw_data_columns{ "measurement_id","wavenumber","intensity" };
const QString raw_data_table = "ftir_raw_data";
const QString sorting_strategy = "ORDER BY measurement_id, wavenumber ASC";
const int columns_to_show = 5;
const int measurement_id_column = what_to_collect.indexOf( "measurement_id" );


static XY_Data Scale_FTIR_XY_Data( const Metadata & meta, const XY_Data & data )
{
	return data;
	int gain = meta[ header_titles.indexOf( "Gain" ) ].toInt();

	QVector<double> scale_y_data = std::get<1>( data );
	for( double& y : scale_y_data )
		y /= gain;

	//for( double& y : scale_y_data )
	//	y *= 100;
	return { std::get<0>( data ), std::move(scale_y_data) };
}

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
	: QMainWindow( parent )
{
	ui.setupUi( this );
	QString config_filename = "configuration.ini";

	Add_Mouse_Position_Label();

	Initialize_Tree_Table();
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
	msgBox->setWindowTitle( "FTIR Analyzer" );
	msgBox->setText( "Attempting SQL connection, please wait..." );
	msgBox->setWindowModality( Qt::NonModal ); // Don't block
	msgBox->show();
	QTimer::singleShot( 500, [this, config_filename, msgBox] // Run this as soon as windows can be loaded
	{
		sql_manager.Connect_To_DB( config_filename );
		msgBox->close();
		//QMetaObject::invokeMethod( this->thin_film_manager, [=] { this->thin_film_manager->Get_Best_Fit( temperature, copy_layers, wavelength_data, transmission_data ); } );
	} );
}

void FTIR_Analyzer::Graph_Blackbody( double temperature_in_k, double amplitude )
{
	double lower_bound = std::max( 0.0, ui.customPlot->xAxis->range().lower );
	double upper_bound = std::max( 0.0, ui.customPlot->xAxis->range().upper );
	arma::vec x_data = arma::linspace( lower_bound, upper_bound, 2049 ).tail( 2048 );
	x_data.transform( [=]( double x ) { return Convert_Units( ui.customPlot->x_axis_units, Unit_Type::WAVELENGTH_MICRONS, x ) * 1E-6; } );
	arma::vec y_blackbody = Blackbody_Radiation( x_data, temperature_in_k, amplitude );
	arma::vec x_data_wavenumber = x_data;
	x_data_wavenumber.transform( [=]( double x ) { return Convert_Units( Unit_Type::WAVELENGTH_MICRONS, Unit_Type::WAVE_NUMBER, x * 1E6 ); } );

	ui.customPlot->Graph( toQVec( x_data_wavenumber ), toQVec( y_blackbody ), "Black Body", "Black Body", false );
	ui.customPlot->replot();
}

void FTIR_Analyzer::Graph_Refractive_Index( Material material, double temperature_in_k, double composition )
{
	double lower_bound = std::max( 0.0, ui.customPlot->xAxis->range().lower );
	double upper_bound = std::max( 0.00000001, ui.customPlot->xAxis->range().upper );
	arma::vec x_data = arma::linspace( lower_bound, upper_bound, 2049 ).tail( 2048 );
	x_data.transform( [=]( double x ) { return Convert_Units( ui.customPlot->x_axis_units, Unit_Type::WAVELENGTH_MICRONS, x ) * 1E-6; } );

	auto x_data_wavenumber = x_data;
	x_data_wavenumber.transform( [=]( double x ) { return Convert_Units( Unit_Type::WAVELENGTH_MICRONS, Unit_Type::WAVE_NUMBER, x * 1E6 ); } );
	auto refractive_index = Thin_Film_Interference().Get_Refraction_Index( material, x_data, temperature_in_k, composition );
	ui.customPlot->Graph( toQVec( x_data_wavenumber ), toQVec( arma::real( refractive_index ) ), "Index (n)", "Index (n)", false );
	ui.customPlot->Graph( toQVec( x_data_wavenumber ), toQVec( arma::imag( refractive_index ) ), "Index (k)", "Index (k)", false );

	ui.customPlot->replot();

}

void FTIR_Analyzer::Graph_Simulation( std::vector<Material_Layer> layers, double temperature_in_k, std::tuple<bool,bool,bool> what_to_plot, double largest_transmission, Material_Layer backside_material )
{
	double lower_bound = std::max( 0.0, ui.customPlot->xAxis->range().lower );
	double upper_bound = std::max( 0.00000001, ui.customPlot->xAxis->range().upper );
	arma::vec x_data = arma::linspace( lower_bound, upper_bound, 2049 ).tail( 2048 ); // Remove zero x value (to avoid divide by zero type errors)
	x_data.transform( [=]( double x ) { return Convert_Units( ui.customPlot->x_axis_units, Unit_Type::WAVELENGTH_MICRONS, x ) * 1E-6; } );

	Thin_Film_Interference tfi;
	try
	{
		auto[ transmission, reflection ] = tfi.Get_Expected_Transmission( temperature_in_k, layers, x_data, backside_material );
		transmission *= largest_transmission;
		reflection *= largest_transmission;

		auto x_data_wavenumber = x_data;
		x_data_wavenumber.transform( [=]( double x ) { return Convert_Units( Unit_Type::WAVELENGTH_MICRONS, Unit_Type::WAVE_NUMBER, x * 1E6 ); } );

		if( std::get<0>( what_to_plot ) )
			ui.customPlot->Graph( toQVec( x_data_wavenumber ), toQVec( transmission ), "Transmission Simulation", "Transmission Simulation", false );
		if( std::get<1>( what_to_plot ) )
			ui.customPlot->Graph( toQVec( x_data_wavenumber ), toQVec( reflection ), "Reflection Simulation", "Reflection Simulation", false );
		if( std::get<2>( what_to_plot ) )
			ui.customPlot->Graph( toQVec( x_data_wavenumber ), toQVec( 100.0 - transmission - reflection ), "Absorption Simulation", "Absorption Simulation", false );
		ui.customPlot->replot();
	}
	catch( ... )
	{
		return;
	}
}

void FTIR_Analyzer::Run_Fit()
{
	Single_Graph selected_graph = ui.customPlot->GetSelectedGraphData();
	if( selected_graph.graph_pointer == nullptr )
		return;

	std::vector<Material_Layer> copy_layers = ui.simulated_listWidget->Build_Material_List();
	arma::vec wavelength_data( &selected_graph.x_data[ 0 ], selected_graph.x_data.size() );
	wavelength_data.transform( [&selected_graph]( double x ) { return Convert_Units( selected_graph.x_units, Unit_Type::WAVELENGTH_MICRONS, x ) * 1E-6; } );
	arma::vec transmission_data( &selected_graph.y_data[ 0 ], selected_graph.y_data.size() );
	for( int i = 0; i < transmission_data.size(); i++ )
		transmission_data( i ) = ui.customPlot->y_display_method( selected_graph.x_data[ i ], selected_graph.y_data[ i ] ); // Do the y stuff first to not screw up x data for y_display_method
	double temperature = ui.simulationTemperature_doubleSpinBox->value();
	std::string material_name = ui.backsideMaterial_comboBox->currentText().toStdString();
	Material_Layer backside_material = { name_to_material[ material_name ], 0.0, 0.0 };
	QMetaObject::invokeMethod( this->thin_film_manager, [=] { this->thin_film_manager->Get_Best_Fit( temperature, copy_layers, wavelength_data, transmission_data, backside_material ); } );
}

void FTIR_Analyzer::Initialize_Tree_Table()
{
	ui.treeWidget->Set_Data_To_Gather( header_titles, what_to_collect, columns_to_show );

	QString user = ui.sqlUser_lineEdit->text();
	ui.treeWidget->Repoll_SQL( this->sql_manager.sql_db, sql_table, user );

	QString filter = ui.filter_lineEdit->text();
	ui.treeWidget->Refilter( filter );

	connect( ui.treeWidget, &SQL_Tree_Widget::itemDoubleClicked, [this]( QTreeWidgetItem* tree_item, int column )
	{
		std::vector<const QTreeWidgetItem*> things_to_graph = ui.treeWidget->Get_Bottom_Children_Elements_Under( tree_item );

		for( const QTreeWidgetItem* x : things_to_graph )
		{
			Graph_Tree_Node( x );
		}
	} );

	// setup policy and connect slot for context menu popup:
	ui.treeWidget->setContextMenuPolicy( Qt::CustomContextMenu );
	connect( ui.treeWidget, &QWidget::customContextMenuRequested, this, &FTIR_Analyzer::treeContextMenuRequest );

	connect( ui.refresh_commandLinkButton, &QCommandLinkButton::clicked,
			 [this]
	{
		ui.treeWidget->Repoll_SQL( this->sql_manager.sql_db, sql_table, this->ui.sqlUser_lineEdit->text() );
		ui.treeWidget->Refilter( this->ui.filter_lineEdit->text() );
	} );
	connect( ui.sqlUser_lineEdit, &QLineEdit::returnPressed,
			 [this]
	{
		ui.treeWidget->Repoll_SQL( this->sql_manager.sql_db, sql_table, this->ui.sqlUser_lineEdit->text() );
		ui.treeWidget->Refilter( this->ui.filter_lineEdit->text() );
	} );
	connect( ui.filter_lineEdit, &QLineEdit::textEdited, ui.treeWidget, [this] { ui.treeWidget->Refilter( this->ui.filter_lineEdit->text() ); } );

	//ui.treeWidget->addAction( new QAction( tr( "Set As Background" ), [this]()// QTreeWidgetItem* tree_item, int column )
	//{
	//	//vector<const QTreeWidgetItem*> things_to_graph = this->Get_All_Children_Full_Tree_Elements_Under( tree_item );

	//	//for( const QTreeWidgetItem* x : things_to_graph )
	//	//{
	//	//	Graph_Tree_Node( x );
	//	//}
	//} ) );
}

QString Info_Or_Default( const Metadata & meta, QString column, QString Default )
{
	if( meta.empty() )
		return Default;
	int index = header_titles.indexOf( column );
	if( index == -1 )
		return Default;
	QVariant stuff = meta[ index ];
	if( stuff == QVariant::Invalid )
		return Default;
	return meta[ index ].toString();
}

void FTIR_Analyzer::Initialize_Graph()
{
	ui.interactiveGraphToolbar->Connect_To_Graph( ui.customPlot );
	connect( ui.customPlot, &Interactive_Graph::Graph_Selected, [this]( QCPGraph* selected_graph )
	{
		const Single_Graph & measurement = ui.customPlot->FindDataFromGraphPointer( selected_graph );
		this->ui.selectedName_lineEdit->setText(        Info_Or_Default( measurement.meta, "Sample Name", "" ) );
		this->ui.selectedTemperature_lineEdit->setText( Info_Or_Default( measurement.meta, "Temperature (K)", "" ) );
		this->ui.selectedCutoff_lineEdit->setText(      Info_Or_Default( measurement.meta, "Gain", "" ) );

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
			Material_Layer backside_material = { name_to_material[ material_name ], 0.0, 0.0 };
			this->Graph_Simulation( ui.simulated_listWidget->Build_Material_List(),
									ui.simulationTemperature_doubleSpinBox->value(),
									{ plot_T, plot_R, plot_A },
									100.0, backside_material );
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
			this->Graph_Blackbody( ui.blackbodyTemperature_horizontalSlider->value() / 100.0, ui.blackbodyAmplitude_horizontalSlider->value() / 1000.0 );
		else
			ui.customPlot->Hide_Graph( "Black Body" );
	};
	auto replot_refractive_index = [this]
	{
		if( this->ui.plotMaterialIndex_checkBox->isChecked() )
		{
			std::string material_name = ui.plotMaterialIndex_comboBox->currentText().toStdString();
			this->Graph_Refractive_Index( name_to_material[ material_name ], ui.simulationTemperature_doubleSpinBox->value() );
		}
		else
		{
			ui.customPlot->Hide_Graph( "Index (n)" );
			ui.customPlot->Hide_Graph( "Index (k)" );
		}
	};

	connect( ui.customPlot->xAxis, qOverload<const QCPRange &>( &QCPAxis::rangeChanged ), [replot_simulation, replot_blackbody, replot_refractive_index]( const QCPRange & ) { replot_simulation();  replot_blackbody(); replot_refractive_index(); } );
	connect( ui.backsideMaterial_comboBox, qOverload<int>( &QComboBox::currentIndexChanged ), [replot_simulation](int){ replot_simulation(); } );
	connect( ui.simulated_listWidget, &Layer_Builder::Materials_List_Changed, [replot_simulation]( const std::vector<Material_Layer> & mats ) { replot_simulation(); } );
	connect( ui.simulationTemperature_doubleSpinBox, qOverload<double>( &QDoubleSpinBox::valueChanged ), [replot_simulation, replot_refractive_index]( double ) { replot_simulation(); replot_refractive_index(); } );
	connect( ui.simulationTransmissionOn_checkBox, &QCheckBox::stateChanged, [replot_simulation]( int ) { replot_simulation(); } );
	connect( ui.simulationReflectionOn_checkBox, &QCheckBox::stateChanged, [replot_simulation]( int ) { replot_simulation(); } );
	connect( ui.simulationAbsorptionOn_checkBox, &QCheckBox::stateChanged, [replot_simulation]( int ) { replot_simulation(); } );

	connect( ui.plotMaterialIndex_comboBox, qOverload<int>( &QComboBox::currentIndexChanged ), [replot_refractive_index]( int ) { replot_refractive_index(); } );
	connect( ui.plotMaterialIndex_checkBox, &QCheckBox::stateChanged, [replot_refractive_index]( int ) { replot_refractive_index(); } );

	connect( ui.blackbodyAmplitude_horizontalSlider, &QSlider::valueChanged, [this, Avoid_Resignalling_setValue, replot_blackbody]( int )
	{
		Avoid_Resignalling_setValue( ui.blackbodyAmplitude_doubleSpinBox, ui.blackbodyAmplitude_horizontalSlider->value() / 1000.0 );
		replot_blackbody();
	} );
	connect( ui.blackbodyTemperature_horizontalSlider, &QSlider::valueChanged, [this, Avoid_Resignalling_setValue, replot_blackbody]( int )
	{
		Avoid_Resignalling_setValue( ui.blackbodyTemperature_doubleSpinBox, ui.blackbodyTemperature_horizontalSlider->value() / 100.0 );
		replot_blackbody();
	} );
	connect( ui.blackbodyTemperature_doubleSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), [this, Avoid_Resignalling_setValue, replot_blackbody]( double )
	{
		Avoid_Resignalling_setValue( ui.blackbodyTemperature_horizontalSlider, ui.blackbodyTemperature_doubleSpinBox->value() * 100.0 );
		replot_blackbody();
	} );
	connect( ui.blackbodyAmplitude_doubleSpinBox,   qOverload<double>(&QDoubleSpinBox::valueChanged), [this, Avoid_Resignalling_setValue, replot_blackbody]( double )
	{
		Avoid_Resignalling_setValue( ui.blackbodyAmplitude_horizontalSlider, ui.blackbodyAmplitude_doubleSpinBox->value() * 1000.0 );
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

		int result = dialog.exec();
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
	
	{ // Start the thread
		QThread* thread = new QThread;
		this->thin_film_manager = new Thin_Film_Interference();
		this->thin_film_manager->moveToThread( thread );
		//connect( this->thin_film_manager, &Thin_Film_Interference::Final_Guess, thread, &QThread::quit );
		//automatically delete thread and task object when work is done:
		connect( thread, &QThread::finished, this->thin_film_manager, &QObject::deleteLater );
		connect( thread, &QThread::finished, thread, &QObject::deleteLater );
		auto test = connect( this->thin_film_manager, &Thin_Film_Interference::Updated_Guess, this, &FTIR_Analyzer::Graph_Simulation );
		thread->start();
	}
}

void FTIR_Analyzer::showPointToolTip( QMouseEvent *event )
{
	double x = ui.customPlot->xAxis->pixelToCoord( event->pos().x() );
	double y = ui.customPlot->yAxis->pixelToCoord( event->pos().y() );

	statusLabel->setText( QString( "%1 , %2" ).arg( x ).arg( y ) );
}

void FTIR_Analyzer::treeContextMenuRequest( QPoint pos )
{
	QMenu *menu = new QMenu( this );
	menu->setAttribute( Qt::WA_DeleteOnClose );
	auto selected = ui.treeWidget->selectedItems();
	auto actually_clicked = ui.treeWidget->itemAt( pos );
	bool only_one_thing_selected = selected.size() == 1 && ui.treeWidget->Get_Bottom_Children_Elements_Under( selected[ 0 ] ).size() == 1;
	if( only_one_thing_selected ) // Only show up if only 1 thing is selected
	{
		menu->addAction( "Set As Background", [this, selected]
		{
			QTreeWidgetItem* first_selected = selected[ 0 ];
			QString measurement_id = first_selected->text( measurement_id_column );
			XY_Data data = sql_manager.Grab_SQL_Data_From_Measurement_IDs( raw_data_columns, raw_data_table, { measurement_id } )[ measurement_id ];
			Metadata row = ui.treeWidget->Get_Metadata_For_Row( first_selected );
			XY_Data background_data = Scale_FTIR_XY_Data( row, data );
			ui.customPlot->Set_As_Background( background_data );
		} );
	}

	menu->addAction( "Clear Background", ui.customPlot, &Interactive_Graph::Clear_Background );

	menu->addAction( "Graph Selected", [this, selected]
	{
		for( const auto & tree_item : selected )
		{
			std::vector<const QTreeWidgetItem*> things_to_graph = ui.treeWidget->Get_Bottom_Children_Elements_Under( tree_item );

			for( const QTreeWidgetItem* x : things_to_graph )
			{
				Graph_Tree_Node( x );
			}
		}
	} );

	menu->addAction( "Save to csv file", [this, selected]
	{
		std::vector<const QTreeWidgetItem*> all_things_to_save;
		for( const auto & tree_item : selected )
		{
			std::vector<const QTreeWidgetItem*> things_to_save = ui.treeWidget->Get_Bottom_Children_Elements_Under( tree_item );

			all_things_to_save.insert( all_things_to_save.end(), things_to_save.begin(), things_to_save.end() );
		}

		this->Save_To_CSV( all_things_to_save );
	} );

	menu->popup( ui.treeWidget->mapToGlobal( pos ) );
}

void FTIR_Analyzer::Save_To_CSV( const std::vector<const QTreeWidgetItem*> & things_to_save )
{
	QFileInfo file_name = QFileDialog::getSaveFileName( this, tr( "Save Data" ), QString(), tr( "CSV File (*.csv)" ) );//;; JPG File (*.jpg);; BMP File (*.bmp);; PDF File (*.pdf)" ) );

	//if( file_name.suffix().toLower() == "csv" )
	{
		int measurment_id_column = ui.treeWidget->columnCount() - 1;

		QVector<double> repeating_x_data;
		std::vector< std::vector<std::string> > data_before_transpose;
		int longest_y_data = 0;
		QStringList measurements_to_graph;
		for( const QTreeWidgetItem* tree_item : things_to_save )
		{
			QString measurement_id = tree_item->text( measurment_id_column );
			measurements_to_graph.push_back( measurement_id );

			Metadata meta_data = sql_manager.Grab_SQL_Metadata_From_Measurement_IDs( what_to_collect, sql_table, { measurement_id } )[ measurement_id ];
			XY_Data data = sql_manager.Grab_SQL_Data_From_Measurement_IDs( raw_data_columns, raw_data_table, { measurement_id } )[ measurement_id ];
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
				QString info = meta_data[ 0 ].toString() + " " + meta_data[ 1 ].toString() + "K";
				current_line.push_back( info.toStdString() );
				longest_y_data = std::max( longest_y_data, y_data.size() );
				for( int i = 0; i < y_data.size(); i++ )
					current_line.push_back( std::to_string( ui.customPlot->y_display_method( x_data[ i ], y_data[ i ] ) ) );
			}
		}


		std::ofstream out_file( file_name.absoluteFilePath().toStdString() );
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
	}
}

void FTIR_Analyzer::Graph_Tree_Node( const QTreeWidgetItem* tree_item )
{
	QString measurement_id = tree_item->text( measurement_id_column );
	Metadata row = ui.treeWidget->Get_Metadata_For_Row( tree_item );

	XY_Data data = sql_manager.Grab_SQL_Data_From_Measurement_IDs( raw_data_columns, raw_data_table, { measurement_id }, sorting_strategy )[ measurement_id ];

	auto[ x, y ] = Scale_FTIR_XY_Data( row, data );
	this->Graph( measurement_id, x, y, QString( "%1 %2 K" ).arg( row[ 0 ].toString(), row[ 2 ].toString() ) );
}

void FTIR_Analyzer::Graph( QString measurement_id, const QVector<double> & x_data, const QVector<double> & y_data, QString data_title, bool allow_y_scaling, Metadata meta )
{
	ui.customPlot->Graph( x_data, y_data, measurement_id, data_title, allow_y_scaling, meta );
	ui.customPlot->replot();
}

void FTIR_Analyzer::Add_Mouse_Position_Label()
{
	QLabel* statusLabel = new QLabel( this );
	statusLabel->setText( "Status Label" );
	ui.statusBar->addPermanentWidget( statusLabel );
	connect( ui.customPlot, &QCustomPlot::mouseMove, [this, statusLabel]( QMouseEvent *event )
	{
		double x = ui.customPlot->xAxis->pixelToCoord( event->pos().x() );
		double y = ui.customPlot->yAxis->pixelToCoord( event->pos().y() );

		statusLabel->setText( QString( "%1 , %2" ).arg( x ).arg( y ) );
	} );
}

}