#pragma once

#include <QMainWindow>
#include <QObject>
#include <QLabel>
#include "ui_Main.h"

#include "Web_Listener.h"

class Main_Widget : public QMainWindow
{
	Q_OBJECT

signals:

public slots:

public:
	Ui::Main ui;
	Web_Listener web_listener;

	Main_Widget( QWidget *parent = Q_NULLPTR );

	void Prepare_New_Tab( QLabel* statusLabel );
	void Run_Command( QString command );
};
