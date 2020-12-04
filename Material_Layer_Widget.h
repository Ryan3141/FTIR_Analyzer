#pragma once

#include <QWidget>
#include "ui_Material_Layer_Widget.h"

class Material_Layer_Widget : public QWidget
{
	Q_OBJECT

public:
	Material_Layer_Widget( QWidget *parent = Q_NULLPTR, const QStringList & material_names = {}, QString material = "", double thickness = 1.0, double composition = 0.5 );

	std::tuple<std::string, double, double> Get_Details() const;

private:
	QString previous_value;

	Ui::Material_Layer_Widget ui;

signals:
	void New_Material_Created();
	void Material_Changed();
	void Delete_Requested();

};
