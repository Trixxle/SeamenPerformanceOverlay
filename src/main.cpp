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
#include <QTimer>
#include <QSurfaceFormat>
#include <memory>
#include "userinterface/dashboardui.h"
#include "userinterface/panicdashboard.h"
#include "steamvrcontrol/steamvrlogic.h"
#include "systemResourcesHandler.h"
#include "frameHandler.h"

int main(int argc, char *argv[])
{
    QSurfaceFormat format;
    format.setMajorVersion( 4 );
    format.setMinorVersion( 1 );
    format.setProfile( QSurfaceFormat::CoreProfile );
    QSurfaceFormat::setDefaultFormat( format );

    QApplication a(argc, argv);

    qRegisterMetaType<FrameHandler::frameStats>("frameStats");
    qRegisterMetaType<FrameHandler::FrameStatsList>("FrameStatsList");

    qRegisterMetaType<SystemResourcesHandler::systemResources>("systemResources");
    qRegisterMetaType<SystemResourcesHandler::systemResourceUsage>("systemResourceUsage");

    SteamVRLogic* vrLogic = SteamVRLogic::SharedInstance();


    if (!vrLogic->Init()) {
        std::cerr << "Failed to initialize SteamVR Logic. Exiting." << std::endl;
        return -1;
    }

    auto* pDashboardUI = new DashboardUI(vrLogic->GetHeadsetRefreshRate(),
        vrLogic->GetHeadsetMaxFrameRate());

    auto *pPanicDashboard = new panicDashboard();

    auto frameThread = std::make_unique<QThread>();
    auto systemResourcesThread = std::make_unique<QThread>();

    vrLogic->SetWidget(pDashboardUI);
    vrLogic->SetPanicWidget(pPanicDashboard);

    auto *frameHandler = new FrameHandler(vrLogic->GetHeadsetRefreshRate(), vrLogic->GetHeadsetMaxFrameRate());
    auto *systemResourcesHandler = new SystemResourcesHandler();

    frameHandler->moveToThread(frameThread.get());
    systemResourcesHandler->moveToThread(systemResourcesThread.get());

    // Connects multithreading frame statistics and system resource statistics
    QObject::connect(frameThread.get(), &QThread::started, frameHandler, &FrameHandler::startProcessing);
    QObject::connect(frameThread.get(), &QThread::finished, frameHandler, &QObject::deleteLater);
    QObject::connect(systemResourcesThread.get(), &QThread::started, systemResourcesHandler, &SystemResourcesHandler::startSystemResourcesProcessing);
    QObject::connect(systemResourcesThread.get(), &QThread::finished, systemResourcesHandler, &QObject::deleteLater);

    // Connects for backend to frontend communication (backend to overlay)
    QObject::connect(frameHandler, &FrameHandler::updateGraphs, pDashboardUI, &DashboardUI::updateGraphs);
    QObject::connect(frameHandler, &FrameHandler::updateLabels, pDashboardUI, &DashboardUI::updateLabels);
    QObject::connect(systemResourcesHandler, &SystemResourcesHandler::updateSystemResources, pDashboardUI, &DashboardUI::updateSystemResources);
    QObject::connect(systemResourcesHandler, &SystemResourcesHandler::updateSystemResourceUsage, pDashboardUI, &DashboardUI::updateSystemResourceUsage);
    QObject::connect(vrLogic, &SteamVRLogic::hideUi, pDashboardUI, &DashboardUI::hideUi);
    QObject::connect(vrLogic, &SteamVRLogic::distanceFadeValueChanged, pDashboardUI, &DashboardUI::setDistanceFadeValue);
    QObject::connect(vrLogic, &SteamVRLogic::saveOpacity, pDashboardUI, &DashboardUI::saveOpacity);
    QObject::connect(vrLogic, &SteamVRLogic::restoreOpacity, pDashboardUI, &DashboardUI::restoreOpacity);
    QObject::connect(vrLogic, &SteamVRLogic::distanceFadeCheckChanged, pDashboardUI, &DashboardUI::setDistanceFadeState);
    QObject::connect(vrLogic, &SteamVRLogic::appLaunched, pDashboardUI, &DashboardUI::setAppLaunch);
    QObject::connect(vrLogic, &SteamVRLogic::appQuit, pDashboardUI, &DashboardUI::setAppQuit);
    QObject::connect(vrLogic, &SteamVRLogic::leftControllerBattery, pDashboardUI, &DashboardUI::setLeftControllerBatteryLevel);
    QObject::connect(vrLogic, &SteamVRLogic::rightControllerBattery, pDashboardUI, &DashboardUI::setRightControllerBatteryLevel);
    QObject::connect(vrLogic, &SteamVRLogic::headsetBattery, pDashboardUI, &DashboardUI::setHeadseyBatteryLevel);

    // Connects for backend to frontend communication (backend to dashboard)
    QObject::connect(vrLogic, &SteamVRLogic::overlayScaleChanged, pPanicDashboard, &panicDashboard::setScaleValue);
    QObject::connect(vrLogic, &SteamVRLogic::distanceFadeValueChanged, pPanicDashboard, &panicDashboard::setDistanceFadeValue);
    QObject::connect(vrLogic, &SteamVRLogic::restoreDistanceFade, pPanicDashboard, &panicDashboard::setDistanceFadeState);
    QObject::connect(vrLogic, &SteamVRLogic::saveDistanceFade, pPanicDashboard, &panicDashboard::saveDistanceFadeState);
    QObject::connect(vrLogic, &SteamVRLogic::distanceFadeCheckChanged, pPanicDashboard, &panicDashboard::setDistanceFadeState);

    // Connects for front end to backend communication (overlay to backend)
    QObject::connect(pDashboardUI, &DashboardUI::requestControllerSwitch, vrLogic, &SteamVRLogic::switchController);
    QObject::connect(pDashboardUI, &DashboardUI::requestMoveBegin, vrLogic, &SteamVRLogic::startMove);
    QObject::connect(pDashboardUI, &DashboardUI::requestScaleBegin, vrLogic, &SteamVRLogic::startScale);
    QObject::connect(pDashboardUI, &DashboardUI::requestDistanceFadeStartUp, vrLogic, &SteamVRLogic::increaseFadeDistanceStart);
    QObject::connect(pDashboardUI, &DashboardUI::requestDistanceFadeStartDown, vrLogic, &SteamVRLogic::decreaseFadeDistanceStart);
    QObject::connect(pDashboardUI, &DashboardUI::distanceCheckboxToggled, vrLogic, &SteamVRLogic::setDistanceFade);

    // Connects for front end to backend communication (dashboard to backend)
    QObject::connect(pPanicDashboard, &panicDashboard::requestScaleUp, vrLogic, &SteamVRLogic::increaseOverlayScale);
    QObject::connect(pPanicDashboard, &panicDashboard::requestScaleDown, vrLogic, &SteamVRLogic::decreaseOverlayScale);
    QObject::connect(pPanicDashboard, &panicDashboard::requestOpacityUp, pDashboardUI, &DashboardUI::increaseOpacityButtonClicked);
    QObject::connect(pPanicDashboard, &panicDashboard::requestOpacityDown, pDashboardUI, &DashboardUI::decreaseOpacityButtonClicked);
    QObject::connect(pPanicDashboard, &panicDashboard::requestDistanceFadeStartUp, vrLogic, &SteamVRLogic::increaseFadeDistanceStart);
    QObject::connect(pPanicDashboard, &panicDashboard::requestDistanceFadeStartDown, vrLogic, &SteamVRLogic::decreaseFadeDistanceStart);
    QObject::connect(pPanicDashboard, &panicDashboard::panicButtonClicked,pDashboardUI, &DashboardUI::resetOpacityToDefault);
    QObject::connect(pPanicDashboard, &panicDashboard::panicButtonClicked, vrLogic, &SteamVRLogic::resetOverlayToDefault);
    QObject::connect(pPanicDashboard, &panicDashboard::distanceCheckboxToggled, vrLogic, &SteamVRLogic::setDistanceFade);
    QObject::connect(pPanicDashboard, &panicDashboard::requestRightControllerAttach, vrLogic, &SteamVRLogic::attachToRightController);
    QObject::connect(pPanicDashboard, &panicDashboard::requestLeftControllerAttach, vrLogic, &SteamVRLogic::attachToLeftController);
    QObject::connect(pPanicDashboard, &panicDashboard::requestHmdAttach, vrLogic, &SteamVRLogic::attachToHmd);

    // Connects for the overlay to dashboard communication and vice versa
    QObject::connect(pDashboardUI, &DashboardUI::opacityChanged, pPanicDashboard, &panicDashboard::setOpacityValue);

    frameThread->start();
    systemResourcesThread->start();

    QTimer::singleShot(0, vrLogic, []() {
            SteamVRLogic::SharedInstance()->steamDashboardStateForUi();
    });
    QTimer::singleShot(0, vrLogic, []() {
        SteamVRLogic::SharedInstance()->setCurrentGame();
    });

    QTimer::singleShot(0, vrLogic, []() {
    SteamVRLogic::SharedInstance()->setControllersBatteryLevel();
    });
    QTimer::singleShot(0, vrLogic, []() {
    SteamVRLogic::SharedInstance()->setHeadsetBatteryLevel();
    });

    int exitCode = a.exec();

    frameThread->quit();
    frameThread->wait();
    systemResourcesThread->quit();
    systemResourcesThread->wait();

    vrLogic->Shutdown();

    SteamVRLogic::DestroyInstance();

    return exitCode;
}
