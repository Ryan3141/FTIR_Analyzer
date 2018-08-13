#include "FTIR_Analyzer.h"
#include <QtWidgets/QApplication>

#include <armadillo>

#include "Thin_Film_Interference.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	FTIR_Analyzer w;
	w.show();
	return a.exec();
}
