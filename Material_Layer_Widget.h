#pragma once

#include <optional>
#include <array>
#include <string>

#include <QWidget>
#include "ui_Material_Layer_Widget.h"

struct Material_Adjustable_Parameters
{
	Material_Adjustable_Parameters( std::string material ) : all()
	{
		this->material = material;
	}

	std::string material;
	union
	{
		struct
		{
			std::optional< double > thickness;
			std::optional< double > composition;
			std::optional< double > tauts_gap;
			std::optional< double > urbach_energy;
		};

		std::optional< double > all[ 4 ];
	};
};

class Material_Layer_Widget : public QWidget
{
	Q_OBJECT

public:
	Material_Layer_Widget( QWidget *parent,
						   const QStringList & material_names,
						   Material_Adjustable_Parameters parameters );

	std::tuple< Material_Adjustable_Parameters, std::array< bool, 4 > > Get_Details() const;

	Ui::Material_Layer_Widget ui;
	std::array< Fit_DoubleSpinBox*, 4 > double_widgets;
private:


signals:
	void New_Material_Created();
	void Material_Changed();
	void Material_Values_Changed();
	void Delete_Requested();

};
