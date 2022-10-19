#include "Lifetime_Plotter.h"

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
#include "HgCdTe.h"

#include "rangeless_helper.hpp"

template<>
Lifetime::Single_Graph& ::Interactive_Graph<Lifetime::Single_Graph, Lifetime::Axes>::FindDataFromGraphPointer( QCPGraph* graph_pointer )
{
	static Lifetime::Single_Graph nothing_selected;
	auto result = std::find_if(
		this->remembered_graphs.begin(),
		this->remembered_graphs.end(),
		[graph_pointer]( const auto& mo )
		{
			return mo.second.graph_pointer == graph_pointer
				|| mo.second.early_fit_graph == graph_pointer
				|| mo.second.late_fit_graph == graph_pointer;
		} );

	//RETURN VARIABLE IF FOUND
	if ( result != this->remembered_graphs.end() )
		return result->second;
	else
		return nothing_selected;
}


namespace Lifetime
{

static QVector<double> Find_Zero_Crossings( QVector<double> x_data, QVector<double> y_data, double offset )
{
	QVector<double> output;
	for( int i = 1; i < y_data.size(); i++ )
	{
		if( y_data[ i - 1 ] == offset )
			output.push_back( x_data[ i - 1 ] );
		else if( signbit( y_data[ i ] - offset ) != signbit( y_data[ i - 1 ] - offset ) )
			output.push_back( ( x_data[ i ] + x_data[ i - 1 ] ) / 2 );
	}

	return output;
}

template< typename Func >
void Graph_Theoretical_Lifetime( Func func_to_graph, Interactive_Graph* graph, double Cd_Composition, double doping, QString unique_name, QString legend_label )
{
	double lower_bound = std::max( 50.0, graph->xAxis->range().lower );
	double upper_bound = std::max( 300.0, graph->xAxis->range().upper );
	arma::vec temperature_data = arma::linspace( lower_bound, upper_bound, 2048 );
	//temperature_data.transform( [=]( double x ) { return Convert_Units( ui.customPlot->axes.x_units, FTIR::X_Units::WAVELENGTH_MICRONS, x ) * 1E-6; } );
	arma::vec tau_data = func_to_graph( Cd_Composition, temperature_data, doping );
	Single_Graph& single_graph = graph->Graph<X_Units::TEMPERATURE_K, Y_Units::TIME_US>( toQVec( temperature_data ), toQVec( tau_data ), unique_name, legend_label, {} );
	graph->replot();
}

Plotter::Plotter( QWidget *parent )
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
	Initialize_Theoretical_Plots();
}

