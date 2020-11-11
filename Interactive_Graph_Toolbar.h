#pragma once

#include <QWidget>
#include "ui_Interactive_Graph_Toolbar.h"

class Interactive_Graph;

class Interactive_Graph_Toolbar : public QWidget
{
	Q_OBJECT

public:
	Interactive_Graph_Toolbar(QWidget *parent = Q_NULLPTR);
	void Connect_To_Graph( Interactive_Graph* graph );

private:
	Ui::Interactive_Graph_Toolbar ui;
};
