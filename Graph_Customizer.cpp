#include "Graph_Customizer.h"

#include <QLabel>
#include <QDoubleSpinBox>
#include <QColorDialog>
#include <QPushButton>
#include <QPen>

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

