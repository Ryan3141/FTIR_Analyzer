#pragma once

#include <QMainWindow>
#include <QObject>
#include <QLabel>
#include "ui_Main.h"

class Main_Widget : public QMainWindow
{
	Q_OBJECT

signals:

public slots:

public:
	Ui::Main ui;

	Main_Widget( QWidget *parent = Q_NULLPTR );

	void Prepare_New_Tab( QLabel* statusLabel );
};
