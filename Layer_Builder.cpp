#include "Layer_Builder.h"

#include <QComboBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QLabel>

#include "Material_Layer_Widget.h"

Layer_Builder::Layer_Builder(QWidget *parent )
	: QListWidget(parent)
{
	//for( int i = 0; i < 10; i++ )
	//{
	//	QListWidgetItem *iconItem = new QListWidgetItem( this );
	//	//QIcon icon;
	//	//icon.addFile( list.at( i ).absoluteFilePath(), QSize(), QIcon::Normal,
	//	//			  QIcon::Off );
	//	//iconItem->setIcon( icon );
	//	//QComboBox *box = new QComboBox( (QWidget*)iconItem );

	//	//QWidget* frame = new QWidget;
	//	//QHBoxLayout *layout2 = new QHBoxLayout;
	//	//layout2->addWidget( new QPushButton( "xyz" ) );
	//	//layout2->addWidget( new QComboBox );
	//	//layout2->addWidget( new QComboBox );
	//	//QLabel *label = new QLabel();
	//	//label->setText( "test" );
	//	//layout2->addWidget( label );
	//	////frame->setFrameShape( QFrame::StyledPanel );
	//	////frame->setFrameShadow( QFrame::Raised );

	//	////QComboBox *box = new QComboBox( frame );
	//	////QComboBox *box2 = new QComboBox( frame );
	//	//frame->setLayout( layout2 );

	//	//QHBoxLayout* testing = new QHBoxLayout;
	//	//testing->setSizeConstraint( QLayout::SetFixedSize );
	//	Material_Layer_Widget* one_material = new Material_Layer_Widget;
	//	iconItem->setSizeHint( one_material->sizeHint() );
	//	this->addItem( iconItem );
	//	this->setItemWidget( iconItem, one_material );
	//}
}

Layer_Builder::~Layer_Builder()
{
}

void Layer_Builder::dropEvent( QDropEvent* event )
{
	QListWidget::dropEvent( event );
	emit Materials_List_Changed( Build_Material_List() );
}

void Layer_Builder::Add_New_Material( const QStringList & material_names )
{
	QListWidgetItem *iconItem = new QListWidgetItem( this );
	Material_Layer_Widget* one_material = new Material_Layer_Widget( this, material_names );
	connect( one_material, &Material_Layer_Widget::Material_Changed, [this, material_names]( QString previous_value, QString new_value )
	{
		if( previous_value == "" )
		{
			this->Add_New_Material( material_names );
		}
		emit Materials_List_Changed( Build_Material_List() );
	} );

	connect( one_material, &Material_Layer_Widget::Thickness_Changed, [this]( double new_value )
	{
		emit Materials_List_Changed( Build_Material_List() );
	} );

	connect( one_material, &Material_Layer_Widget::Composition_Changed, [this]( double new_value )
	{
		emit Materials_List_Changed( Build_Material_List() );
	} );

	connect( one_material, &Material_Layer_Widget::Delete_Requested, [iconItem, this]
	{
		this->removeItemWidget( iconItem );
		delete iconItem;
		emit Materials_List_Changed( Build_Material_List() );
	} );

	iconItem->setSizeHint( one_material->sizeHint() );
	this->addItem( iconItem );
	this->setItemWidget( iconItem, one_material );
}

std::vector<Material_Layer> Layer_Builder::Build_Material_List() const
{
	std::vector<Material_Layer> output;

	for( int i = 0; i < this->count(); i++ )
	{
		const Material_Layer_Widget* layer = (const Material_Layer_Widget*)this->itemWidget( this->item( i ) );
		auto [is_valid, description] = layer->Get_Details();
		
		if( is_valid )
			output.push_back( description );
	}
	return output;
}

void Layer_Builder::Set_Material_List( const QStringList & material_names )
{
	Add_New_Material( material_names );
}