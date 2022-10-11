#include "Material_Layer_Widget.h"

#include <QComboBox>

#include "rangeless_helper.hpp"

Material_Layer_Widget::Material_Layer_Widget( QWidget *parent, const QStringList & material_names, Optional_Material_Parameters parameters )
	: QWidget(parent)
{
	ui.setupUi(this);

	double_widgets = { ui.thickness_doubleSpinBox, ui.composition_doubleSpinBox, ui.tautsGap_doubleSpinBox, ui.urbachEnergy_doubleSpinBox };
	//QComboBox x;
	////x.addItems;
	ui.material_comboBox->addItem( "" );
	ui.material_comboBox->addItems( material_names );
	if( parameters.material_name != "" )
		ui.material_comboBox->addItem( "Delete" );

	for( auto [ widget, parameter ] : fn::zip( double_widgets, parameters.all ) )
	{
		if( !parameter.has_value() )
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

	this->Update_Values( parameters );
}

void Material_Layer_Widget::Update_Values( Optional_Material_Parameters parameters )
{
	if( parameters.thickness.has_value() )
		parameters.thickness.value() *= 1E6;
	if( parameters.tauts_gap_eV.has_value() )
		parameters.tauts_gap_eV.value() *= 1E3;
	if( parameters.urbach_energy_eV.has_value() )
		parameters.urbach_energy_eV.value() *= 1E3;

	auto Avoid_Resignalling_setValue = []( auto going_to_change, auto value )
	{
		bool oldState = going_to_change->blockSignals( true ); // Prevent remove from triggering another changed signal
		if( value.has_value() )
		{
			going_to_change->setEnabled( true );
			going_to_change->setValue( value.value() );
		}
		else
			going_to_change->setEnabled( false );
		going_to_change->blockSignals( oldState );
	};

	auto Avoid_Resignalling_setCurrentText = []( auto going_to_change, auto value )
	{
		bool oldState = going_to_change->blockSignals( true ); // Prevent remove from triggering another changed signal
		going_to_change->setCurrentText( value );
		going_to_change->blockSignals( oldState );
	};

	Avoid_Resignalling_setCurrentText( this->ui.material_comboBox, QString::fromStdString( parameters.material_name ) );
	for( auto[ widget, parameter ] : fn::zip( this->double_widgets, parameters.all ) )
	{
		Avoid_Resignalling_setValue( widget, parameter );
	}
	//if( layer.optional.thickness.has_value() )
	//	Avoid_Resignalling_setValue( layer_widget->ui.thickness_doubleSpinBox, std::optional<double>{ layer.optional.thickness.value() * 1E6 } );

}

std::tuple< Optional_Material_Parameters, std::array< bool, 4 > >  Material_Layer_Widget::Get_Details() const
{
	std::string mat_name = ui.material_comboBox->currentText().toStdString();
	
	Optional_Material_Parameters parameters( mat_name );
	for( auto [ widget, parameter ] : fn::zip( double_widgets, parameters.all ) )
	{
		if( widget->isEnabled() )
			parameter = widget->value();
	}
	if( parameters.thickness.has_value() )
		parameters.thickness.value() *= 1E-6;
	if( parameters.tauts_gap_eV.has_value() )
		parameters.tauts_gap_eV.value() *= 1E-3;
	if( parameters.urbach_energy_eV.has_value() )
		parameters.urbach_energy_eV.value() *= 1E-3;

	std::array< bool, 4 > should_fit;
	for( auto [ widget, fit_bool ] : fn::zip( double_widgets, should_fit ) )
	{
		if( widget->isEnabled() )
			fit_bool = widget->use_in_fit;
		else
			fit_bool = false;
	}

	return { std::move( parameters ), std::move( should_fit ) };
}
