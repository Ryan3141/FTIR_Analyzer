#pragma once

#include <QWidget>
#include <QGridLayout>
#include "qcustomplot.h"

//graphCustomizer
struct Graph_Double_Adjustment
{
	QString text;
	double initial_value;
	std::function<void( double )> action;
	double min_value = 0.0;
	double max_value = 50.0;
	double step = 0.1;
	QString suffix = " " + QString( QChar( 0x03BC ) ) + "s";
};

struct Graph_Color_Adjustment
{
	QString text;
	double initial_value;
	std::function<void( double )> action;
};

class Graph_Customizer  : public QWidget
{
	Q_OBJECT

private:
	QGridLayout layout;
	void clearLayout();

public:
	Graph_Customizer(QWidget *parent);

	template< typename SingleGraph >
	void New_Graph_Selected( const std::vector<Graph_Double_Adjustment>& label_type_value_list, SingleGraph & g )
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
		connect( color_button, &QPushButton::clicked, [ this, &g ] {
			QColorDialog* color = new QColorDialog( this );
			//QColorDialog* color = new QColorDialog( g->pen().color(), this );
			connect( color, &QColorDialog::colorSelected, [ this, &g ]( const QColor& color ) {
				g.SetColor( color );
			} );
			layout.addWidget( color );
		} );
		layout.addWidget( new QLabel( "Line Color", this ), i, 0);
		layout.addWidget( color_button, i++, 1 );

		// QPushButton* brush_button = new QPushButton( this );
		// connect( brush_button, &QPushButton::clicked, [ this, &g ] {
		// 		static int brush_style = 0;
		// 		QBrush brush( static_cast<Qt::BrushStyle>( (brush_style++) % 24 ) );
		// 		g->setBrush( brush );
		// 	} );
		// layout.addWidget( new QLabel( "Brush Style", this ), i, 0);
		// layout.addWidget( brush_button, i++, 1 );

	}
};
