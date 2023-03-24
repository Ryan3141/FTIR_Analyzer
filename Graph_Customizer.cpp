#include "Graph_Customizer.h"

#include <QLabel>
#include <QDoubleSpinBox>
#include <QColorDialog>
#include <QPushButton>

Graph_Customizer::Graph_Customizer( QWidget* parent )
	: QWidget(parent)
{
	this->setLayout( &layout );
}

void Graph_Customizer::clearLayout()
{
	QLayoutItem* item;
	while( item = layout.takeAt( 0 ) )
	{
		delete item->widget();
		delete item;
	}
}

//template<typename Graph_Adjustment>
//void Graph_Customizer::New_Graph_Selected( const std::vector<Graph_Adjustment> & label_type_value_list )
void Graph_Customizer::New_Graph_Selected( const std::vector<Graph_Double_Adjustment>& label_type_value_list )
{
	clearLayout();

	int i = 0;
	for( auto& [text, initial_value, action, min, max, step, suffix] : label_type_value_list )
	{
		QLabel* label = new QLabel( text, this );
		layout.addWidget( label, i, 0 );
		auto* spinbox = new QDoubleSpinBox( this );
		spinbox->setMinimum( min );
		spinbox->setMaximum( max );
		spinbox->setSingleStep( step );
		spinbox->setSuffix( suffix );
		spinbox->setValue( initial_value );
		layout.addWidget( spinbox, i++, 1);
		connect( spinbox, qOverload<double>( &QDoubleSpinBox::valueChanged ), [ action ]( double new_value ){ action( new_value ); } );
	}

	QPushButton* color_button = new QPushButton( this );
	connect( color_button, &QPushButton::clicked, [ this ] {
		QColorDialog* color = new QColorDialog( this );
		//QColorDialog* color = new QColorDialog( g->pen().color(), this );
		layout.addWidget( color );
	} );
	layout.addWidget( new QLabel( "Line Color", this ), i, 0);
	layout.addWidget( color_button, i++, 1 );
}
