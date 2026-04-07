#include "userinterface/dashboardui.h"
#include "steamvrcontrol/steamvrlogic.h"
#include "frameHandler.h"
#include <iostream>
#include <QtWidgets/QApplication>
#include <QThread>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    DashboardUI *pDashboardUI = new DashboardUI;
    QThread *frameThread = new QThread;

    if (!SteamVRLogic::SharedInstance()->Init()) {
        std::cerr << "Failed to initialize SteamVR Logic. Exiting." << std::endl;
        return -1;
    }

    SteamVRLogic::SharedInstance()->SetWidget(pDashboardUI);

    FrameHandler *frameHandler = new FrameHandler(SteamVRLogic::SharedInstance()->GetHeadsetRefreshRate(),
        SteamVRLogic::SharedInstance()->GetHeadsetMaxFrameRate(),
        pDashboardUI
        );
    frameHandler->moveToThread(frameThread);

    QObject::connect(frameThread, &QThread::started, frameHandler, &FrameHandler::run);
    std::cout << "Started the thread (maybe)" << std::endl;

    frameThread->start(QThread::NormalPriority);

    return a.exec();
}