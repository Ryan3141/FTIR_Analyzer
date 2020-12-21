#include "SQL_Tree_Widget.h"

#include <algorithm>
#include "fn.hpp"

namespace fn = rangeless::fn;
using fn::operators::operator%;   // arg % f % g % h; // h(g(f(std::forward<Arg>(arg))));
using fn::operators::operator%=;  // arg %= fn;       // arg = fn(std::move(arg));

SQL_Tree_Widget::SQL_Tree_Widget( QWidget* parent ) : QTreeWidget( parent )
{
	//QStringList header_titles{ "Sample Name", "Date", "Temperature (K)", "Dewar Temp (C)", "Time of Day", "measurement_id" };
	//QStringList what_to_collect{ "sample_name", "date(time)", "temperature_in_k", "dewar_temp_in_c", "time(time)", "measurement_id" }; // DATE_FORMAT(time, '%b %e %Y') DATE_FORMAT(time, '%H:%i:%s')
	//																															 //QStringList sql_queries{ "SELECT DISTINCT sample_name FROM ftir_measurements",
	//																															 //						 "SELECT DISTINCT date(time) FROM ftir_measurements WHERE sample_name is " };
	//Set_Data_To_Gather( header_titles, what_to_collect );
}


void SQL_Tree_Widget::Set_Data_To_Gather( const QStringList & header_titles, const QStringList & what_to_collect, int columns_to_show )
{
	this->header_titles = header_titles;
	this->what_to_collect = what_to_collect;
	this->columns_to_show = columns_to_show;

	this->setHeaderLabels( header_titles );
	for( int i = columns_to_show; i < header_titles.size(); i++ )
		this->hideColumn( i );
}

void SQL_Tree_Widget::Refilter( QString filter_string )
{
	this->clear();
	this->setSizeAdjustPolicy( QAbstractScrollArea::AdjustToContents );
	int measurment_name_i = header_titles.indexOf( "Sample Name" );

	std::vector<Metadata> filtered_data( this->current_meta_data.size() );
	auto last_match = std::copy_if( this->current_meta_data.begin(), this->current_meta_data.end(), filtered_data.begin(),
									[measurment_name_i, filter_string]( const Metadata & row ) { return row[ measurment_name_i ].toString().contains( filter_string, Qt::CaseInsensitive ); } );
	filtered_data.resize( std::distance( filtered_data.begin(), last_match ) );

	Recursive_Build( filtered_data, this->invisibleRootItem(), 0 );
	for( int i = 0; i < header_titles.size(); i++ )
		this->resizeColumnToContents( i );
}

void SQL_Tree_Widget::Recursive_Build( const std::vector<Metadata> & row_data, QTreeWidgetItem* parent_tree, int current_collectable_i )
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
		const std::vector<Metadata> matching_row_data = fn::cfrom( row_data )
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

