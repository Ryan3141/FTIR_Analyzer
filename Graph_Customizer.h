#pragma once

#include <QWidget>
#include <QGridLayout>

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

	//template<typename Graph_Adjustment>
	//void New_Graph_Selected( const std::vector<Graph_Adjustment>& label_type_value_list );
	void New_Graph_Selected( const std::vector<Graph_Double_Adjustment>& label_type_value_list );
};
