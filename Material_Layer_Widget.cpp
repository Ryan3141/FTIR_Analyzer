#include "Material_Layer_Widget.h"

#include <QComboBox>

Material_Layer_Widget::Material_Layer_Widget( QWidget *parent, const QStringList & material_names, QString material, double thickness, double composition )
	: QWidget(parent)
{
	ui.setupUi(this);
	//QComboBox x;
	////x.addItems;
	ui.material_comboBox->addItem( "" );
	ui.material_comboBox->addItems( material_names );
	ui.material_comboBox->setCurrentText( material );

	ui.thickness_doubleSpinBox->setValue( thickness );
	ui.composition_doubleSpinBox->setValue( composition );

	connect( ui.material_comboBox, qOverload<const QString &>( &QComboBox::currentIndexChanged ), [ this ]( const QString & new_value )
	{
		if( new_value == "Delete" )
			emit Delete_Requested();
		else
		{
			if( ui.material_comboBox->itemData( 0 ).toString() == "" && new_value != "" )
			{
				bool oldState = ui.material_comboBox->blockSignals( true ); // Prevent remove from triggering another currentIndexChanged signal
				ui.material_comboBox->removeItem( 0 );
				ui.material_comboBox->addItem( "Delete" );
				ui.material_comboBox->blockSignals( oldState );
				emit New_Material_Created();
			}
			emit Material_Changed();
		}
		this->previous_value = new_value;
	} );
	connect( ui.thickness_doubleSpinBox, qOverload<double>( &QDoubleSpinBox::valueChanged ), [this]( double new_value )
	{
		emit Material_Changed();
	} );
	connect( ui.composition_doubleSpinBox, qOverload<double>( &QDoubleSpinBox::valueChanged ), [this]( double new_value )
	{
		emit Material_Changed();
	} );
}


std::tuple<std::string, double, double> Material_Layer_Widget::Get_Details() const
{
	std::string mat_name = ui.material_comboBox->currentText().toStdString();
	double thickness = ui.thickness_doubleSpinBox->value() * 1E-6;
	double composition = ui.composition_doubleSpinBox->value();
	return { mat_name, thickness, composition };
}
