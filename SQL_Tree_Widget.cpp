#include "SQL_Tree_Widget.h"

#include <QHeaderView>

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

	connect( this, &SQL_Tree_Widget::itemDoubleClicked, [ this ]( QTreeWidgetItem* tree_item, int column )
	{
		emit Data_Double_Clicked( this->Selected_Data() );
	} );
}

void SQL_Tree_Widget::Set_Column_Info( const QStringList & header_titles, const QStringList & what_to_collect, int columns_to_show )
{
	this->header_titles = header_titles;
	this->what_to_collect = what_to_collect;
	this->columns_to_show = columns_to_show;
	this->measurement_id_column = what_to_collect.indexOf( "measurement_id" );
	//shown_header_titles.insert( shown_header_titles.begin() + 1, "" ); // Need to have a blank first column to allow first column reordering, thanks Qt

	this->setHeaderLabels( header_titles );
	this->header()->setFirstSectionMovable( true );

	for( int i = columns_to_show; i < header_titles.size(); i++ )
		this->hideColumn( i );

	connect( this->header(), &QHeaderView::sectionMoved,
			 [ this ]( int logicalIndex, int oldVisualIndex, int newVisualIndex )
	{
		auto h = this->header();
		bool oldState = h->blockSignals( true ); // Prevent column shifting from triggering another changed signal
		{ // Do this while signals are turned off
			QStringList reordered_headers = Get_Displayed_Column_Headers();
			this->header()->moveSection( newVisualIndex, oldVisualIndex ); // Move columns immediately back, I'll handle the ordering
			this->setHeaderLabels( reordered_headers );
		}
		h->blockSignals( oldState );
		this->Refilter( this->filter_by );
	} );
}

void SQL_Tree_Widget::Refilter( QString filter_string )
{
	this->filter_by = filter_string;
	this->clear();
	this->setSizeAdjustPolicy( QAbstractScrollArea::AdjustToContents );
	int measurment_name_i = header_titles.indexOf( "Sample Name" );

	//std::vector<Metadata> filtered_data( this->current_meta_data.size() );
	//auto last_match = std::copy_if( this->current_meta_data.begin(), this->current_meta_data.end(), filtered_data.begin(),
	//								[measurment_name_i, filter_string]( const Metadata & row ) { return row[ measurment_name_i ].toString().contains( filter_string, Qt::CaseInsensitive ); } );
	//filtered_data.resize( std::distance( filtered_data.begin(), last_match ) );

	std::vector<Metadata> filtered_data = this->current_meta_data.data
		% fn::where( [ measurment_name_i, filter_string ]( const auto & row ) { return row[ measurment_name_i ].toString().contains( filter_string, Qt::CaseInsensitive ); } );

	const std::vector<int> displayed_index_to_original = Get_Displayed_Column_Headers()
		% fn::transform( [ this ]( const auto & new_header ) -> int { return header_titles.indexOf( new_header ); } )
		% fn::to_vector();

	Recursive_Build( filtered_data, this->invisibleRootItem(), 0, displayed_index_to_original );

	for( int i = 0; i < header_titles.size() + 1; i++ )
		this->resizeColumnToContents( i );
}

QStringList SQL_Tree_Widget::Get_Displayed_Column_Headers() const
{
	QStringList headers;
	for( int i = 0; i < this->model()->columnCount(); i++ )
	{
		//this->header()->isSectionHidden( int logicalIndex );
		auto test2 = this->header()->logicalIndex( i );
		auto test = this->model()->headerData( this->header()->logicalIndex( i ), Qt::Horizontal );
		auto test3 = this->model()->headerData( i, Qt::Horizontal );
		headers.append( this->model()->headerData( this->header()->logicalIndex( i ), Qt::Horizontal ).toString() );
	}

	return headers;
}

