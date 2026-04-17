#include "userinterface/dashboardui.h"
#include "steamvrcontrol/steamvrlogic.h"
#include "frameHandler.h"
#include <iostream>
#include <QtWidgets/QApplication>
#include <QThread>
#include <QMetaType>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    qRegisterMetaType<FrameHandler::frameStats>("frameStats");

    qRegisterMetaType<FrameHandler::frameStats>("FrameStatsList");

    if (!SteamVRLogic::SharedInstance()->Init()) {
        std::cerr << "Failed to initialize SteamVR Logic. Exiting." << std::endl;
        return -1;
    }

    DashboardUI *pDashboardUI = new DashboardUI(SteamVRLogic::SharedInstance()->GetHeadsetRefreshRate(),
        SteamVRLogic::SharedInstance()->GetHeadsetMaxFrameRate());
    QThread *frameThread = new QThread;

    SteamVRLogic::SharedInstance()->SetWidget(pDashboardUI);

    FrameHandler *frameHandler = new FrameHandler(SteamVRLogic::SharedInstance()->GetHeadsetRefreshRate(),
        SteamVRLogic::SharedInstance()->GetHeadsetMaxFrameRate()
        );

    frameHandler->moveToThread(frameThread);

    QObject::connect(frameThread, &QThread::started, frameHandler, &FrameHandler::startProcessing);
    QObject::connect(frameThread, &QThread::finished, frameHandler, &QObject::deleteLater);
    QObject::connect(frameHandler, &FrameHandler::updateGraphs, pDashboardUI, &DashboardUI::updateGraphs);
    QObject::connect(frameHandler, &FrameHandler::updateLabels, pDashboardUI, &DashboardUI::updateLabels);
    QObject::connect(pDashboardUI, &DashboardUI::requestControllerSwitch, SteamVRLogic::SharedInstance(), &SteamVRLogic::switchController);
    QObject::connect(pDashboardUI, &DashboardUI::requestMoveBegin, SteamVRLogic::SharedInstance(), &SteamVRLogic::startMove);
    QObject::connect(pDashboardUI, &DashboardUI::requestScaleUp, SteamVRLogic::SharedInstance(), &SteamVRLogic::increaseOverlayScale);
    QObject::connect(pDashboardUI, &DashboardUI::requestScaleDown, SteamVRLogic::SharedInstance(), &SteamVRLogic::decreaseOverlayScale);
    QObject::connect(SteamVRLogic::SharedInstance(), &SteamVRLogic::saveOpacity, pDashboardUI, &DashboardUI::saveOpacity);
    QObject::connect(SteamVRLogic::SharedInstance(), &SteamVRLogic::restoreOpacity, pDashboardUI, &DashboardUI::restoreOpacity);

    //QObject::connect(pDashboardUI, &DashboardUI::requestMoveEnd, SteamVRLogic::SharedInstance(), &SteamVRLogic::stopMove);

    //std::cout << "Started the thread (maybe)" << std::endl;

    frameThread->start();

    int exitCode = a.exec();

    frameThread->quit();
    frameThread->wait();

    delete frameThread;
    //delete frameHandler;
    delete pDashboardUI;

    SteamVRLogic::SharedInstance()->Shutdown();

    return exitCode;
}
