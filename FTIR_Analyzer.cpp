#include "FTIR_Analyzer.h"

#include <QSettings>
#include <QFileDialog>
#include <vector>
#include <math.h>

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

	Initialize_Graph();


	//setGeometry( QApplication::desktop()->availableGeometry().x(),
	//			 QApplication::desktop()->availableGeometry().y(),
	//			 QApplication::desktop()->availableGeometry().width(),
	//			 QApplication::desktop()->availableGeometry().height() );
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

void FTIR_Analyzer::Initialize_Graph()
{
	connect( ui.customPlot, &QWidget::customContextMenuRequested, ui.customPlot, &Interactive_Graph::graphContextMenuRequest );
	connect( ui.customPlot, &Interactive_Graph::Graph_Selected,[this]( QCPGraph* selected_graph )
	{
		QString measurement_id = selected_graph->name();
		// Grab SQL data
		{
			QSqlQuery query( this->sql_db );
			QStringList what_to_collect{ "sample_name", "time", "temperature_in_k", "bias_in_v" };
			query.prepare( QString( "SELECT %1 FROM ftir_measurements WHERE measurement_id = \"%2\"" ).arg( what_to_collect.join( ',' ), measurement_id ) );
			if( !query.exec() )
			{
				qDebug() << "Error pulling data from ftir_measurments: "
					<< query.lastError();
				return;
			}

			map<QString, QString> data;
			if( query.next() )
			{
				int i = 0;
				for( QString header : what_to_collect )
					data[ header ] = query.value( i++ ).toString();
			}

			this->ui.selectedName_lineEdit->setText( data[ "sample_name" ] );
			this->ui.selectedTemperature_lineEdit->setText( data[ "temperature_in_k" ] );
		}

		{
			QSqlQuery query( this->sql_db );
			query.prepare( "SELECT wavenumber,intensity FROM raw_ftir_data WHERE measurement_id = \"" + measurement_id + '"' );
			if( !query.exec() )
			{
				qDebug() << "Error pulling data from raw_ftir_data: "
					<< query.lastError();
				return;
			}

			QVector<double> x_data, y_data;
			while( query.next() )
			{
				x_data.push_back( query.value( 0 ).toDouble() );
				y_data.push_back( query.value( 1 ).toDouble() );
			}

			for( int i = 0; i < y_data.size(); i++ )
				y_data[ i ] = y_display_method( x_data[ i ], y_data[ i ] ); // Do the y stuff first to not screw up x data for y_display_method
			for( double & x : x_data )
				x = x_display_method( x );

			QVector<double> cutoffs = Find_Zero_Crossings( x_data, y_data, *std::max_element( y_data.constBegin(), y_data.constEnd() ) / 2 );
			if( !cutoffs.isEmpty() )
			{
				QStringList all_cutoffs;
				for( double cutoff : cutoffs )
					all_cutoffs.push_back( QString::number( cutoff ) );
				this->ui.selectedCutoff_lineEdit->setText( all_cutoffs.join( ", " ) );
			}
		}
	} );

	ui.customPlot->menu_functions.push_back( [this]( Interactive_Graph* graph, QMenu* menu, QPoint pos )
	{
		if( graph->xAxis->selectTest( pos, false ) >= 0 ) // general context menu on graphs requested
		{
			//menu->addAction( "Add random graph", this, SLOT( addRandomGraph() ) );
			menu->addAction( "Change to Wavelength", [this, graph]
			{
				this->x_display_method = []( double x ) { return 10000.0 / x; };
				graph->xAxis->setLabel( "Wavelength (" + QString( QChar( 0x03BC ) ) + "m)" );
				this->Regraph_All_Plots();
			} );
			menu->addAction( "Change to Wave Number", [this, graph]
			{
				this->x_display_method = []( double x ) { return x; };
				graph->xAxis->setLabel( "Wave Number (cm" + QString( QChar( 0x207B ) ) + QString( QChar( 0x00B9 ) ) + ")" );
				this->Regraph_All_Plots();
			} );
			menu->addAction( "Change to Energy", [this, graph]
			{
				this->x_display_method = []( double x ) { return x * 1.239842e-4; };
				graph->xAxis->setLabel( "Photon Energy (eV)" );
				this->Regraph_All_Plots();
			} );
		}
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
			this->y_display_method = [x_data, y_data]( double x, double y )
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
		this->y_display_method = []( double x, double y ) { return y; };
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

	menu->popup( ui.treeWidget->mapToGlobal( pos ) );
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
	bool sql_worked = sql_db.open();
	if( sql_worked )
		qInfo() << QString( "Connected to %1 Database %2" ).arg( host_location ).arg( database_name );
	else
	{
		auto problem = sql_db.lastError();
		qCritical() << "Error with SQL Connection: " << problem;
	}

	return sql_worked;
}

void FTIR_Analyzer::Initialize_SQL()
{
	QSettings settings( "configuration.ini", QSettings::IniFormat, this );

	while( !Initialize_DB_Connection( settings.value( "SQL_Server/database_type" ).toString(),
									  settings.value( "SQL_Server/host_location" ).toString(),
									  settings.value( "SQL_Server/database_name" ).toString(),
									  settings.value( "SQL_Server/username" ).toString(),
									  settings.value( "SQL_Server/password" ).toString() ) )
	{
		qCritical() << "Error with SQL Connection";
		int delay_seconds = 10;
		qCritical() << QString( "Trying again in %1 seconds" ).arg( delay_seconds );
		QThread::sleep( delay_seconds );
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
	this->Graph(measurement_to_graph, x_data, y_data );
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

void FTIR_Analyzer::Graph( QString measurement_id, QVector<double> x_data, QVector<double> y_data )
{
	for( int i = 0; i < y_data.size(); i++ )
		y_data[ i ] = y_display_method( x_data[ i ], y_data[ i ] ); // Do the y stuff first to not screw up x data for y_display_method
	for( double & x : x_data )
		x = x_display_method( x );

	QCPGraph* current_graph = ui.customPlot->Graph( x_data, y_data, measurement_id );
}

void FTIR_Analyzer::Regraph_All_Plots()
{
	for( int i = 0; i < ui.customPlot->graphCount(); i++ )
	{
		QCPGraph* graph = ui.customPlot->graph( i );
		QString measurement_id = graph->name();

		QVector<double> x_data, y_data;
		Grab_SQL_Data_From_Measurement_ID( measurement_id, x_data, y_data );

		for( int i = 0; i < y_data.size(); i++ )
			y_data[ i ] = y_display_method( x_data[ i ], y_data[ i ] ); // Do the y stuff first to not screw up x data for y_display_method
		for( double & x : x_data )
			x = x_display_method( x );

		if( x_data.size() != y_data.size() )
			throw "Trying to graph different x and y size arrays";

		ui.customPlot->UpdateGraph( graph, x_data, y_data );
	}

	ui.customPlot->rescaleAxes();
	ui.customPlot->yAxis->setRangeLower( 0 );
	double upper = ui.customPlot->yAxis->range().upper;
	ui.customPlot->yAxis->setRangeUpper( std::min( upper, 1. ) );
	ui.customPlot->replot();
}