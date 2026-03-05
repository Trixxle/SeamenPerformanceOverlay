#include "userinterface/dashboardui.h"
#include "steamvrcontrol/steamvrlogic.h"
#include <iostream>
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    std::cout << "Can I even print anything wtf?" << std::endl;

    QApplication a(argc, argv);
    DashboardUI *pDashboardUI = new DashboardUI;

    SteamVRLogic::SharedInstance()->Init();
    SteamVRLogic::SharedInstance()->SetWidget(pDashboardUI);
    
    return a.exec();
}