#include "FTIR_Analyzer.h"

#include <QSettings>
#include <QFileDialog>
#include <vector>
#include <math.h>

#include "Thin_Film_Interference.h"

using namespace std;

// from: http://www.qtcentre.org/threads/55363-QTreeView-and-Column-Row-Gridlines
//class GridDelegate : public QStyledItemDelegate
//{
//public:
//	explicit GridDelegate( QObject * parent = 0 ) : QStyledItemDelegate( parent ) {}
//
//	void paint( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const
//	{
//		painter->save();
//		painter->setPen( QColor( Qt::black ) );
//		painter->drawRect( option.rect );
//		painter->restore();
//
//		QStyledItemDelegate::paint( painter, option, index );
//	}
//};
//
static QString Sanitize_SQL( const QString & raw_string )
{ // Got regex from https://stackoverflow.com/questions/9651582/sanitize-table-column-name-in-dynamic-sql-in-net-prevent-sql-injection-attack
	if( raw_string.contains( ';' ) )
		return QString();

	QRegularExpression re( R"(^[\p{L}{\p{Nd}}$#_][\p{L}{\p{Nd}}@$#_]*$)" );
	QRegularExpressionMatch match = re.match( raw_string );
	bool hasMatch = match.hasMatch();

	if( hasMatch )
		return raw_string;
	else
		return QString();

}

FTIR_Analyzer::FTIR_Analyzer( QWidget *parent )
	: QMainWindow( parent )
{
	srand( 0 );
	ui.setupUi( this );

	Initialize_SQL();
	connect( ui.customPlot, &QCustomPlot::mouseMove, this, &FTIR_Analyzer::showPointToolTip );
	//ui.customPlot->rescaleAxes();

	statusLabel = new QLabel( this );
	statusLabel->setText( "Status Label" );
	ui.statusBar->addPermanentWidget( statusLabel );

	//ui.treeWidget->setItemDelegate( new GridDelegate( ui.treeWidget ) );
	Initialize_Tree_Table();

	connect( ui.refresh_commandLinkButton, &QCommandLinkButton::clicked, this, &FTIR_Analyzer::Reinitialize_Tree_Table );
	connect( ui.sqlUser_lineEdit, &QLineEdit::returnPressed, this, &FTIR_Analyzer::Reinitialize_Tree_Table );
	connect( ui.filter_lineEdit, &QLineEdit::textEdited, this, &FTIR_Analyzer::Reinitialize_Tree_Table );
	connect( ui.fitGraph_pushButton, &QPushButton::clicked, this, &FTIR_Analyzer::Run_Fit );

	Initialize_Graph();
	

	QStringList material_names;
//	for( const auto & entry : name_to_material )
	for( const auto &[ name, material ] : name_to_material )
		material_names.push_back( QString::fromStdString(name) );

	ui.simulated_listWidget->Set_Material_List( material_names );

	connect( ui.simulated_listWidget, &Layer_Builder::Materials_List_Changed, this, &FTIR_Analyzer::Graph_Simulation );

	//setGeometry( QApplication::desktop()->availableGeometry().x(),
	//			 QApplication::desktop()->availableGeometry().y(),
	//			 QApplication::desktop()->availableGeometry().width(),
	//			 QApplication::desktop()->availableGeometry().height() );
}

void FTIR_Analyzer::Graph_Simulation( const std::vector<Material_Layer> & layers )
{
	//auto x_data = arma::linspace( 6.5E-6, 6.8E-6, 1001 );
	//auto x_data = arma::linspace( 1E-9, 10E-6, 10001 );
	ui.customPlot->x_display_method;
	double lower_bound = 1 / (100. * 100E-9);
	double upper_bound = 1 / (100. * 100E-9);
	auto x_data = arma::linspace( 100E-9, 25E-6, 10001 );
	Thin_Film_Interference tfi;
	//auto y_data = tfi.Get_Expected_Transmission( std::vector<Material_Layer>{
	//	{ Material::Si, 500E-6 },
	//		//{ Material::HgCdTe, 0, 1E-6 },
	//		{ Material::ZnS, 0, 10E-6 },
	//		{ Material::ZnSe, 0, 20E-6 } }, x_data );

	auto y_data = tfi.Get_Expected_Transmission( 300., layers, x_data );

	//auto y_data = tfi.Get_Expected_Transmission( std::vector<Material_Layer>{
	//	{ Material::TestA, 1E-6 },
	//	{ Material::TestB, 2E-6 } }, x_data );

	//auto y_data = tfi.Get_Expected_Transmission( std::vector<Material_Layer>{
	//	{ Material::Si, 1000E-6 },
	//	 }, x_data );

	x_data = 1 / (100. * x_data);
	using stdvec = std::vector<double>;
	this->Graph( "Test", QVector<double>::fromStdVector( arma::conv_to<stdvec>::from( x_data ) ), QVector<double>::fromStdVector( y_data ), "Simulation", false );
}

