#pragma once

#include <optional>
#include <array>
#include <string>

#include <QWidget>
#include "ui_Material_Layer_Widget.h"

#include "Thin_Film_Interference.h"

class Material_Layer_Widget : public QWidget
{
	Q_OBJECT

public:
	Material_Layer_Widget( QWidget *parent,
						   const QStringList & material_names,
						   Optional_Material_Parameters parameters );

	std::tuple< Optional_Material_Parameters, std::array< bool, 4 > > Get_Details() const;
	void Update_Values( Optional_Material_Parameters parameters );

	Ui::Material_Layer_Widget ui;
	std::array< Fit_DoubleSpinBox*, 4 > double_widgets;
private:


signals:
	void New_Material_Created();
	void Material_Changed();
	void Material_Values_Changed();
	void Delete_Requested();

};
