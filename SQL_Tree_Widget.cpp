#include "SQL_Tree_Widget.h"

//#include <range/v3/all.hpp> // get everything

#include <algorithm>
#include "fn.hpp"

namespace fn = rangeless::fn;
using fn::operators::operator%;   // arg % f % g % h; // h(g(f(std::forward<Arg>(arg))));
using fn::operators::operator%=;  // arg %= fn;       // arg = fn(std::move(arg));

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

SQL_Tree_Widget::SQL_Tree_Widget( QWidget* parent ) : QTreeWidget( parent )
{
	//QStringList header_titles{ "Sample Name", "Date", "Temperature (K)", "Dewar Temp (C)", "Time of Day", "measurement_id" };
	//QStringList what_to_collect{ "sample_name", "date(time)", "temperature_in_k", "dewar_temp_in_c", "time(time)", "measurement_id" }; // DATE_FORMAT(time, '%b %e %Y') DATE_FORMAT(time, '%H:%i:%s')
	//																															 //QStringList sql_queries{ "SELECT DISTINCT sample_name FROM ftir_measurements",
	//																															 //						 "SELECT DISTINCT date(time) FROM ftir_measurements WHERE sample_name is " };
	//Set_Data_To_Gather( header_titles, what_to_collect );
}


SQL_Tree_Widget::~SQL_Tree_Widget()
{
}

void SQL_Tree_Widget::Set_Data_To_Gather( const QStringList & header_titles, const QStringList & what_to_collect, int columns_to_show )
{
	this->header_titles = header_titles;
	this->what_to_collect = what_to_collect;
	this->columns_to_show = columns_to_show;
}

void SQL_Tree_Widget::Refilter( QString filter_string )
{
	this->clear();
	this->setSizeAdjustPolicy( QAbstractScrollArea::AdjustToContents );
	int measurment_name_i = header_titles.indexOf( "Sample Name" );

	std::vector<QVariantList> filtered_data( this->current_meta_data.size() );
	auto last_match = std::copy_if( this->current_meta_data.begin(), this->current_meta_data.end(), filtered_data.begin(),
									[measurment_name_i, filter_string]( const QVariantList & row ) { return row[ measurment_name_i ].toString().contains( filter_string, Qt::CaseInsensitive ); } );
	filtered_data.resize( std::distance( filtered_data.begin(), last_match ) );

	Recursive_Build( filtered_data, this->invisibleRootItem(), 0 );
	for( int i = 0; i < header_titles.size(); i++ )
		this->resizeColumnToContents( i );
}

void SQL_Tree_Widget::Repoll_SQL( QSqlDatabase & sql_db, QString sql_table, QString user )
{
	if( !sql_db.isOpen() )
		return;

	QSqlQuery query( sql_db );
	this->setHeaderLabels( header_titles );
	for( int i = columns_to_show; i < header_titles.size(); i++ )
		this->hideColumn( i );

	QString query_string = QString( "SELECT %1 FROM %2 WHERE user=\"%3\"" ).arg( what_to_collect.join( "," ), sql_table, Sanitize_SQL( user ) );
	query.prepare( query_string );
	if( !query.exec() )
	{
		qDebug() << "Error pulling data from " << sql_table << ": "
			<< query.lastError();
		return;
	}

	int number_of_rows = 0;
	if( query.last() ) // Count the number of rows returned
	{
		number_of_rows = query.at() + 1;
		this->current_meta_data.clear();
		this->current_meta_data.resize( number_of_rows );
		query.first();
		query.previous();
	}

	int number_of_columns = query.record().count();
	for( int row_i = 0; row_i < number_of_rows; row_i++ )
	{
		query.next();
		for( int col_i = 0; col_i < number_of_columns; col_i++ )
		{
			QVariant current_value = query.value( col_i );
			this->current_meta_data[ row_i ].push_back( current_value );
		}
	}
}

void SQL_Tree_Widget::Recursive_Build( const std::vector<QVariantList> & row_data, QTreeWidgetItem* parent_tree, int current_collectable_i )
{
	if( current_collectable_i == what_to_collect.size() )
		return;

	const std::vector<QVariant> unique_elements = fn::cfrom( row_data )
		% fn::unique_all_by( [current_collectable_i]( const auto & e ) { return e[ current_collectable_i ]; } )
		% fn::transform( [current_collectable_i]( const auto & e ) { return e[ current_collectable_i ]; } )
		% fn::sort();

	// Now add a new node for each unique element (in this column)
	for( QVariant element : unique_elements )
	{
		const std::vector<QVariantList> matching_row_data = fn::cfrom( row_data )
			% fn::to_vector() // Make the vector that the results will be stored in
			% fn::where( [element, current_collectable_i]( const auto & row ) { return row[ current_collectable_i ] == element; } );

		QTreeWidgetItem* new_tree_branch = parent_tree; // Only add a new breakout for the first one, and if more than 1 child
		if( unique_elements.size() > 1 || current_collectable_i == 0 )
		{
			new_tree_branch = new QTreeWidgetItem( parent_tree );
			//if( !(current_value.size() > 1) )
			//	int i = 0;
		}
		new_tree_branch->setData( current_collectable_i, Qt::DisplayRole, element );
		//new_tree_branch->setText( current_collectable_i, element.toString() );
		Recursive_Build( matching_row_data, new_tree_branch, current_collectable_i + 1 );
	}
}

std::vector<const QTreeWidgetItem*> SQL_Tree_Widget::Get_Bottom_Children_Elements_Under( const QTreeWidgetItem* tree_item ) const
{
	int number_of_children = tree_item->childCount();
	if( number_of_children == 0 )
		return { tree_item };

	std::vector<const QTreeWidgetItem*> lowest_level_children;

	for( int i = 0; i < number_of_children; i++ )
	{
		std::vector<const QTreeWidgetItem*> i_lowest_level_children = Get_Bottom_Children_Elements_Under( tree_item->child( i ) );
		lowest_level_children.insert( lowest_level_children.end(), i_lowest_level_children.begin(), i_lowest_level_children.end() );
	}

	return lowest_level_children;
}

std::vector< SQL_Tree_Widget::Metadata > SQL_Tree_Widget::Get_Metadata_For_Rows( const std::vector<const QTreeWidgetItem*> tree_items ) const
{
	std::vector< std::vector<QVariant> > row_data;
	for( const QTreeWidgetItem* tree_node : tree_items )
	{
		row_data.push_back( Get_Metadata_For_Row( tree_node ) );
	}

	return row_data;
}

SQL_Tree_Widget::Metadata SQL_Tree_Widget::Get_Metadata_For_Row( const QTreeWidgetItem* tree_item ) const
{
	std::vector<const QTreeWidgetItem*> nodes_to_root;
	for( const QTreeWidgetItem* current_node = tree_item; current_node != this->invisibleRootItem() && current_node != nullptr; current_node = current_node->parent() )
		nodes_to_root.push_back( current_node );

	const int number_of_columns = this->header_titles.size();
	Metadata element_data( number_of_columns );

	// Iterate in reverse (from root to child)
	for( size_t i = 0; i < nodes_to_root.size(); i++ )
	{
		for( int current_column = 0; current_column < number_of_columns; current_column++ )
		{
			QVariant contents_of_cell = nodes_to_root[ i ]->data( current_column, Qt::DisplayRole );
			if( contents_of_cell != QVariant::Invalid && contents_of_cell.toString() != "" )
				element_data[ current_column ] = contents_of_cell;
		}
	}

	return element_data;
}

