#include "FTIR_Analyzer.h"

#include <QSettings>
#include <QFileDialog>
#include <vector>

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
	Initialize_Graph();
	//ui.customPlot->rescaleAxes();

	//ui.treeWidget->setItemDelegate( new GridDelegate( ui.treeWidget ) );
	Initialize_Tree_Table();
	Initialize_Table();

	//setGeometry( QApplication::desktop()->availableGeometry().x(),
	//			 QApplication::desktop()->availableGeometry().y(),
	//			 QApplication::desktop()->availableGeometry().width(),
	//			 QApplication::desktop()->availableGeometry().height() );
}

void FTIR_Analyzer::Initialize_Graph()
{
	ui.customPlot->setOpenGl( true );
	ui.customPlot->setInteractions( QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectAxes |
									QCP::iSelectLegend | QCP::iSelectPlottables );
	ui.customPlot->xAxis->setRange( -8, 8 );
	ui.customPlot->yAxis->setRange( -5, 5 );
	ui.customPlot->axisRect()->setupFullAxesBox();

	ui.customPlot->plotLayout()->insertRow( 0 );
	QCPTextElement *title = new QCPTextElement( ui.customPlot, "Transmission", QFont( "sans", 17, QFont::Bold ) );
	ui.customPlot->plotLayout()->addElement( 0, 0, title );

	ui.customPlot->xAxis->setLabel( "Wavelength (" + QString( QChar( 0x03BC ) ) + "m)" );
	ui.customPlot->yAxis->setLabel( "Intensity" );
	ui.customPlot->legend->setVisible( true );
	QFont legendFont = font();
	legendFont.setPointSize( 10 );
	ui.customPlot->legend->setFont( legendFont );
	ui.customPlot->legend->setSelectedFont( legendFont );
	ui.customPlot->legend->setSelectableParts( QCPLegend::spItems ); // legend box shall not be selectable, only legend items

																	 // connect slot that ties some axis selections together (especially opposite axes):
	connect( ui.customPlot, &QCustomPlot::selectionChangedByUser, this, &FTIR_Analyzer::selectionChanged );
	// connect slots that takes care that when an axis is selected, only that direction can be dragged and zoomed:
	connect( ui.customPlot, &QCustomPlot::mousePress, this, &FTIR_Analyzer::mousePress );
	connect( ui.customPlot, &QCustomPlot::mouseWheel, this, &FTIR_Analyzer::mouseWheel );

	connect( ui.customPlot, &QCustomPlot::mouseDoubleClick, this, &FTIR_Analyzer::refitGraphs );

	// make bottom and left axes transfer their ranges to top and right axes:
	connect( ui.customPlot->xAxis, SIGNAL( rangeChanged( QCPRange ) ), ui.customPlot->xAxis2, SLOT( setRange( QCPRange ) ) );
	connect( ui.customPlot->yAxis, SIGNAL( rangeChanged( QCPRange ) ), ui.customPlot->yAxis2, SLOT( setRange( QCPRange ) ) );

	// connect some interaction slots:
	connect( ui.customPlot, &QCustomPlot::axisDoubleClick, this, &FTIR_Analyzer::axisLabelDoubleClick );
	connect( ui.customPlot, &QCustomPlot::legendDoubleClick, this, &FTIR_Analyzer::legendDoubleClick );
	connect( title, &QCPTextElement::doubleClicked, this, &FTIR_Analyzer::titleDoubleClick );

	// connect slot that shows a message in the status bar when a graph is clicked:
	connect( ui.customPlot, SIGNAL( plottableClick( QCPAbstractPlottable*, int, QMouseEvent* ) ), this, SLOT( graphClicked( QCPAbstractPlottable*, int ) ) );

	// setup policy and connect slot for context menu popup:
	ui.customPlot->setContextMenuPolicy( Qt::CustomContextMenu );
	connect( ui.customPlot, &QWidget::customContextMenuRequested, this, &FTIR_Analyzer::graphContextMenuRequest );
}

