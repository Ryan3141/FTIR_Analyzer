#include "FTIR_Analyzer.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	FTIR_Analyzer w;
	w.show();
	return a.exec();
}