void FTIR_Analyzer::Run_Fit()
{

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

std::map<QString, QString> FTIR_Analyzer::Grab_SQL_Metadata_From_Measurement( const QString & measurement_id )
{
	QSqlQuery query( this->sql_db );
	QStringList what_to_collect{ "sample_name", "time", "temperature_in_k", "bias_in_v" };
	query.prepare( QString( "SELECT %1 FROM ftir_measurements WHERE measurement_id = \"%2\"" ).arg( what_to_collect.join( ',' ), measurement_id ) );
	if( !query.exec() )
	{
		qDebug() << "Error pulling data from ftir_measurments: "
			<< query.lastError();
		return std::map<QString, QString>();
	}

	map<QString, QString> data;
	if( query.next() )
	{
		int i = 0;
		for( QString header : what_to_collect )
			data[ header ] = query.value( i++ ).toString();
	}

	return data;
}

void FTIR_Analyzer::Initialize_Graph()
{
	connect( ui.customPlot, &QWidget::customContextMenuRequested, ui.customPlot, &Interactive_Graph::graphContextMenuRequest );
	connect( ui.customPlot, &Interactive_Graph::Graph_Selected,[this]( QCPGraph* selected_graph )
	{
		QString measurement_id = selected_graph->name();
		std::map<QString, QString> data = Grab_SQL_Metadata_From_Measurement( measurement_id );
		this->ui.selectedName_lineEdit->setText( data[ "sample_name" ] );
		this->ui.selectedTemperature_lineEdit->setText( data[ "temperature_in_k" ] );

		//{
		//	QSqlQuery query( this->sql_db );
		//	query.prepare( "SELECT wavenumber,intensity FROM raw_ftir_data WHERE measurement_id = \"" + measurement_id + '"' );
		//	if( !query.exec() )
		//	{
		//		qDebug() << "Error pulling data from raw_ftir_data: "
		//			<< query.lastError();
		//		return;
		//	}

		//	QVector<double> x_data, y_data;
		//	while( query.next() )
		//	{
		//		x_data.push_back( query.value( 0 ).toDouble() );
		//		y_data.push_back( query.value( 1 ).toDouble() );
		//	}

		//	for( int i = 0; i < y_data.size(); i++ )
		//		y_data[ i ] = y_display_method( x_data[ i ], y_data[ i ] ); // Do the y stuff first to not screw up x data for y_display_method
		//	for( double & x : x_data )
		//		x = x_display_method( x );

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
	if( selected.size() == 1 && this->Get_Bottom_Children_Elements_Under( selected[0] ).size() == 1 ) // context menu on legend requested
	{
		menu->addAction( "Set As Background", [this, selected]
		{
			QString measurement_for_background = selected[ 0 ]->text( selected[ 0 ]->columnCount() - 1 );
			QVector<double> x_data, y_data;
			Grab_SQL_Data_From_Measurement_ID( measurement_for_background, x_data, y_data );
			ui.customPlot->y_display_method = [x_data, y_data]( double x, double y )
			{
				//return Divide_Data_Sets( x, y, x_data, y_data );
				int i = std::distance( x_data.begin(), std::lower_bound( x_data.begin(), x_data.end(), x ) );
				i = std::min( y_data.size() - 1, i );

				return (y / y_data[ i ]);
			};
			this->Regraph_All_Plots();
		} );
	}

	menu->addAction( "Clear Background", [this]
	{
		ui.customPlot->y_display_method = []( double x, double y ) { return y; };
		this->Regraph_All_Plots();
	} );

	menu->addAction( "Graph Selected", [this, selected]
	{
		for( const auto & tree_item : selected )
		{
			vector<const QTreeWidgetItem*> things_to_graph = this->Get_Bottom_Children_Elements_Under( tree_item );

			for( const QTreeWidgetItem* x : things_to_graph )
			{
				Graph_Tree_Node( x );
			}
		}
	} );

	menu->addAction( "Save to csv file", [this, selected]
	{
		vector<const QTreeWidgetItem*> all_things_to_save;
		for( const auto & tree_item : selected )
		{
			vector<const QTreeWidgetItem*> things_to_save = this->Get_Bottom_Children_Elements_Under( tree_item );

			all_things_to_save.insert( all_things_to_save.end(), things_to_save.begin(), things_to_save.end() );
		}

		this->Save_To_CSV( all_things_to_save );
	} );

	menu->popup( ui.treeWidget->mapToGlobal( pos ) );
}

void FTIR_Analyzer::Save_To_CSV( const std::vector<const QTreeWidgetItem*> & things_to_save )
{
	QFileInfo file_name = QFileDialog::getSaveFileName( this,
														tr( "Save Data" ), QString(), tr( "CSV File (*.csv)" ) );//;; JPG File (*.jpg);; BMP File (*.bmp);; PDF File (*.pdf)" ) );

	if( file_name.suffix().toLower() == "csv" )
	{
		std::ofstream out_file( file_name.absoluteFilePath().toStdString() );
		int measurment_id_column = ui.treeWidget->columnCount() - 1;

		QVector<double> repeating_x_data;
		for( const QTreeWidgetItem* tree_item : things_to_save )
		{
			QString measurement_to_graph = tree_item->text( measurment_id_column );

			std::map<QString, QString> meta_data = Grab_SQL_Metadata_From_Measurement( measurement_to_graph );
			QString info = meta_data[ "sample_name" ] + " " + meta_data[ "temperature_in_k" ] + "K";


			QVector<double> x_data, y_data;
			Grab_SQL_Data_From_Measurement_ID( measurement_to_graph, x_data, y_data );

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
				out_file << "Wavenumber";
				for( double x : x_data )
					out_file << "," << x;
				out_file << "\n";
			}

			out_file << info.toStdString();
			for( int i = 0; i < y_data.size(); i++ )
				out_file << "," << ui.customPlot->y_display_method( x_data[ i ], y_data[ i ] );
			out_file << "\n";
		}
	}
}

bool FTIR_Analyzer::Initialize_DB_Connection( const QString & database_type, const QString & host_location, const QString & database_name, const QString & username, const QString & password )
{
	sql_db = QSqlDatabase::addDatabase( database_type );
	if( database_type == "QMYSQL" )
		sql_db.setConnectOptions( "MYSQL_OPT_RECONNECT=1" );

	if( !host_location.isEmpty() )
		sql_db.setHostName( host_location );
	sql_db.setDatabaseName( database_name );
	if( !username.isEmpty() )
		sql_db.setUserName( username );
	if( !password.isEmpty() )
		sql_db.setPassword( password );

	QMessageBox msgBox( this );
	msgBox.setWindowTitle( "FTIR Analyzer" );
	msgBox.setText( "Attempting SQL connection, please wait..." );
	msgBox.setWindowModality( Qt::NonModal ); // Don't block
	msgBox.show();

	bool sql_worked = sql_db.open();
	if( sql_worked )
		qInfo() << QString( "Connected to %1 Database %2" ).arg( host_location ).arg( database_name );
	else
	{
		auto problem = sql_db.lastError();
		qCritical() << "Error with SQL Connection: " << problem;
	}
	msgBox.close();

	return sql_worked;
}

void FTIR_Analyzer::Initialize_SQL()
{
	QString config_filename = "configuration.ini";
	QSettings settings( config_filename, QSettings::IniFormat, this );

	if( !Initialize_DB_Connection( settings.value( "SQL_Server/database_type" ).toString(),
									  settings.value( "SQL_Server/host_location" ).toString(),
									  settings.value( "SQL_Server/database_name" ).toString(),
									  settings.value( "SQL_Server/username" ).toString(),
									  settings.value( "SQL_Server/password" ).toString() ) )
	{
		QMessageBox msgBox;
		msgBox.setText( "Error Opening SQL" );
		msgBox.setInformativeText( "Do you want to open your config file?\n(Restart program to try SQL again)" );
		msgBox.setStandardButtons( QMessageBox::Yes | QMessageBox::No );
		msgBox.setDefaultButton( QMessageBox::Save );
		int ret = msgBox.exec();
		if( ret == QMessageBox::Yes )
		{
			QDesktopServices::openUrl( QUrl( config_filename ) ); // Open config file in default program
		}
	}

	ui.sqlUser_lineEdit->setText( settings.value( "SQL_Server/default_user" ).toString() );

	//this->sql_insert_command.prepare( "INSERT INTO " + Sanitize_SQL( identifier_to_table[ this->listener ] )
	//								  + " (location," + headers.join( ',' ) + ") "
	//								  "VALUES (:location," + value_binds.join( ',' ) + ")" );

	//this->sql_insert_command.bindValue( ":location", this->name );

}

void FTIR_Analyzer::Recursive_Tree_Table_Build( const QStringList & what_to_collect, QTreeWidgetItem* parent_tree, int current_collectable_i, QStringList filters )
{
	if( current_collectable_i == what_to_collect.size() )
		return;

	QSqlQuery query( sql_db );
	QString querey_string = QString( "SELECT DISTINCT %1 FROM ftir_measurements" ).arg( what_to_collect[ current_collectable_i ] );
	if( !filters.empty() )
	{
		QString requirements = filters.join( " AND " );
		querey_string += QString( " WHERE %1" ).arg( requirements );
	}
	query.prepare( querey_string );

	if( !query.exec() )
	{
		qDebug() << "Error pulling data from ftir_measurments: "
			<< query.lastError();
		return;
	}

	int numberOfRows = 0;
	if( query.last() )
	{
		numberOfRows = query.at() + 1;
		query.first();
		query.previous();
	}

	while( query.next() )
	{
		QString current_value = query.value( 0 ).toString();
		QStringList new_filters( filters );
		if( query.value( 0 ).isNull() )
			new_filters.append( QString( "%1 IS NULL" ).arg( what_to_collect[ current_collectable_i ] ) );
		else
			new_filters.append( QString( "%1 = \"%2\"" ).arg( what_to_collect[ current_collectable_i ], current_value ) );
		QTreeWidgetItem* new_tree_branch = parent_tree; // Only add a new breakout for the first one, and if more than 1 child
		if( numberOfRows > 1 || current_collectable_i == 0 )
		{
			new_tree_branch = new QTreeWidgetItem( parent_tree );
			if( !(current_value.size() > 1) )
				int i = 0;
		}
		new_tree_branch->setText( current_collectable_i, current_value );
		Recursive_Tree_Table_Build( what_to_collect, new_tree_branch, current_collectable_i + 1, new_filters );
	}

}

void FTIR_Analyzer::Reinitialize_Tree_Table()
{
	if( !sql_db.isOpen() )
		return;

	ui.treeWidget->clear();
	ui.treeWidget->setSizeAdjustPolicy( QAbstractScrollArea::AdjustToContents );

	QString user = ui.sqlUser_lineEdit->text();

	QSqlQuery query( sql_db );
	QStringList header_titles{ "Sample Name", "Date", "Temperature (K)", "Bias (V)", "Time of Day", "measurement_id" };
	ui.treeWidget->setHeaderLabels( header_titles );
	ui.treeWidget->hideColumn( header_titles.size() - 1 );
	QStringList what_to_collect{ "sample_name", "date(time)", "temperature_in_k", "bias_in_v", "time(time)", "measurement_id" }; // DATE_FORMAT(time, '%b %e %Y') DATE_FORMAT(time, '%H:%i:%s')
																																 //QStringList sql_queries{ "SELECT DISTINCT sample_name FROM ftir_measurements",
																																 //						 "SELECT DISTINCT date(time) FROM ftir_measurements WHERE sample_name is " };

	QString first_filter = "sample_name LIKE \"%" + ui.filter_lineEdit->text() + "%\"";
	QString user_filter = "user = \"" + user + "\"";
	//QTreeWidgetItem* tree_item = new QTreeWidgetItem( ui.treeWidget );
	Recursive_Tree_Table_Build( what_to_collect, ui.treeWidget->invisibleRootItem(), 0, { first_filter, user_filter } );
	for( int i = 0; i < header_titles.size(); i++ )
		ui.treeWidget->resizeColumnToContents( i );
}

void FTIR_Analyzer::Initialize_Tree_Table()
{
	Reinitialize_Tree_Table();

	connect( ui.treeWidget, &QTreeWidget::itemDoubleClicked, [this]( QTreeWidgetItem* tree_item, int column )
	{
		vector<const QTreeWidgetItem*> things_to_graph = this->Get_Bottom_Children_Elements_Under( tree_item );

		for( const QTreeWidgetItem* x : things_to_graph )
		{
			Graph_Tree_Node( x );
		}
	} );

	// setup policy and connect slot for context menu popup:
	ui.treeWidget->setContextMenuPolicy( Qt::CustomContextMenu );
	connect( ui.treeWidget, &QWidget::customContextMenuRequested, this, &FTIR_Analyzer::treeContextMenuRequest );

	//ui.treeWidget->addAction( new QAction( tr( "Set As Background" ), [this]()// QTreeWidgetItem* tree_item, int column )
	//{
	//	//vector<const QTreeWidgetItem*> things_to_graph = this->Get_All_Children_Full_Tree_Elements_Under( tree_item );

	//	//for( const QTreeWidgetItem* x : things_to_graph )
	//	//{
	//	//	Graph_Tree_Node( x );
	//	//}
	//} ) );
}

void FTIR_Analyzer::Graph_Tree_Node( const QTreeWidgetItem* tree_item )
{
	int column = ui.treeWidget->columnCount() - 1;
	QString measurement_to_graph = tree_item->text( column );

	//QStringList graph_label;
	//for( int i = 0; i < ui.sampleList_tableWidget->columnCount() - 1; i++ )
	//	graph_label.push_back( ui.sampleList_tableWidget->item( row, i )->text() );

	QVector<double> x_data, y_data;
	Grab_SQL_Data_From_Measurement_ID( measurement_to_graph, x_data, y_data );
	this->Graph( measurement_to_graph, x_data, y_data );
}

vector<const QTreeWidgetItem*> FTIR_Analyzer::Get_Bottom_Children_Elements_Under( const QTreeWidgetItem* tree_item ) const
{
	int number_of_children = tree_item->childCount();
	if( number_of_children == 0 )
		return { tree_item };

	vector<const QTreeWidgetItem*> lowest_level_children;

	for( int i = 0; i < number_of_children; i++ )
	{
		vector<const QTreeWidgetItem*> i_lowest_level_children = Get_Bottom_Children_Elements_Under( tree_item->child( i ) );
		lowest_level_children.insert( lowest_level_children.end(), i_lowest_level_children.begin(), i_lowest_level_children.end() );
	}

	return lowest_level_children;
}

void FTIR_Analyzer::Grab_SQL_Data_From_Measurement_ID( QString measurement_id, QVector<double> & x_data, QVector<double> & y_data )
{
	// Grab SQL data
	QSqlQuery query( sql_db );
	query.prepare( "SELECT wavenumber,intensity FROM raw_ftir_data WHERE measurement_id = \"" + measurement_id + '"' );
	if( !query.exec() )
	{
		qDebug() << "Error pulling data from ftir_measurments: "
			<< query.lastError();
		return;
	}

	while( query.next() )
	{
		x_data.push_back( query.value( 0 ).toDouble() );
		y_data.push_back( query.value( 1 ).toDouble() );
	}
}


void FTIR_Analyzer::Graph( QString measurement_id, const QVector<double> & x_data, const QVector<double> & y_data, QString data_title, bool allow_y_scaling )
{
	QCPGraph* current_graph = ui.customPlot->Graph( x_data, y_data, measurement_id, data_title, allow_y_scaling );
}

void FTIR_Analyzer::Regraph_All_Plots()
{
	ui.customPlot->RegraphAll();
	ui.customPlot->rescaleAxes();
	ui.customPlot->yAxis->setRangeLower( 0 );
	double upper = ui.customPlot->yAxis->range().upper;
	ui.customPlot->yAxis->setRangeUpper( std::min( upper, 1. ) );
	ui.customPlot->replot();
}