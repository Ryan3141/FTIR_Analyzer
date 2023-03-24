//#include "FTIR_Analyzer.h"
#include <QtWidgets/QApplication>

#include "IV_By_Size_Plotter.h"
#include "IV_Plotter.h"
#include "CV_Plotter.h"
#include "FTIR_Analyzer.h"
#include "Lifetime_Plotter.h"
#include "Report_Plots.h"

#include "main.h"

void Add_Mouse_Position_Label( QCustomPlot* graph, QLabel* statusLabel )
{
	statusLabel->setText( "Status Label" );
	QObject::connect( graph, &QCustomPlot::mouseMove, [ statusLabel, graph ]( QMouseEvent *event )
	{
		double x = graph->xAxis->pixelToCoord( event->pos().x() );
		double y = graph->yAxis->pixelToCoord( event->pos().y() );

		statusLabel->setText( QString( "%1 , %2" ).arg( x ).arg( y ) );
	} );
}

void Add_Mouse_Position_And_Point_Label( QCustomPlot* graph, QLabel* statusLabel )
{
	statusLabel->setText( "Status Label" );
	QObject::connect( graph, &QCustomPlot::mouseMove, [ statusLabel, graph ]( QMouseEvent *event )
	{
		double x = graph->xAxis->pixelToCoord( event->pos().x() );
		double y = graph->yAxis->pixelToCoord( event->pos().y() );

		statusLabel->setText( QString( "%1 , %2" ).arg( x ).arg( y ) );
	} );
}

Main_Widget::Main_Widget( QWidget *parent )
	: QMainWindow( parent ),
		web_listener( this, QFileInfo( "config.ini" ) )
{
	ui.setupUi( this );

	QLabel* statusLabel = new QLabel( this );
	ui.statusBar->addPermanentWidget( statusLabel );
	Prepare_New_Tab( statusLabel );

	connect( &web_listener, &Web_Listener::Command_Recieved, this, &Main_Widget::Run_Command );
	web_listener.Start_Thread();
}

void Main_Widget::Run_Command( QString command )
{
	auto prepare_new_tab2 = [ this ]( auto* new_tab, QString title )
	{
		int index = ui.main_tabWidget->count() - 1;
		ui.main_tabWidget->insertTab( index, new_tab, title );
		ui.main_tabWidget->setCurrentIndex( index );
		
		connect( new_tab, &Report::Report_Plots::Change_Tab_Name, [ this, new_tab ]( QString new_name )
		{
			int index = ui.main_tabWidget->indexOf( new_tab );
			ui.main_tabWidget->setTabText( index, new_name );
		} );
	};

	QStringList split_by_slash = command.split( '/' );
	if( split_by_slash.size() < 3 )
		return;
	if( split_by_slash[ 1 ].toLower() == "report" )
	{
		prepare_new_tab2( new Report::Report_Plots( this, split_by_slash[2] ), "&Report" );
	}
	// { prepare_new_tab( new FTIR::FTIR_Analyzer( this ), "&FTIR" ); } );
	// { prepare_new_tab( new IV::Plotter( this ), "&IV" ); } );
	// { prepare_new_tab( new Lifetime::Plotter( this ), "&Lifetime" ); } );
	// { prepare_new_tab2( new Report::Report_Plots( this ), "&Report" ); } );
	// { prepare_new_tab( new IV_By_Size::Plotter( this ), "IV By &Size" ); } );
	// { prepare_new_tab( new CV::Plotter( this ), "&CV" ); } );
	// std::cout << "Got command: " << command.toStdString() << "\n";
}

void Main_Widget::Prepare_New_Tab( QLabel* statusLabel )
{
	connect( this->ui.main_tabWidget, &QTabWidget::tabCloseRequested, this->ui.main_tabWidget, &QTabWidget::removeTab );
	auto prepare_new_tab = [ this, statusLabel ]( auto* new_tab, QString title )
	{
		int index = ui.main_tabWidget->count() - 1;
		ui.main_tabWidget->insertTab( index, new_tab, title );
		ui.main_tabWidget->setCurrentIndex( index );
		Add_Mouse_Position_Label( new_tab->ui.customPlot, statusLabel );
	};
	auto prepare_new_tab2 = [ this, statusLabel ]( auto* new_tab, QString title )
	{
		int index = ui.main_tabWidget->count() - 1;
		ui.main_tabWidget->insertTab( index, new_tab, title );
		ui.main_tabWidget->setCurrentIndex( index );
	};
	connect( ui.startFTIR_pushButton,     &QPushButton::clicked, [ this, prepare_new_tab ]( bool checked ) { prepare_new_tab( new FTIR::FTIR_Analyzer( this ), "&FTIR" ); } );
	connect( ui.startIV_pushButton,       &QPushButton::clicked, [ this, prepare_new_tab ]( bool checked ) { prepare_new_tab( new IV::Plotter( this ), "&IV" ); } );
	connect( ui.startLifetime_pushButton, &QPushButton::clicked, [ this, prepare_new_tab ]( bool checked ) { prepare_new_tab( new Lifetime::Plotter( this ), "&Lifetime" ); } );
	connect( ui.startReport_pushButton,   &QPushButton::clicked, [ this, prepare_new_tab2 ]( bool checked ) { prepare_new_tab2( new Report::Report_Plots( this ), "&Report" ); } );
	connect( ui.startIVBySize_pushButton, &QPushButton::clicked, [ this, prepare_new_tab ]( bool checked ) { prepare_new_tab( new IV_By_Size::Plotter( this ), "IV By &Size" ); } );
	connect( ui.startCV_pushButton,       &QPushButton::clicked, [ this, prepare_new_tab ]( bool checked ) { prepare_new_tab( new CV::Plotter( this ), "&CV" ); } );

	ui.main_tabWidget->tabBar()->tabButton( 0, QTabBar::RightSide )->resize( 0, 0 );
}

