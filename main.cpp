#include "WidgetTorrentsController.h"

#include <QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	WidgetTorrentsController w;
	w.show();
	return a.exec();
}