void FTIR_Analyzer::titleDoubleClick( QMouseEvent* event )
{
	Q_UNUSED( event )
		if( QCPTextElement *title = qobject_cast<QCPTextElement*>(sender()) )
		{
			// Set the plot title by double clicking on it
			bool ok;
			QString newTitle = QInputDialog::getText( this, "QCustomPlot example", "New plot title:", QLineEdit::Normal, title->text(), &ok );
			if( ok )
			{
				title->setText( newTitle );
				ui.customPlot->replot();
			}
		}
}

void FTIR_Analyzer::axisLabelDoubleClick( QCPAxis *axis, QCPAxis::SelectablePart part )
{
	// Set an axis label by double clicking on it
	if( part == QCPAxis::spAxisLabel ) // only react when the actual axis label is clicked, not tick label or axis backbone
	{
		bool ok;
		QString newLabel = QInputDialog::getText( this, "QCustomPlot example", "New axis label:", QLineEdit::Normal, axis->label(), &ok );
		if( ok )
		{
			axis->setLabel( newLabel );
			ui.customPlot->replot();
		}
	}
}

void FTIR_Analyzer::legendDoubleClick( QCPLegend *legend, QCPAbstractLegendItem *item )
{
	// Rename a graph by double clicking on its legend item
	Q_UNUSED( legend )
		if( item ) // only react if item was clicked (user could have clicked on border padding of legend where there is no item, then item is 0)
		{
			QCPPlottableLegendItem *plItem = qobject_cast<QCPPlottableLegendItem*>(item);
			bool ok;
			QString newName = QInputDialog::getText( this, "QCustomPlot example", "New graph name:", QLineEdit::Normal, plItem->plottable()->name(), &ok );
			if( ok )
			{
				plItem->plottable()->setName( newName );
				ui.customPlot->replot();
			}
		}
}

void FTIR_Analyzer::selectionChanged()
{
	/*
	normally, axis base line, axis tick labels and axis labels are selectable separately, but we want
	the user only to be able to select the axis as a whole, so we tie the selected states of the tick labels
	and the axis base line together. However, the axis label shall be selectable individually.

	The selection state of the left and right axes shall be synchronized as well as the state of the
	bottom and top axes.

	Further, we want to synchronize the selection of the graphs with the selection state of the respective
	legend item belonging to that graph. So the user can select a graph by either clicking on the graph itself
	or on its legend item.
	*/

	// make top and bottom axes be selected synchronously, and handle axis and tick labels as one selectable object:
	if( ui.customPlot->xAxis->selectedParts().testFlag( QCPAxis::spAxis ) || ui.customPlot->xAxis->selectedParts().testFlag( QCPAxis::spTickLabels ) ||
		ui.customPlot->xAxis2->selectedParts().testFlag( QCPAxis::spAxis ) || ui.customPlot->xAxis2->selectedParts().testFlag( QCPAxis::spTickLabels ) )
	{
		ui.customPlot->xAxis2->setSelectedParts( QCPAxis::spAxis | QCPAxis::spTickLabels );
		ui.customPlot->xAxis->setSelectedParts( QCPAxis::spAxis | QCPAxis::spTickLabels );
	}
	// make left and right axes be selected synchronously, and handle axis and tick labels as one selectable object:
	if( ui.customPlot->yAxis->selectedParts().testFlag( QCPAxis::spAxis ) || ui.customPlot->yAxis->selectedParts().testFlag( QCPAxis::spTickLabels ) ||
		ui.customPlot->yAxis2->selectedParts().testFlag( QCPAxis::spAxis ) || ui.customPlot->yAxis2->selectedParts().testFlag( QCPAxis::spTickLabels ) )
	{
		ui.customPlot->yAxis2->setSelectedParts( QCPAxis::spAxis | QCPAxis::spTickLabels );
		ui.customPlot->yAxis->setSelectedParts( QCPAxis::spAxis | QCPAxis::spTickLabels );
	}

	// synchronize selection of graphs with selection of corresponding legend items:
	for( int i = 0; i < ui.customPlot->graphCount(); ++i )
	{
		QCPGraph *graph = ui.customPlot->graph( i );
		QCPPlottableLegendItem *item = ui.customPlot->legend->itemWithPlottable( graph );
		if( item->selected() || graph->selected() )
		{
			item->setSelected( true );
			graph->setSelection( QCPDataSelection( graph->data()->dataRange() ) );
		}
	}
}

