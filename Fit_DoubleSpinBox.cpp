#include "Fit_DoubleSpinBox.h"

Fit_DoubleSpinBox::Fit_DoubleSpinBox(QWidget *parent)
	: QDoubleSpinBox(parent)
{
	//ui.setupUi(this);
}

void Fit_DoubleSpinBox::keyPressEvent( QKeyEvent *event )
{
	if( event->key() == Qt::Key_Question )
	{
		use_in_fit = !use_in_fit;

		if( use_in_fit )
			this->setStyleSheet( "QDoubleSpinBox { background-color: rgba(0,255,0,255); color: rgba(0, 0, 0,255) }" );
		else
			this->setStyleSheet( "QDoubleSpinBox { background-color: rgba(255,255,255,255); color: rgba(0, 0, 0,255) }" );
	}
	else
		QDoubleSpinBox::keyPressEvent( event );
}
