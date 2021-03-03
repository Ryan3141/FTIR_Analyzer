#include "Material_Layer_Widget.h"

#include <QComboBox>

#include "rangeless_helper.hpp"

Material_Layer_Widget::Material_Layer_Widget( QWidget *parent, const QStringList & material_names, Material_Adjustable_Parameters parameters )
	: QWidget(parent)
{
	ui.setupUi(this);

	double_widgets = { ui.thickness_doubleSpinBox, ui.composition_doubleSpinBox, ui.tautsGap_doubleSpinBox, ui.urbachEnergy_doubleSpinBox };
	//QComboBox x;
	////x.addItems;
	ui.material_comboBox->addItem( "" );
	ui.material_comboBox->addItems( material_names );
	ui.material_comboBox->setCurrentText( QString::fromStdString( parameters.material ) );
	if( parameters.material != "" )
		ui.material_comboBox->addItem( "Delete" );

	if( parameters.thickness.has_value() )
		parameters.thickness.value() *= 1E6;
	if( parameters.tauts_gap.has_value() )
		parameters.tauts_gap.value() *= 1E3;
	if( parameters.urbach_energy.has_value() )
		parameters.urbach_energy.value() *= 1E3;

	for( auto [ widget, parameter ] : fn::zip( double_widgets, parameters.all ) )
	{
		if( parameter.has_value() )
			widget->setValue( parameter.value() );
		else
			widget->setEnabled( false );
		connect( widget, qOverload<double>( &QDoubleSpinBox::valueChanged ), [ this ]( double new_value )
		{
			emit Material_Values_Changed();
		} );
	}

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
	} );
}


std::tuple< Material_Adjustable_Parameters, std::array< bool, 4 > >  Material_Layer_Widget::Get_Details() const
{
	std::string mat_name = ui.material_comboBox->currentText().toStdString();
	
	Material_Adjustable_Parameters parameters( mat_name );
	for( auto [ widget, parameter ] : fn::zip( double_widgets, parameters.all ) )
	{
		if( widget->isEnabled() )
			parameter = widget->value();
	}
	if( parameters.thickness.has_value() )
		parameters.thickness.value() *= 1E-6;
	if( parameters.tauts_gap.has_value() )
		parameters.tauts_gap.value() *= 1E-3;
	if( parameters.urbach_energy.has_value() )
		parameters.urbach_energy.value() *= 1E-3;

	std::array< bool, 4 > should_fit;
	for( auto [ widget, fit_bool ] : fn::zip( double_widgets, should_fit ) )
	{
		if( widget->isEnabled() )
			fit_bool = widget->use_in_fit;
	}

	return { std::move( parameters ), std::move( should_fit ) };
}
