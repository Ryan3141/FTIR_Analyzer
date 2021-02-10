#pragma once

#include <QDoubleSpinBox>
#include <QKeyEvent>

//#include "ui_Fit_DoubleSpinBox.h"

class Fit_DoubleSpinBox : public QDoubleSpinBox
{
	Q_OBJECT

public:
	Fit_DoubleSpinBox(QWidget *parent = Q_NULLPTR);

	void keyPressEvent( QKeyEvent *event ) override;

	bool use_in_fit = false;
//private:
//	Ui::Fit_DoubleSpinBox ui;
};
