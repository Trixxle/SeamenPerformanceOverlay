#include "userinterface/dashboardui.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    dashboardui w;
    w.show();
    return a.exec();
}
