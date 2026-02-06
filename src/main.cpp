#include "userinterface/dashboardui.h"
#include "steamvrcontrol/steamvrlogic.h"
#include <iostream>
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    // TODO: Currently printing to console doesn't seem to work. This will make debugging a nightmare. Must fix in the future.
    std::cout << "Can I even print anything wtf?" << std::endl;

    QApplication a(argc, argv);
    DashboardUI *pDashboardUI = new DashboardUI;

    SteamVRLogic::SharedInstance()->Init();
    SteamVRLogic::SharedInstance()->SetWidget(pDashboardUI);

    return 0;
}