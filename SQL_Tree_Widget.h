#pragma once
#include <QTreeWidget>
#include <QtSql>
#include <vector>

class SQL_Tree_Widget :
	public QTreeWidget
{
public:
	SQL_Tree_Widget( QWidget* parent = nullptr );
	~SQL_Tree_Widget();
	void Set_Data_To_Gather( const QStringList & header_titles, const QStringList & what_to_collect );

	void Refilter( QString filter_string );
	void Repoll_SQL( QSqlDatabase & sql_db, QString user );

	void Recursive_Build( const std::vector<QVariantList> & row_data, QTreeWidgetItem* parent_tree, int current_collectable_i );

	std::vector<const QTreeWidgetItem*> Get_Bottom_Children_Elements_Under( const QTreeWidgetItem * tree_item ) const;

private:
	QStringList header_titles;
	QStringList what_to_collect;

	std::vector<QVariantList> current_meta_data;
};