void Plotter::Initialize_SQL( QString config_filename )
{
	QSettings settings( config_filename, QSettings::IniFormat, this );
	ui.sqlUser_lineEdit->setText( settings.value( "SQL_Server/default_user" ).toString() );
	QMessageBox* msgBox = new QMessageBox( this );

	sql_manager = new SQL_Manager_With_Local_Cache( this, config_filename, "Lifetime" );
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

void Plotter::Graph_Data( const ID_To_Metadata & selected_data )
{
	for( auto[ measurement_id, metadata ] : selected_data )
	{
		auto labeled_metadata = Label_Metadata( metadata, config.what_to_collect );
		sql_manager->Grab_SQL_XY_Blob_Data_From_Measurement_IDs( config.raw_data_columns, config.raw_data_table, { measurement_id }, this,
															[ this, measurement_id, labeled_metadata ]( ID_To_XY_Data data )
		{
			Graph_Measurement( std::move( data ), ui.customPlot, measurement_id, labeled_metadata );
		}, config.sorting_strategy );
	}
}

void Plotter::Initialize_Tree_Table()
{
	config.header_titles = QStringList{ "Sample Name", "T Gain", "Location", "Device Side Length (" + QString( QChar( 0x03BC ) ) + "m)", "Bias (V)", "Temperature (K)", "Date", "Time of Day", "measurement_id" };
	config.what_to_collect = QStringList{ "sample_name", "transimpedance_gain", "device_location", "device_side_length_um", "bias_v", "temperature_in_k", "date(time)", "time(time)", "measurement_id" }; // DATE_FORMAT(time, '%b %e %Y') DATE_FORMAT(time, '%H:%i:%s')
	config.sql_table = "lifetime_measurements";
	config.raw_data_columns = QStringList{ "measurement_id","time_s","voltage_v" };
	config.raw_data_table = "lifetime_raw_data";
	config.sorting_strategy = "";
	config.columns_to_show = 8;

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
	connect( ui.customPlot, &Lifetime::Interactive_Graph::Graph_Selected, [ this ]( QCPGraph* selected_graph )
	{
		Single_Graph & measurement = ui.customPlot->FindDataFromGraphPointer( selected_graph );
		this->ui.selectedName_lineEdit->setText( Info_Or_Default<QString>( measurement.meta, "sample_name", "" ) );
		this->ui.selectedTemperature_lineEdit->setText( Info_Or_Default<QString>( measurement.meta, "temperature_in_k", "" ) );
		this->ui.selectedCutoff_lineEdit->setText( Info_Or_Default<QString>( measurement.meta, "gain", "" ) );

		std::vector<std::tuple<QString, Single_Graph&>> graphs_for_fit = { {Info_Or_Default<QString>( measurement.meta, "measurement_id", "Unknown" ), measurement} };
		std::vector<Graph_Double_Adjustment> label_type_value_list = {
			{ "Lower Fit", measurement.lower_x_fit, [this, &measurement, graphs_for_fit]( double new_value )
				{
					measurement.lower_x_fit = new_value;
					ui.customPlot->Redo_Fits( graphs_for_fit );
					ui.customPlot->replot();
				} },
			{ "Middle Of Fit", measurement.upper_x_fit, [this, &measurement, graphs_for_fit]( double new_value )
				{
					measurement.upper_x_fit = new_value;
					ui.customPlot->Redo_Fits( graphs_for_fit );
					ui.customPlot->replot();
				} },
			{ "Upper Fit", measurement.upper_x_fit2, [this, &measurement, graphs_for_fit]( double new_value )
				{
					measurement.upper_x_fit2 = new_value;
					ui.customPlot->Redo_Fits( graphs_for_fit );
					ui.customPlot->replot();
				} },
		};
		//std::vector<Graph_Adjustment> label_type_value_list = {
		//	{ "Color", [this, &measurement]( const QColor & new_color )
		//		{
		//			auto Recolor_Graph = []( QCPGraph* g, QColor color )
		//			{ QPen copy_pen = g->pen(); copy_pen.setColor( color ); g->setPen( copy_pen ); };
		//			Recolor_Graph( measurement.graph_pointer, new_color );
		//			Recolor_Graph( measurement.early_fit_graph, new_color );
		//			Recolor_Graph( measurement.late_fit_graph, new_color );
		//			ui.customPlot->replot();
		//		} },
		//};

		ui.graphCustomizer->New_Graph_Selected( label_type_value_list );
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

void Plotter::Initialize_Theoretical_Plots()
{
	auto replot_simulation = [this]
	{
		double doping_mantissa = ui.dopingMantissa_doubleSpinBox->value();
		int doping_exponent = ui.dopingExponent_spinBox->value();
		double doping = doping_mantissa * std::pow( 10, doping_exponent );
		double temperature = ui.temperature_doubleSpinBox->value();
		double Cd_Composition = ui.CdComposition_doubleSpinBox->value();

		//if( ui.SRHEnable_checkBox->isChecked() )
		//	Graph_Theoretical_Lifetime( &Auger_Intrinsic_Lifetimes, ui.customPlot, "SRH Lifetime", "SRH Lifetime" );
		//else
		//	ui.customPlot->Hide_Graph( "SRH Lifetime" );

		//if( ui.RadiativeEnable_checkBox->isChecked() )
		//else
		//	ui.customPlot->Hide_Graph( "Radiative Lifetime" );
		if( ui.Auger1Enable_checkBox->isChecked() )
			Graph_Theoretical_Lifetime( &HgCdTe::Auger1_Lifetime<double, arma::vec, double>, ui.customPlot, Cd_Composition, doping, "Auger1 Lifetime", "Auger1 Lifetime" );
		else
			ui.customPlot->Hide_Graph( "Auger1 Lifetime" );
		//if( ui.Auger7Enable_checkBox->isChecked() )
		//else
		//	ui.customPlot->Hide_Graph( "Auger7 Lifetime" );
	};

	connect( ui.customPlot->xAxis, qOverload<const QCPRange&>( &QCPAxis::rangeChanged ), [replot_simulation]( const QCPRange& ) { replot_simulation(); } );
	connect( ui.SRHEnable_checkBox, &QCheckBox::stateChanged, [replot_simulation]( int ) { replot_simulation(); } );
	connect( ui.RadiativeEnable_checkBox, &QCheckBox::stateChanged, [replot_simulation]( int ) { replot_simulation(); } );
	connect( ui.Auger1Enable_checkBox, &QCheckBox::stateChanged, [replot_simulation]( int ) { replot_simulation(); } );
	connect( ui.Auger7Enable_checkBox, &QCheckBox::stateChanged, [replot_simulation]( int ) { replot_simulation(); } );

	connect( ui.dopingMantissa_doubleSpinBox, qOverload<double>( &QDoubleSpinBox::valueChanged ), [replot_simulation]( double ) { replot_simulation(); } );
	connect( ui.dopingExponent_spinBox, qOverload<int>( &QSpinBox::valueChanged ), [replot_simulation]( int ) { replot_simulation(); } );
	connect( ui.temperature_doubleSpinBox, qOverload<double>( &QDoubleSpinBox::valueChanged ), [replot_simulation]( double ) { replot_simulation(); } );
}

void Plotter::treeContextMenuRequest( QPoint pos )
{
	QMenu *menu = new QMenu( this );
	menu->setAttribute( Qt::WA_DeleteOnClose );
	ID_To_Metadata selected = ui.treeWidget->Selected_Data();

	if( !selected.empty() )
		menu->addAction( "Graph Selected", [ this, selected ] { Graph_Data( selected ); } );

	bool only_one_thing_selected = selected.size() == 1;
	if( only_one_thing_selected ) // Only show up if only 1 thing is selected
	{
	}

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


template< typename Func >
QString lookup( const Labeled_Metadata & metadata, const QString & lookup_name, QString the_default, Func run_if_exists )
{
	auto lookup = metadata.find( lookup_name );
	if( lookup == metadata.end() || lookup->second.isNull() )
		return the_default;
	else
		return run_if_exists( lookup->second );
}

void Graph_Measurement( ID_To_XY_Data data, Interactive_Graph* graph, QString measurement_id, Labeled_Metadata metadata, QString legend_label )
{
	const auto &[ x_data, y_data ] = data[ measurement_id ];
	QString used_legend_label = legend_label;
	if( used_legend_label == "" )
	{
		auto to_string = []( QVariant value ) { return value.toString(); };
		QString sample_name = lookup( metadata, "sample_name", "", to_string );
		QString temperature_label = lookup( metadata, "temperature_in_k", "", []( QVariant value ) { return QString::number(std::round(value.toFloat())) + " K"; } );
		QString bias_label = lookup( metadata, "bias_in_v", "", []( QVariant value ) { return QString::number( std::round( value.toFloat() * 1.0E3 ) ) + " mV ";  } );
		QString side_length_label = lookup( metadata, "device_side_length_um", "",
											[]( QVariant value ) { return QString::number( std::round( value.toFloat() ) ) + " " + QString( QChar( 0x03BC ) ) + "m";  } );
		QString device_location = lookup( metadata, "device_location", "", to_string );
		used_legend_label = QStringList{ sample_name, device_location, side_length_label, temperature_label, bias_label }.join( " " );
	}
	Single_Graph & single_graph = graph->Graph<X_Units::TIME_US, Y_Units::VOLTAGE_V>( x_data, y_data, measurement_id, used_legend_label, metadata );
	graph->Redo_Fits( { std::tuple<QString, Single_Graph&>{measurement_id, single_graph} } );
	graph->replot();
}

}