void FTIR_Analyzer::mousePress()
{
	// if an axis is selected, only allow the direction of that axis to be dragged
	// if no axis is selected, both directions may be dragged

	if( ui.customPlot->xAxis->selectedParts().testFlag( QCPAxis::spAxis ) )
		ui.customPlot->axisRect()->setRangeDrag( ui.customPlot->xAxis->orientation() );
	else if( ui.customPlot->yAxis->selectedParts().testFlag( QCPAxis::spAxis ) )
		ui.customPlot->axisRect()->setRangeDrag( ui.customPlot->yAxis->orientation() );
	else
		ui.customPlot->axisRect()->setRangeDrag( Qt::Horizontal | Qt::Vertical );
}

void FTIR_Analyzer::mouseWheel()
{
	// if an axis is selected, only allow the direction of that axis to be zoomed
	// if no axis is selected, both directions may be zoomed

	if( ui.customPlot->xAxis->selectedParts().testFlag( QCPAxis::spAxis ) )
		ui.customPlot->axisRect()->setRangeZoom( ui.customPlot->xAxis->orientation() );
	else if( ui.customPlot->yAxis->selectedParts().testFlag( QCPAxis::spAxis ) )
		ui.customPlot->axisRect()->setRangeZoom( ui.customPlot->yAxis->orientation() );
	else
		ui.customPlot->axisRect()->setRangeZoom( Qt::Horizontal | Qt::Vertical );
}

void FTIR_Analyzer::refitGraphs()
{
	ui.customPlot->rescaleAxes();
	ui.customPlot->yAxis->setRangeLower( 0 );
	//ui.customPlot->yAxis->setRange( -10, 110 );
	ui.customPlot->replot();
}

void FTIR_Analyzer::removeSelectedGraph()
{
	if( ui.customPlot->selectedGraphs().size() > 0 )
	{
		QCPGraph* selected_graph = ui.customPlot->selectedGraphs().first();
		active_graphs.erase( selected_graph );
		ui.customPlot->removeGraph( selected_graph );
		ui.customPlot->replot();
	}
}

void FTIR_Analyzer::removeAllGraphs()
{
	active_graphs.clear();
	ui.customPlot->clearGraphs();
	ui.customPlot->replot();
}

void FTIR_Analyzer::graphContextMenuRequest( QPoint pos )
{
	QMenu *menu = new QMenu( this );
	menu->setAttribute( Qt::WA_DeleteOnClose );

	if( ui.customPlot->legend->selectTest( pos, false ) >= 0 ) // context menu on legend requested
	{
		menu->addAction( "Move to top left", this, &FTIR_Analyzer::moveLegend )->setData( (int)(Qt::AlignTop | Qt::AlignLeft) );
		menu->addAction( "Move to top center", this, &FTIR_Analyzer::moveLegend )->setData( (int)(Qt::AlignTop | Qt::AlignHCenter) );
		menu->addAction( "Move to top right", this, &FTIR_Analyzer::moveLegend )->setData( (int)(Qt::AlignTop | Qt::AlignRight) );
		menu->addAction( "Move to bottom right", this, &FTIR_Analyzer::moveLegend )->setData( (int)(Qt::AlignBottom | Qt::AlignRight) );
		menu->addAction( "Move to bottom left", this, &FTIR_Analyzer::moveLegend )->setData( (int)(Qt::AlignBottom | Qt::AlignLeft) );
	}
	else  // general context menu on graphs requested
	{
		//menu->addAction( "Add random graph", this, SLOT( addRandomGraph() ) );
		if( ui.customPlot->selectedGraphs().size() > 0 )
			menu->addAction( "Remove selected graph", this, &FTIR_Analyzer::removeSelectedGraph );
		if( ui.customPlot->graphCount() > 0 )
			menu->addAction( "Remove all graphs", this, &FTIR_Analyzer::removeAllGraphs );
		menu->addAction( "Save current graph", this, &FTIR_Analyzer::saveCurrentGraph );
	}

	menu->popup( ui.customPlot->mapToGlobal( pos ) );
}