#include "Ceres_Curve_Fitting.h"
#include "Optimize.h"
bool get_started();
int main(int argc, char *argv[])
{
	google::InitGoogleLogging( argv[ 0 ] );
	if constexpr( false )
	{
		ceres_main( argc, argv );
	}
	if constexpr( false )
	{
		get_started();
	}
	if constexpr( false )
	{
		auto func = []( const arma::vec & fit_params, const arma::vec & x ) -> arma::vec
		{
			return fit_params - x;
			//return ( 1 - x[ 0 ] ) * ( 1 - x[ 1 ] ) * ( 1 - x[ 2 ] );
		};
		const arma::vec x_data = { 1,2,3 };
		const arma::vec y_data = { 0,0,0 };
		const arma::vec lower_bounds = { -10,-10,-10 };
		const arma::vec upper_bounds = { +10,+10,+10 };

		arma::vec best_fit_params = Fit_Data_To_Function( func, x_data, y_data, lower_bounds, upper_bounds,
			1E-5, 1000, 1E-2 );
		std::cout << "Test fit should be 1,2,3: " << best_fit_params[ 0 ] << " " << best_fit_params[ 1 ] << " " << best_fit_params[ 2 ] << "\n";
	}

	if constexpr( false )
	{
		QApplication a( argc, argv );

		SQL_Configuration config;
		//config.header_titles = QStringList{ "Sample Name", "Date", "Temperature (K)", "Dewar Temp (C)", "Time of Day", "Gain", "Bias (V)", "measurement_id" };
		//config.what_to_collect = QStringList{ "sample_name", "date(time)", "temperature_in_k", "dewar_temp_in_c", "time(time)", "gain", "bias_in_v", "measurement_id" }; // DATE_FORMAT(time, '%b %e %Y') DATE_FORMAT(time, '%H:%i:%s')
		config.header_titles = QStringList{ "Sample Name", "Temperature (K)", "Dewar Temp (C)", "Gain", "Bias (V)", "measurement_id" };
		config.what_to_collect = QStringList{ "sample_name", "temperature_in_k", "dewar_temp_in_c", "gain", "bias_in_v", "measurement_id" }; // DATE_FORMAT(time, '%b %e %Y') DATE_FORMAT(time, '%H:%i:%s')
		config.sql_table = "ftir_measurements";
		config.raw_data_columns = QStringList{ "measurement_id","wavenumber","intensity" };
		config.raw_data_table = "ftir_raw_data";
		config.sorting_strategy = "ORDER BY measurement_id, wavenumber ASC";
		config.columns_to_show = 7;

		QString config_filename = "configuration.ini";
		SQL_Manager test( nullptr, config_filename, "Test", "Local_SQL_Cache" );
		//ID_To_Metadata data;
		//data[ "99998" ] = { 1, 2, 3, 4, 5, 6, 7, 99998 };
		//data[ "99999" ] = { 1, 2, 3, 4, 5, 6, 7, 99999 };
		//data[ "99998" ] = { 1, 2, 3, 4, 5, 99998 };
		//data[ "99990" ] = { 1, 2, 3, 4, 5, 99990 };
		ID_To_XY_Data data;
		data[ "99998" ] = { {1}, {2} };
		data[ "99990" ] = { {2, 3, 4}, {3, 4, 5} };
		test.Start_Thread();
		//test.Write_SQL_Metadata( data, config.what_to_collect, config.sql_table );
		test.Write_SQL_XY_Data( data, config.raw_data_columns, config.raw_data_table );
		return a.exec();
	}

	QApplication a(argc, argv);
	Main_Widget w;
	//Ui::Main ui;
	//ui.setupUi( &w );
	//QLabel* statusLabel = new QLabel( &w );
	//ui.statusBar->addPermanentWidget( statusLabel );
	//Prepare_New_Tab( &w, ui, statusLabel );
	//Add_Mouse_Position_Label( ui.ftir_tab->ui.customPlot, statusLabel );
	//Add_Mouse_Position_Label( ui.iv_tab->ui.customPlot, statusLabel );
	//Add_Mouse_Position_Label( ui.ivBySize_tab->ui.customPlot, statusLabel );
	//Add_Mouse_Position_Label( ui.cv_tab->ui.customPlot, statusLabel );
	w.show();
	return a.exec();
}