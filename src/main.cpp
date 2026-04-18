/*
Copyright (C) 2026 Jorn ten Kate, The Seamen

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "userinterface/dashboardui.h"
#include "userinterface/panicdashboard.h"
#include "steamvrcontrol/steamvrlogic.h"
#include "systemResourcesHandler.h"
#include "frameHandler.h"
#include <iostream>
#include <QtWidgets/QApplication>
#include <QThread>
#include <QMetaType>

#include "systemResourcesHandler.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    qRegisterMetaType<FrameHandler::frameStats>("frameStats");
    qRegisterMetaType<FrameHandler::FrameStatsList>("FrameStatsList");

    qRegisterMetaType<SystemResourcesHandler::systemResources>("systemResources");
    qRegisterMetaType<SystemResourcesHandler::systemResourceUsage>("systemResourceUsage");

    if (!SteamVRLogic::SharedInstance()->Init()) {
        std::cerr << "Failed to initialize SteamVR Logic. Exiting." << std::endl;
        return -1;
    }

    DashboardUI *pDashboardUI = new DashboardUI(SteamVRLogic::SharedInstance()->GetHeadsetRefreshRate(),
        SteamVRLogic::SharedInstance()->GetHeadsetMaxFrameRate());

    panicDashboard *pPanicDashboard = new panicDashboard();

    QThread *frameThread = new QThread;
    QThread *systemResourcesThread = new QThread;

    SteamVRLogic::SharedInstance()->SetWidget(pDashboardUI);
    SteamVRLogic::SharedInstance()->SetPanicWidget(pPanicDashboard);

    FrameHandler *frameHandler = new FrameHandler(SteamVRLogic::SharedInstance()->GetHeadsetRefreshRate(),
        SteamVRLogic::SharedInstance()->GetHeadsetMaxFrameRate()
        );

    SystemResourcesHandler *systemResourcesHandler = new SystemResourcesHandler();

    frameHandler->moveToThread(frameThread);
    systemResourcesHandler->moveToThread(systemResourcesThread);

    QObject::connect(frameThread, &QThread::started, frameHandler, &FrameHandler::startProcessing);
    QObject::connect(frameThread, &QThread::finished, frameHandler, &QObject::deleteLater);
    QObject::connect(systemResourcesThread, &QThread::started, systemResourcesHandler, &SystemResourcesHandler::startSystemResourcesProcessing);
    QObject::connect(systemResourcesThread, &QThread::finished, systemResourcesHandler, &QObject::deleteLater);

    QObject::connect(frameHandler, &FrameHandler::updateGraphs, pDashboardUI, &DashboardUI::updateGraphs);
    QObject::connect(frameHandler, &FrameHandler::updateLabels, pDashboardUI, &DashboardUI::updateLabels);
    QObject::connect(systemResourcesHandler, &SystemResourcesHandler::updateSystemResources, pDashboardUI, &DashboardUI::updateSystemResources);
    QObject::connect(systemResourcesHandler, &SystemResourcesHandler::updateSystemResourceUsage, pDashboardUI, &DashboardUI::updateSystemResourceUsage);

    QObject::connect(pDashboardUI, &DashboardUI::requestControllerSwitch, SteamVRLogic::SharedInstance(), &SteamVRLogic::switchController);
    QObject::connect(pDashboardUI, &DashboardUI::requestMoveBegin, SteamVRLogic::SharedInstance(), &SteamVRLogic::startMove);
    QObject::connect(pDashboardUI, &DashboardUI::requestScaleUp, SteamVRLogic::SharedInstance(), &SteamVRLogic::increaseOverlayScale);
    QObject::connect(pDashboardUI, &DashboardUI::requestScaleDown, SteamVRLogic::SharedInstance(), &SteamVRLogic::decreaseOverlayScale);

    QObject::connect(SteamVRLogic::SharedInstance(), &SteamVRLogic::saveOpacity, pDashboardUI, &DashboardUI::saveOpacity);
    QObject::connect(SteamVRLogic::SharedInstance(), &SteamVRLogic::restoreOpacity, pDashboardUI, &DashboardUI::restoreOpacity);

    QObject::connect(pPanicDashboard, &panicDashboard::panicButtonClicked,pDashboardUI, &DashboardUI::resetOpacityToDefault);
    QObject::connect(pPanicDashboard, &panicDashboard::panicButtonClicked, SteamVRLogic::SharedInstance(), &SteamVRLogic::resetOverlayToDefault);

    frameThread->start();
    systemResourcesThread->start();

    int exitCode = a.exec();

    frameThread->quit();
    frameThread->wait();
    systemResourcesThread->quit();
    systemResourcesThread->wait();

    delete frameThread;
    delete systemResourcesThread;
    //delete frameHandler;
    delete pDashboardUI;

    SteamVRLogic::SharedInstance()->Shutdown();

    return exitCode;
}
