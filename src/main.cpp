/*
Original work Copyright (c) 2015, Valve Corporation. All rights reserved.
Modified work Copyright (C) 2026 Jorn ten Kate, The Seamen.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

-------------------------------------------------------------------------------

This program is also licensed under the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version.

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

#include <iostream>
#include <QtWidgets/QApplication>
#include <QThread>
#include <QMetaType>
#include "userinterface/dashboardui.h"
#include "userinterface/panicdashboard.h"
#include "steamvrcontrol/steamvrlogic.h"
#include "systemResourcesHandler.h"
#include "frameHandler.h"
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

    // Connects multithreading frame statistics and system resource statistics
    QObject::connect(frameThread, &QThread::started, frameHandler, &FrameHandler::startProcessing);
    QObject::connect(frameThread, &QThread::finished, frameHandler, &QObject::deleteLater);
    QObject::connect(systemResourcesThread, &QThread::started, systemResourcesHandler, &SystemResourcesHandler::startSystemResourcesProcessing);
    QObject::connect(systemResourcesThread, &QThread::finished, systemResourcesHandler, &QObject::deleteLater);

    // Connects for backend to frontend communication (backend to overlay)
    QObject::connect(frameHandler, &FrameHandler::updateGraphs, pDashboardUI, &DashboardUI::updateGraphs);
    QObject::connect(frameHandler, &FrameHandler::updateLabels, pDashboardUI, &DashboardUI::updateLabels);
    QObject::connect(systemResourcesHandler, &SystemResourcesHandler::updateSystemResources, pDashboardUI, &DashboardUI::updateSystemResources);
    QObject::connect(systemResourcesHandler, &SystemResourcesHandler::updateSystemResourceUsage, pDashboardUI, &DashboardUI::updateSystemResourceUsage);
    QObject::connect(SteamVRLogic::SharedInstance(), &SteamVRLogic::hideUi, pDashboardUI, &DashboardUI::hideUi);
    QObject::connect(SteamVRLogic::SharedInstance(), &SteamVRLogic::distanceFadeValueChanged, pDashboardUI, &DashboardUI::setDistanceFadeValue);
    QObject::connect(SteamVRLogic::SharedInstance(), &SteamVRLogic::saveOpacity, pDashboardUI, &DashboardUI::saveOpacity);
    QObject::connect(SteamVRLogic::SharedInstance(), &SteamVRLogic::restoreOpacity, pDashboardUI, &DashboardUI::restoreOpacity);
    QObject::connect(SteamVRLogic::SharedInstance(), &SteamVRLogic::distanceFadeCheckChanged, pDashboardUI, &DashboardUI::setDistanceFadeState);
    QObject::connect(SteamVRLogic::SharedInstance(), &SteamVRLogic::appLaunched, pDashboardUI, &DashboardUI::setAppLaunch);
    QObject::connect(SteamVRLogic::SharedInstance(), &SteamVRLogic::appQuit, pDashboardUI, &DashboardUI::setAppQuit);

    // Connects for backend to frontend communication (backend to dashboard)
    QObject::connect(SteamVRLogic::SharedInstance(), &SteamVRLogic::overlayScaleChanged, pPanicDashboard, &panicDashboard::setScaleValue);
    QObject::connect(SteamVRLogic::SharedInstance(), &SteamVRLogic::distanceFadeValueChanged, pPanicDashboard, &panicDashboard::setDistanceFadeValue);
    QObject::connect(SteamVRLogic::SharedInstance(), &SteamVRLogic::restoreDistanceFade, pPanicDashboard, &panicDashboard::setDistanceFadeState);
    QObject::connect(SteamVRLogic::SharedInstance(), &SteamVRLogic::saveDistanceFade, pPanicDashboard, &panicDashboard::saveDistanceFadeState);
    QObject::connect(SteamVRLogic::SharedInstance(), &SteamVRLogic::distanceFadeCheckChanged, pPanicDashboard, &panicDashboard::setDistanceFadeState);

    // Connects for front end to backend communication (overlay to backend)
    QObject::connect(pDashboardUI, &DashboardUI::requestControllerSwitch, SteamVRLogic::SharedInstance(), &SteamVRLogic::switchController);
    QObject::connect(pDashboardUI, &DashboardUI::requestMoveBegin, SteamVRLogic::SharedInstance(), &SteamVRLogic::startMove);
    QObject::connect(pDashboardUI, &DashboardUI::requestScaleUp, SteamVRLogic::SharedInstance(), &SteamVRLogic::increaseOverlayScale);
    QObject::connect(pDashboardUI, &DashboardUI::requestScaleDown, SteamVRLogic::SharedInstance(), &SteamVRLogic::decreaseOverlayScale);
    QObject::connect(pDashboardUI, &DashboardUI::requestDistanceFadeStartUp, SteamVRLogic::SharedInstance(), &SteamVRLogic::increaseFadeDistanceStart);
    QObject::connect(pDashboardUI, &DashboardUI::requestDistanceFadeStartDown, SteamVRLogic::SharedInstance(), &SteamVRLogic::decreaseFadeDistanceStart);
    QObject::connect(pDashboardUI, &DashboardUI::distanceCheckboxToggled, SteamVRLogic::SharedInstance(), &SteamVRLogic::setDistanceFade);

    // Connects for front end to backend communication (dashboard to backend)
    QObject::connect(pPanicDashboard, &panicDashboard::requestScaleUp, SteamVRLogic::SharedInstance(), &SteamVRLogic::increaseOverlayScale);
    QObject::connect(pPanicDashboard, &panicDashboard::requestScaleDown, SteamVRLogic::SharedInstance(), &SteamVRLogic::decreaseOverlayScale);
    QObject::connect(pPanicDashboard, &panicDashboard::requestOpacityUp, pDashboardUI, &DashboardUI::increaseOpacityButtonClicked);
    QObject::connect(pPanicDashboard, &panicDashboard::requestOpacityDown, pDashboardUI, &DashboardUI::decreaseOpacityButtonClicked);
    QObject::connect(pPanicDashboard, &panicDashboard::requestDistanceFadeStartUp, SteamVRLogic::SharedInstance(), &SteamVRLogic::increaseFadeDistanceStart);
    QObject::connect(pPanicDashboard, &panicDashboard::requestDistanceFadeStartDown, SteamVRLogic::SharedInstance(), &SteamVRLogic::decreaseFadeDistanceStart);
    QObject::connect(pPanicDashboard, &panicDashboard::panicButtonClicked,pDashboardUI, &DashboardUI::resetOpacityToDefault);
    QObject::connect(pPanicDashboard, &panicDashboard::panicButtonClicked, SteamVRLogic::SharedInstance(), &SteamVRLogic::resetOverlayToDefault);
    QObject::connect(pPanicDashboard, &panicDashboard::distanceCheckboxToggled, SteamVRLogic::SharedInstance(), &SteamVRLogic::setDistanceFade);

    // Connects for the overlay to dashboard communication and vice versa
    QObject::connect(pDashboardUI, &DashboardUI::opacityChanged, pPanicDashboard, &panicDashboard::setOpacityValue);

    frameThread->start();
    systemResourcesThread->start();

    int exitCode = a.exec();

    frameThread->quit();
    frameThread->wait();
    systemResourcesThread->quit();
    systemResourcesThread->wait();

    delete frameThread;
    delete systemResourcesThread;
    delete pDashboardUI;

    SteamVRLogic::SharedInstance()->Shutdown();

    return exitCode;
}
