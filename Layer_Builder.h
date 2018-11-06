#pragma once

#include <QtCore/QObject>
#include <QListWidget>

#include "Thin_Film_Interference.h"

class Layer_Builder : public QListWidget
{
	Q_OBJECT

public:
	Layer_Builder( QWidget *parent = Q_NULLPTR );
	~Layer_Builder();
	void Set_Material_List( const QStringList & material_names );

private:
	void Add_New_Material( const QStringList & material_names );
	std::vector<Material_Layer> Build_Material_List() const;
	void dropEvent( QDropEvent* event );


signals:
	void Materials_List_Changed( const std::vector<Material_Layer> & layers );
};