void SQL_Tree_Widget::Recursive_Build( const std::vector<Metadata> & row_data, QTreeWidgetItem* parent_tree, int column_i, const std::vector<int> & convert_header_index )
{
	if( column_i == convert_header_index.size() )
		return;
	assert( column_i >= 0 && column_i < convert_header_index.size() );
	int data_i = convert_header_index[ column_i ]; // Location in the unshifted dataset
	const std::vector<QVariant> unique_elements = row_data
		% fn::unique_all_by( [ data_i ]( const auto & e ) { return e[ data_i ]; } )
		% fn::transform(     [ data_i ]( const auto & e ) { return e[ data_i ]; } )
		% fn::sort();

	// Now add a new node for each unique element (in this column)
	for( QVariant element : unique_elements )
	{
		const std::vector<Metadata> matching_row_data = fn::cfrom( row_data )
			% fn::to_vector() // Make the vector that the results will be stored in
			% fn::where( [ element, data_i ]( const auto & row ) { return row[ data_i ] == element; } );

		QTreeWidgetItem* new_tree_branch = parent_tree; // Only add a new breakout for the first one, and if more than 1 child
		if( unique_elements.size() > 1 || column_i == 0 )
		{
			new_tree_branch = new QTreeWidgetItem( parent_tree );
		}
		new_tree_branch->setData( column_i, Qt::DisplayRole, element );
		//new_tree_branch->setText( column_i, element.toString() );
		Recursive_Build( matching_row_data, new_tree_branch, column_i + 1, convert_header_index );
	}
}

std::vector<const QTreeWidgetItem*> SQL_Tree_Widget::Get_Bottom_Children_Elements_Under( const std::vector<const QTreeWidgetItem*> & tree_items ) const
{
	std::vector<const QTreeWidgetItem*> lowest_level_children;

	for( const QTreeWidgetItem* tree_item : tree_items )
	{
		int number_of_children = tree_item->childCount();
		if( number_of_children == 0 )
		{
			lowest_level_children.push_back( tree_item );
			continue;
		}

		std::vector<const QTreeWidgetItem*> children;
		for( int i = 0; i < number_of_children; i++ )
			children.push_back( tree_item->child( i ) );

		const std::vector<const QTreeWidgetItem*> i_lowest_level_children = Get_Bottom_Children_Elements_Under( children );
		lowest_level_children.insert( lowest_level_children.end(), i_lowest_level_children.begin(), i_lowest_level_children.end() );
	}

	return lowest_level_children;
}

std::vector<const QTreeWidgetItem*> SQL_Tree_Widget::Get_Bottom_Children_Elements_Under( const QTreeWidgetItem* tree_item ) const
{
	return Get_Bottom_Children_Elements_Under( { tree_item } );
}

std::vector< Metadata > SQL_Tree_Widget::Get_Metadata_For_Rows( const std::vector<const QTreeWidgetItem*> tree_items ) const
{
	const std::vector<int> displayed_index_to_original = Get_Displayed_Column_Headers()
		% fn::transform( [ this ]( const auto & new_header ) -> int { return header_titles.indexOf( new_header ); } )
		% fn::to_vector();
	std::vector< std::vector<QVariant> > row_data;
	for( const QTreeWidgetItem* tree_node : tree_items )
	{
		row_data.push_back( Get_Metadata_For_Row( tree_node, displayed_index_to_original ) );
	}

	return row_data;
}

Metadata SQL_Tree_Widget::Get_Metadata_For_Row( const QTreeWidgetItem* tree_item, const std::vector<int> & displayed_index_to_original ) const
{
	const int number_of_columns = this->header_titles.size();
	Metadata element_data( number_of_columns );

	for( const QTreeWidgetItem* current_node = tree_item; current_node != this->invisibleRootItem() && current_node != nullptr; current_node = current_node->parent() )
	{
		for( int current_column = 0; current_column < number_of_columns; current_column++ )
		{
			QVariant contents_of_cell = current_node->data( current_column, Qt::DisplayRole );
			if( contents_of_cell != QVariant::Invalid && contents_of_cell.toString() != "" )
				element_data[ displayed_index_to_original[ current_column ] ] = contents_of_cell;
		}
	}

	return element_data;
}

ID_To_Metadata SQL_Tree_Widget::Selected_Data()
{
	//auto actually_clicked = this->itemAt( pos );
	std::vector<const QTreeWidgetItem*> selected_stuff = this->Get_Bottom_Children_Elements_Under( this->selectedItems() % fn::to( std::vector<const QTreeWidgetItem*>{} ) );

	const std::vector<int> displayed_index_to_original = Get_Displayed_Column_Headers()
		% fn::transform( [ this ]( const auto & new_header ) -> int { return header_titles.indexOf( new_header ); } )
		% fn::to_vector();
	ID_To_Metadata output;
	for( const QTreeWidgetItem* tree_item : selected_stuff )
	{
		QString measurement_id = tree_item->text( this->measurement_id_column );
		output[ measurement_id ] = this->Get_Metadata_For_Row( tree_item, displayed_index_to_original );
	}

	return output;
}