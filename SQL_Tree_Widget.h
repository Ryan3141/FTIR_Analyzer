#pragma once
#include <QTreeWidget>
#include <vector>
#include <QtSql>
#include <QVariant>

#include "SQL_Manager.h"

class SQL_Tree_Widget :
	public QTreeWidget
{
	Q_OBJECT

signals:
	void Data_Double_Clicked( ID_To_Metadata output );

public:
	SQL_Tree_Widget( QWidget* parent = nullptr );
	void Set_Column_Info( const QStringList & header_titles, const QStringList & what_to_collect, int columns_to_show );
	QStringList Get_Displayed_Column_Headers() const;

	void Refilter( QString filter_string );

	void Recursive_Build( const std::vector<Metadata> & row_data, QTreeWidgetItem* parent_tree, int current_collectable_i, const std::vector<int> & convert_header_index );

	std::vector< Metadata > Get_Metadata_For_Rows( const std::vector<const QTreeWidgetItem*> tree_items ) const;

	//void Exact_Selected_Row( QPoint pos );
	ID_To_Metadata Selected_Data();
	Structured_Metadata Selected_Data_Order();

	Structured_Metadata current_meta_data;

private:
	Metadata Get_Metadata_For_Row( const QTreeWidgetItem* tree_item, const std::vector<int> & displayed_index_to_original ) const;

	std::vector<const QTreeWidgetItem*> Get_Bottom_Children_Elements_Under( const std::vector<const QTreeWidgetItem*> & tree_items ) const;
	std::vector<const QTreeWidgetItem*> Get_Bottom_Children_Elements_Under( const QTreeWidgetItem* tree_item ) const;

	QStringList header_titles;
	QStringList what_to_collect;
	int columns_to_show = 0;
	int measurement_id_column = 0;
	QString filter_by;
};

