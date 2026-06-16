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

#ifndef PERFORMANCEVR_STEAMVRLOGIC_H
#define PERFORMANCEVR_STEAMVRLOGIC_H

#include <QtCore/QtCore>
#include <QThread>
#include <QtGui/QVector2D>
#include <QtGui/QMatrix4x4>
#include <QtCore/QVector>
#include <QtGui/QVector2D>
#include <QtGui/QVector3D>
#include <QtGui/QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QtWidgets/QGraphicsScene>
#include <QtGui/QOffscreenSurface>
#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLPaintDevice>
#include <QPainter>
#include <QtWidgets/QWidget>
#include <QMouseEvent>
#include <QtWidgets/QGraphicsSceneMouseEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsEllipseItem>
#include <QCursor>
#include <iostream>
#include <filesystem>
#include <QGraphicsProxyWidget>
#include <QOpenGLFunctions>
#include <QOpenGLExtraFunctions>
#include <QDir>
#include <QFile>
#include <QMap>
#include <iostream>
#include <qdialogbuttonbox.h>
#include "userSettings.h"
#include "openvr.h"

class SteamVRLogic: public QObject {
    Q_OBJECT
    typedef QObject BaseClass;

public:
    enum initializationError {
        eNone = 0,
        eSteamVrNotInstalled = 1,
        eOpenGLFailedToInitialize = 2,
        eFailedToConnectToSteamVr = 3,
        eFailedToCreateOverlays = 4,
        eFailedToInitialize = 5
    };

    static SteamVRLogic *SharedInstance();
    static void DestroyInstance();

    SteamVRLogic();
    ~SteamVRLogic();

    initializationError Init();
    void Shutdown();
    bool BHMDAvailable();
    void SetWidget( QWidget *pWidget);
    void SetPanicWidget( QWidget *pWidget);
    float GetHeadsetRefreshRate();
    float GetHeadsetMaxFrameRate();
    vr::HmdError GetLastHmdError();
    QString GetVRDriverString();
    QString GetVRDisplayString();
    QString GetName() { return m_strOverlayName; }

    QString GetTrackedDeviceString(vr::IVRSystem *pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop);

public slots:
    void OnSceneChanged(const QList<QRectF>&);
    void OnPanicSceneChanged(const QList<QRectF>&);
    void OnTimeoutPumpEvents();
    void switchController();
    void startMove();
    void stopMove();
    void startScale();
    void stopScale();
    void setDistanceFade(bool enabled);
    void steamDashboardStateForUi();
    void attachToRightController();
    void attachToLeftController();
    void attachToHmd();
    void setCurrentGame();
    void setControllersBatteryLevel();
    void setHeadsetBatteryLevel();
    void setTrackersBattery();
    void searchForTrackers();
    void resetPosition();
    void updateOverlayWidthInMeters();

private:
    struct AppCacheData {
        QString appName;
        QByteArray appKey;
    };

    std::vector<vr::TrackedDeviceIndex_t> m_trackers;
    // Vive and Tundra trackers may connect without yet being available. When this happens they do in this vector until
    // they are available. Then they are passed to m_trackers.
    std::vector<vr::TrackedDeviceIndex_t> m_pendingTrackers;


    int MAX_VRRUNTIME_CONNECTION_ATTEMPTS = 20;
    vr::TrackedDeviceIndex_t m_unLastInteractingDevice = vr::k_unTrackedDeviceIndexInvalid;
    vr::TrackedDeviceIndex_t m_leftController = vr::k_unTrackedDeviceIndexInvalid;
    vr::TrackedDeviceIndex_t m_rightController = vr::k_unTrackedDeviceIndexInvalid;
    vr::VRInputValueHandle_t m_leftHandHandle = vr::k_ulInvalidInputValueHandle;
    vr::VRInputValueHandle_t m_rightHandHandle = vr::k_ulInvalidInputValueHandle;

    vr::TrackedDeviceIndex_t m_deviceOverlayIsAttachedTo = vr::k_unTrackedDeviceIndexInvalid;

