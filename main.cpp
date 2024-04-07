#include "ModelViewer.h"
#include <QtWidgets/QApplication>

int main(int argc, char* argv[])
{
	QLocale::setDefault(QLocale::c());

	QCoreApplication::setOrganizationName("MPM");
	QCoreApplication::setApplicationName("ModelViewer");

	QApplication a(argc, argv);
	ModelViewer w;
	w.show();
	return a.exec();
}