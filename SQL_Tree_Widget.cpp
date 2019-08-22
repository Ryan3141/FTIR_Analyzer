#include "SQL_Tree_Widget.h"

#include <algorithm>

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
	QStringList header_titles{ "Sample Name", "Date", "Temperature (K)", "Bias (V)", "Time of Day", "measurement_id" };
	QStringList what_to_collect{ "sample_name", "date(time)", "temperature_in_k", "bias_in_v", "time(time)", "measurement_id" }; // DATE_FORMAT(time, '%b %e %Y') DATE_FORMAT(time, '%H:%i:%s')
																																 //QStringList sql_queries{ "SELECT DISTINCT sample_name FROM ftir_measurements",
																																 //						 "SELECT DISTINCT date(time) FROM ftir_measurements WHERE sample_name is " };
	Set_Data_To_Gather( header_titles, what_to_collect );
}


SQL_Tree_Widget::~SQL_Tree_Widget()
{
}

void SQL_Tree_Widget::Set_Data_To_Gather( const QStringList & header_titles, const QStringList & what_to_collect )
{
	this->header_titles = header_titles;
	this->what_to_collect = what_to_collect;
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

void SQL_Tree_Widget::Repoll_SQL( QSqlDatabase & sql_db, QString user )
{
	if( !sql_db.isOpen() )
		return;

	QSqlQuery query( sql_db );
	this->setHeaderLabels( header_titles );
	this->hideColumn( header_titles.size() - 1 );

	QString query_string = QString( "SELECT %1 FROM ftir_measurements WHERE user=\"%2\"" ).arg( what_to_collect.join( "," ), Sanitize_SQL( user ) );
	query.prepare( query_string );
	if( !query.exec() )
	{
		qDebug() << "Error pulling data from ftir_measurments: "
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

	std::vector<QVariantList> unique_elements_in_rows( row_data.size() );
	auto last_unique_element = std::unique_copy( row_data.begin(), row_data.end(), unique_elements_in_rows.begin(),
												 [current_collectable_i]( const QVariantList & a, const QVariantList & b ) { return a[ current_collectable_i ] == b[ current_collectable_i ]; } );
	unique_elements_in_rows.resize( std::distance( unique_elements_in_rows.begin(), last_unique_element ) );

	std::vector<QVariant> unique_elements( unique_elements_in_rows.size() );
	for( int i = 0; i < unique_elements_in_rows.size(); i++ )
		unique_elements[ i ] = unique_elements_in_rows[ i ][ current_collectable_i ];
	std::sort( unique_elements.begin(), unique_elements.end() );

	for( QVariant element : unique_elements )
	{
		std::vector<QVariantList> matching_row_data( row_data.size() );
		auto last_match = std::copy_if( row_data.begin(), row_data.end(), matching_row_data.begin(),
					  [current_collectable_i, element]( const QVariantList & row ) { return row[ current_collectable_i ] == element; } );
		matching_row_data.resize( std::distance( matching_row_data.begin(), last_match ) );

		QTreeWidgetItem* new_tree_branch = parent_tree; // Only add a new breakout for the first one, and if more than 1 child
		if( unique_elements.size() > 1 || current_collectable_i == 0 )
		{
			new_tree_branch = new QTreeWidgetItem( parent_tree );
			//if( !(current_value.size() > 1) )
			//	int i = 0;
		}
		new_tree_branch->setText( current_collectable_i, element.toString() );
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