void FTIR_Analyzer::treeContextMenuRequest( QPoint pos )
{
	QMenu *menu = new QMenu( this );
	menu->setAttribute( Qt::WA_DeleteOnClose );

	if( ui.treeWidget->itemAt( pos ) != nullptr ) // context menu on legend requested
	{
		menu->addAction( "Set As Background", [this]
		{
			x_display_method = []( double x ) { return x; };
			this->Regraph_All_Plots();
		} );
		//menu->addAction( "Move to top center", this, &FTIR_Analyzer::moveLegend )->setData( (int)(Qt::AlignTop | Qt::AlignHCenter) );
	}

	menu->popup( ui.treeWidget->mapToGlobal( pos ) );
}

void FTIR_Analyzer::moveLegend()
{
	if( QAction* contextAction = qobject_cast<QAction*>(sender()) ) // make sure this slot is really called by a context menu action, so it carries the data we need
	{
		bool ok;
		int dataInt = contextAction->data().toInt( &ok );
		if( ok )
		{
			ui.customPlot->axisRect()->insetLayout()->setInsetAlignment( 0, (Qt::Alignment)dataInt );
			ui.customPlot->replot();
		}
	}
}

void FTIR_Analyzer::graphClicked( QCPAbstractPlottable *plottable, int dataIndex )
{
	// since we know we only have QCPGraphs in the plot, we can immediately access interface1D()
	// usually it's better to first check whether interface1D() returns non-zero, and only then use it.
	double dataValue = plottable->interface1D()->dataMainValue( dataIndex );
	QString message = QString( "Clicked on graph '%1' at data point #%2 with value %3." ).arg( plottable->name() ).arg( dataIndex ).arg( dataValue );
	ui.statusBar->showMessage( message, 2500 );
}

bool FTIR_Analyzer::Initialize_DB_Connection( const QString & database_type, const QString & host_location, const QString & database_name, const QString & username, const QString & password )
{
	sql_db = QSqlDatabase::addDatabase( database_type );
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
			new_filters.append( QString( "%1 IS \"%2\"" ).arg( what_to_collect[ current_collectable_i ], current_value ) );
		QTreeWidgetItem* new_tree_branch = parent_tree; // Only add a new breakout for the first one, and if more than 1 child
		if( numberOfRows > 1 || current_collectable_i == 0 )
			new_tree_branch = new QTreeWidgetItem( parent_tree );
		new_tree_branch->setText( current_collectable_i, current_value );
		Recursive_Tree_Table_Build( what_to_collect, new_tree_branch, current_collectable_i + 1, new_filters );
	}

}

