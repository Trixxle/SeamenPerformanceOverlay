//
// Created by jornt on 05/02/2026.
//

#ifndef PERFORMANCEVR_STEAMVRLOGIC_H
#define PERFORMANCEVR_STEAMVRLOGIC_H

#include <QOpenGLContext>
#include <QtCore>
#include <QOffscreenSurface>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFramebufferObjectFormat>
#include <QWidget>
#include <QGraphicsScene>
#include <QOpenGLFunctions>
#include <QtWidgets/QApplication>
#include <QPainter>
#include <QBrush>
#include <iostream>

#include "openvr.h"

class SteamVRLogic: public QObject {
    Q_OBJECT
    typedef QObject BaseClass;

public:
    // The instance must be shared with Qt, otherwise it gets killed after main.cpp is ran
    static SteamVRLogic *SharedInstance();

    SteamVRLogic();
    ~SteamVRLogic();

    bool Init();
    void Shutdown();
    void SetWidget( QWidget *pWidget);

    QString GetTrackedDeviceString(vr::IVRSystem *pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop);

private:

    vr::EVRInitError m_eLastHmdError;
    vr::VROverlayHandle_t m_ulOverlayHandle;
    vr::VROverlayHandle_t m_ulOverlayThumbnailHandle;

    bool ConnectToVRRuntime();
    void DisconnectFromVRRuntime();

    vr::TrackedDevicePose_t m_rTrackedDevicePose[ vr::k_unMaxTrackedDeviceCount ];

    // The widget created with Qt
    QWidget *m_Widget;

    QString m_strVRDriver;
    QString m_strVRDisplay;
    QString m_strOverlayName;

    QTimer *m_pPumpEventsTimer;

    QOpenGLContext *m_pOpenGLContext;
    QOffscreenSurface *m_pOffscreenSurface;
    QGraphicsScene *m_pScene;
    QOpenGLFramebufferObject *m_pFbo;
};


#endif //PERFORMANCEVR_STEAMVRLOGIC_H