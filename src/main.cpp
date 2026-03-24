#include "userinterface/dashboardui.h"
#include "steamvrcontrol/steamvrlogic.h"
#include "frameHandler.h"
#include <iostream>
#include <QtWidgets/QApplication>
#include <QThread>

int main(int argc, char *argv[])
{
    std::cout << "Can I even print anything wtf?" << std::endl;

    QApplication a(argc, argv);

    DashboardUI *pDashboardUI = new DashboardUI;
    QThread *frameThread = new QThread;

    SteamVRLogic::SharedInstance()->Init();
    SteamVRLogic::SharedInstance()->SetWidget(pDashboardUI);

    FrameHandler *frameHandler = new FrameHandler(SteamVRLogic::SharedInstance()->GetHeadsetRefreshRate(),  SteamVRLogic::SharedInstance()->GetHeadsetMaxFrameRate(), pDashboardUI);
    frameHandler->moveToThread(frameThread);

    QObject::connect(frameThread, &QThread::started, frameHandler, &FrameHandler::run);
    std::cout << "Started the thread (maybe)" << std::endl;

    frameThread->start(QThread::NormalPriority);

    return a.exec();
}