void FTIR_Analyzer::Initialize_Tree_Table()
{
	ui.treeWidget->clear();
	ui.treeWidget->setSizeAdjustPolicy( QAbstractScrollArea::AdjustToContents );

	QSqlQuery query( sql_db );
	QStringList header_titles{ "Sample Name", "Date", "Temperature (K)", "Bias (V)", "Time of Day", "measurement_id" };
	ui.treeWidget->setHeaderLabels( header_titles );
	ui.treeWidget->hideColumn( header_titles.size() - 1 );
	QStringList what_to_collect{ "sample_name", "date(time)", "temperature_in_k", "bias_in_v", "time(time)", "measurement_id" }; // DATE_FORMAT(time, '%b %e %Y') DATE_FORMAT(time, '%H:%i:%s')
																																 //QStringList sql_queries{ "SELECT DISTINCT sample_name FROM ftir_measurements",
																																 //						 "SELECT DISTINCT date(time) FROM ftir_measurements WHERE sample_name is " };

	QTreeWidgetItem* tree_item = new QTreeWidgetItem( ui.treeWidget );
	Recursive_Tree_Table_Build( what_to_collect, ui.treeWidget->invisibleRootItem(), 0, QStringList() );
	for( int i = 0; i < header_titles.size(); i++ )
		ui.treeWidget->resizeColumnToContents( i );

	connect( ui.treeWidget, &QTreeWidget::itemDoubleClicked, [this]( QTreeWidgetItem* tree_item, int column )
	{
		vector<const QTreeWidgetItem*> things_to_graph = this->Get_All_Children_Full_Tree_Elements_Under( tree_item );

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
	Graph( { "", measurement_to_graph }, x_data, y_data );
}

vector<const QTreeWidgetItem*> FTIR_Analyzer::Get_All_Children_Full_Tree_Elements_Under( const QTreeWidgetItem* tree_item ) const
{
	vector<const QTreeWidgetItem*> lowest_children = Recursive_Get_All_Children_Full_Tree_Elements_Under( tree_item );
	return lowest_children;

	vector< QStringList > reversed_output;
	QTreeWidgetItem* root = nullptr;
	//QTreeWidgetItem* root = ui.treeWidget->invisibleRootItem();
	if( lowest_children.empty() )
		return {};

	int column_count = 0;
	{
		const QTreeWidgetItem* graph = lowest_children[ 0 ];
		for( const QTreeWidgetItem* march_to_root = graph; march_to_root != root; march_to_root = march_to_root->parent() )
			column_count++;
	}

	//for( QTreeWidgetItem* graph : lowest_children )
	//{
	//	reversed_output.push_back( {} );
	//	QStringList & reverse_details = reversed_output.back();
	//	reverse_details.re;
	//	QStringList::fromStdList
	//	int i = column_count - 1;
	//	for( QTreeWidgetItem* march_to_root = graph; march_to_root != root; march_to_root = march_to_root->parent() )
	//	{
	//		reverse_details[ i ] = march_to_root->text( i );
	//		i--;
	//	}
	//}
}

vector<const QTreeWidgetItem*> FTIR_Analyzer::Recursive_Get_All_Children_Full_Tree_Elements_Under( const QTreeWidgetItem* tree_item ) const
{
	int number_of_children = tree_item->childCount();
	if( number_of_children == 0 )
		return { tree_item };

	vector<const QTreeWidgetItem*> lowest_level_children;

	for( int i = 0; i < number_of_children; i++ )
	{
		vector<const QTreeWidgetItem*> i_lowest_level_children = Recursive_Get_All_Children_Full_Tree_Elements_Under( tree_item->child( i ) );
		lowest_level_children.insert( lowest_level_children.end(), i_lowest_level_children.begin(), i_lowest_level_children.end() );
	}

	return lowest_level_children;
}

void FTIR_Analyzer::Initialize_Table()
{
	QSqlQuery query( sql_db );
	QStringList what_to_collect{ "sample_name", "time", "temperature_in_k", "bias_in_v", "measurement_id" };
	query.prepare( "SELECT " + what_to_collect.join( ',' ) + " FROM ftir_measurements" );
	//query.prepare( "SELECT * FROM ftir_measurements" );
	if( !query.exec() )
	{
		qDebug() << "Error pulling data from ftir_measurments: "
			<< query.lastError();
		return;
	}

	ui.sampleList_tableWidget->setColumnCount( what_to_collect.size() ); // Clear any previous data

	ui.sampleList_tableWidget->setRowCount( 0 ); // Clear any previous data
	while( query.next() )
	{
		int num_rows = ui.sampleList_tableWidget->rowCount();
		ui.sampleList_tableWidget->insertRow( num_rows ); // add another row on bottom
		for( int row = 0; row < what_to_collect.size(); row++ )
			ui.sampleList_tableWidget->setItem( num_rows, row, new QTableWidgetItem( query.value( row ).toString() ) );
	}

	int idName = query.record().indexOf( "measurement_id" );
	ui.sampleList_tableWidget->setColumnHidden( idName, true );

	//ui.sampleList_tableWidget->setSelectionMode( QAbstractItemView::SelectionMode::ContiguousSelection );
	connect( ui.sampleList_tableWidget, &QTableWidget::cellActivated, [this]( int row, int column ) { ui.sampleList_tableWidget->selectRow( row ); } );
	connect( ui.sampleList_tableWidget, &QTableWidget::cellActivated, [this]( int row, int column ) { this->Graph_Row( row ); } );
}

void FTIR_Analyzer::Grab_SQL_Data_From_Measurement_ID( QString measurement_id, QVector<double> & x_data, QVector<double> & y_data )
{
	// Grab SQL data
	QSqlQuery query( sql_db );
	query.prepare( "SELECT wavenumber,intensity FROM raw_ftir_data WHERE measurement_id IS \"" + measurement_id + '"' );
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

void FTIR_Analyzer::Graph_Row( int row )
{
	int column = ui.sampleList_tableWidget->columnCount() - 1;
	const auto* test = ui.sampleList_tableWidget->item( row, column );
	QString measurement_to_graph = test->text();

	QStringList graph_label;
	for( int i = 0; i < ui.sampleList_tableWidget->columnCount() - 1; i++ )
		graph_label.push_back( ui.sampleList_tableWidget->item( row, i )->text() );

	QVector<double> x_data, y_data;
	Grab_SQL_Data_From_Measurement_ID( measurement_to_graph, x_data, y_data );
	Graph( { graph_label.join( ' ' ), measurement_to_graph }, x_data, y_data );
}

void FTIR_Analyzer::Graph( GraphInfo meta_data, QVector<double> x_data, QVector<double> y_data )
{
	for( double & x : x_data )
		x = x_display_method( x );
	for( double & y : y_data )
		y = y_display_method( y );

	// Add graph
	{
		static int color_index = 0;
		QCPGraph* current_graph = ui.customPlot->addGraph();
		active_graphs[ current_graph ] = meta_data;
		ui.customPlot->graph()->setName( meta_data.graph_title );
		ui.customPlot->graph()->setData( x_data, y_data );
		ui.customPlot->graph()->setLineStyle( QCPGraph::lsLine );// (QCPGraph::LineStyle)(rand() % 5 + 1) );
		if( rand() % 100 > 50 )
			ui.customPlot->graph()->setScatterStyle( QCPScatterStyle( (QCPScatterStyle::ScatterShape)(rand() % 14 + 1) ) );
		QPen graphPen;
		QCPColorGradient gradient( QCPColorGradient::gpSpectrum );// gpPolar );
		gradient.setPeriodic( true );
		//gradient.setLevelCount( 10 );
		graphPen.setColor( gradient.color( (color_index++) / 10.0, QCPRange( 0.0, 1.0 ) ) );
		//graphPen.setColor( QColor::fromHslF( (color_index++)/10.0*0.8, 0.95, 0.5) );
		//graphPen.setColor( QColor::fromHsv( rand() % 255, 255, 255 ) );
		//graphPen.setWidthF( rand() / (double)RAND_MAX * 2 + 1 );
		ui.customPlot->graph()->setPen( graphPen );
		ui.customPlot->rescaleAxes();
		ui.customPlot->yAxis->setRangeLower( 0 );
		//ui.customPlot->yAxis->setRange( -10, 110 );
		ui.customPlot->replot();
	}
}

void FTIR_Analyzer::saveCurrentGraph()
{
	QFileInfo file_name = QFileDialog::getSaveFileName( this,
														tr( "Save Graph" ), QString(), tr( "PNG File (*.png);; JPG File (*.jpg);; BMP File (*.bmp);; PDF File (*.pdf)" ) );

	if( file_name.suffix().toLower() == "png" )
		ui.customPlot->savePng( file_name.absoluteFilePath() );
	else if( file_name.suffix().toLower() == "jpg" )
		ui.customPlot->saveJpg( file_name.absoluteFilePath() );
	else if( file_name.suffix().toLower() == "bmp" )
		ui.customPlot->saveBmp( file_name.absoluteFilePath() );
	else if( file_name.suffix().toLower() == "pdf" )
		ui.customPlot->savePdf( file_name.absoluteFilePath() );
}

void FTIR_Analyzer::Regraph_All_Plots()
{
	for( const auto & graph_and_id : active_graphs )
	{
		QCPGraph* graph = graph_and_id.first;
		QString measurement_id = graph_and_id.second.measurement_id;
		QVector<double> x_data, y_data;
		Grab_SQL_Data_From_Measurement_ID( measurement_id, x_data, y_data );

		for( double & x : x_data )
			x = x_display_method( x );
		for( double & y : y_data )
			y = y_display_method( y );

		graph->setData( x_data, y_data );
	}
	ui.customPlot->rescaleAxes();
	ui.customPlot->yAxis->setRangeLower( 0 );
	//ui.customPlot->yAxis->setRange( -10, 110 );
	ui.customPlot->replot();
}