#pragma once

#include <QWidget>
#include "ui_Material_Layer_Widget.h"

#include "Thin_Film_Interference.h"

class Material_Layer_Widget : public QWidget
{
	Q_OBJECT

public:
	Material_Layer_Widget( QWidget *parent = Q_NULLPTR, const QStringList & material_names = {} );
	~Material_Layer_Widget();

	std::tuple<bool, Material_Layer> Get_Details() const;

private:
	QString previous_value;

	Ui::Material_Layer_Widget ui;

signals:
	void Material_Changed( QString previous_value, QString new_value );
	void Thickness_Changed( double new_value );
	void Composition_Changed( double new_value );
	void Delete_Requested();

};