    vr::ETrackedControllerRole m_matrixForRole = vr::TrackedControllerRole_LeftHand;
    vr::EVRInitError m_eLastHmdError;
    vr::EVRInitError m_eCompositorError;
    vr::EVRInitError m_eOverlayError;
    vr::EVRInitError m_ePanicOverlayError;
    vr::VROverlayHandle_t m_ulOverlayHandle;
    vr::VROverlayHandle_t m_ulPanicOverlayHandle;
    vr::VROverlayHandle_t m_ulOverlayThumbnailHandle;
    vr::IVRSystem *m_pVRSystem;

    bool ConnectToVRRuntime();
    void DisconnectFromVRRuntime();
    void saveSession();
    void restoreSession();
    void AttachToDevice(const vr::TrackedDeviceIndex_t& device);
    vr::TrackedDeviceIndex_t getControllerForRole(vr::ETrackedControllerRole role);
    vr::ETrackedControllerRole getRoleForController(vr::TrackedDeviceIndex_t device);
    vr::HmdMatrix34_t calculateRelativeTransform(vr::TrackedDeviceIndex_t device);
    void mirrorMatrix();
    void saveSize();
    void savePosition();
    void saveController();
    void saveDistanceFadeStart();
    void RenderDirtyOverlayScenes();
    void checkClosestControllerForRole();
    void attemptControllerBind();
    float calculateOverlayDistance();
    void switchToSpecificController(vr::TrackedDeviceIndex_t targetDevice);
    std::vector<vr::TrackedDeviceIndex_t> getDevicesForClass(vr::ETrackedDeviceClass classToLookFor);
    void addTracker(vr::TrackedDeviceIndex_t trackerToAdd);
    void removeTracker(vr::TrackedDeviceIndex_t);
    float getDeviceBatteryLevel(vr::TrackedDeviceIndex_t device);
    void checkForCloserController();
    void handlePendingTrackers();

    bool m_isMoving = false;
    bool m_isScaling = false;
    float m_scaleButtonPressX = -1.0f;  // -1 = not yet anchored; captured on first MouseMove after press
    float m_baseAlpha = 1.0f;
    float m_lastAlpha = -1.0f;  // Cached alpha to avoid redundant SetOverlayAlpha calls
    bool m_mainSceneDirty = false;   // Dirty flags: set on scene change, cleared after FBO render
    bool m_panicSceneDirty = false;

    QRect m_mainSceneDirtyRect;
    QRect m_panicSceneDirtyRect;

    vr::TrackedDevicePose_t m_rTrackedDevicePose[ vr::k_unMaxTrackedDeviceCount ];

    // The widget created with Qt
    QWidget *m_pWidget;

    // Dashboard overlay in with a panic button
    QWidget *m_pPanicWidget;

    QString m_strVRDriver;
    QString m_strVRDisplay;
    QString m_strOverlayName;
    uint32_t m_currentGamePid = 0;
    QMap<uint32_t, AppCacheData> m_activeProcesses;

    QTimer *m_pPumpEventsTimer;
    QTimer *m_pRenderTimer;

    std::unique_ptr<QOpenGLContext> m_pOpenGLContext;
    std::unique_ptr<QOffscreenSurface> m_pOffscreenSurface;
    std::unique_ptr<QGraphicsScene> m_pScene;
    std::unique_ptr<QGraphicsScene> m_pPanicScene;
    std::unique_ptr<QOpenGLFramebufferObject> m_pFbo;
    std::unique_ptr<QOpenGLFramebufferObject> m_pPanicFbo;
    std::unique_ptr<QOpenGLPaintDevice> m_pMainPaintDevice;
    std::unique_ptr<QOpenGLPaintDevice> m_pPanicPaintDevice;

    QPointF m_tLastMouse;
    Qt::MouseButtons m_lastMouseButtons;

    int m_bindToControllerAttempts = 0;
    int m_proximityCheckCounter = 0;
    int m_poseCheckCounter = 0;
    int m_batteryCheckCounter = 0;

signals:
    void hideUi(bool hide);
    void appLaunched(const QString& appName);
    void appQuit(const QString& appName);
    void leftControllerBattery(float level, bool charging);
    void rightControllerBattery(float level, bool charging);
    void headsetBattery(float level, bool charging);
    void addTrackerToUi(uint32_t index);
    void trackersBattery(float level, uint32_t index, bool charging);
    void removeTrackerFromUi(uint32_t index);
};


#endif //PERFORMANCEVR_STEAMVRLOGIC_H