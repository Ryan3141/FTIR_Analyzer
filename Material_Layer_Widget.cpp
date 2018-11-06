#include "Material_Layer_Widget.h"

#include <QComboBox>

Material_Layer_Widget::Material_Layer_Widget( QWidget *parent, const QStringList & material_names )
	: QWidget(parent)
{
	ui.setupUi(this);
	//QComboBox x;
	////x.addItems;
	ui.material_comboBox->addItem( "" );
	ui.material_comboBox->addItems( material_names );

	ui.composition_lineEdit->setText( "0" );
	ui.thickness_lineEdit->setText( "1" );

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
			}
			emit Material_Changed( previous_value, new_value );
		}
		this->previous_value = new_value;
	} );
	connect( ui.thickness_lineEdit, &QLineEdit::textChanged, [this]( const QString & new_value )
	{
		emit Thickness_Changed( new_value.toDouble() );
	} );
	connect( ui.composition_lineEdit, &QLineEdit::textChanged, [this]( const QString & new_value )
	{
		emit Composition_Changed( new_value.toDouble() );
	} );
}

Material_Layer_Widget::~Material_Layer_Widget()
{
}

std::tuple<bool, Material_Layer> Material_Layer_Widget::Get_Details() const
{
	double thickness = ui.thickness_lineEdit->text().toDouble() * 1E-6;
	double composition = ui.composition_lineEdit->text().toDouble();
	std::string mat_name = ui.material_comboBox->currentText().toStdString();
	if( mat_name == "" )
		return { false, Material_Layer{} };
	Material mat = name_to_material[ mat_name ];
	return { true, Material_Layer{ mat, composition, thickness } };
}
