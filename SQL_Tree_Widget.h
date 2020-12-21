#pragma once
#include <QTreeWidget>
#include <vector>
#include <QtSql>
#include <QVariant>


class SQL_Tree_Widget :
	public QTreeWidget
{
public:
	using Metadata = std::vector<QVariant>;

	SQL_Tree_Widget( QWidget* parent = nullptr );
	void Set_Data_To_Gather( const QStringList & header_titles, const QStringList & what_to_collect, int columns_to_show );

	void Refilter( QString filter_string );

	void Recursive_Build( const std::vector<Metadata> & row_data, QTreeWidgetItem* parent_tree, int current_collectable_i );

	std::vector<const QTreeWidgetItem*> Get_Bottom_Children_Elements_Under( const QTreeWidgetItem * tree_item ) const;
	std::vector< Metadata > Get_Metadata_For_Rows( const std::vector<const QTreeWidgetItem*> tree_items ) const;
	Metadata Get_Metadata_For_Row( const QTreeWidgetItem* tree_item ) const;

	std::vector<Metadata> current_meta_data;

private:
	QStringList header_titles;
	QStringList what_to_collect;
	int columns_to_show = 0;

};

