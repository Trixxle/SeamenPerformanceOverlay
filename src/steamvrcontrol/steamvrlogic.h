//
// Created by jornt on 05/02/2026.
//

#ifndef PERFORMANCEVR_STEAMVRLOGIC_H
#define PERFORMANCEVR_STEAMVRLOGIC_H

#include <QtCore/QtCore>
// because of incompatibilities with QtOpenGL and GLEW we need to cherry pick includes
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
#include "openvr.h"

class SteamVRLogic: public QObject {
    Q_OBJECT
    typedef QObject BaseClass;

public:
    static SteamVRLogic *SharedInstance();

    SteamVRLogic();
    ~SteamVRLogic();

    bool Init();
    void Shutdown();
    bool BHMDAvailable();
    void SetWidget( QWidget *pWidget);
    float GetHeadsetRefreshRate();
    float GetHeadsetMaxFrameRate();
    //vr::IVRSystem *GetVRSystem();
    vr::HmdError GetLastHmdError();
    QString GetVRDriverString();
    QString GetVRDisplayString();
    QString GetName() { return m_strOverlayName; }

    QString GetTrackedDeviceString(vr::IVRSystem *pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop);

public slots:
    void OnSceneChanged(const QList<QRectF>&);
    void OnTimeoutPumpEvents();
    void switchController();
    void startMove();
    void stopMove();
    void increaseOverlayScale();
    void decreaseOverlayScale();

private:
    vr::TrackedDeviceIndex_t m_unLastInteractingDevice = vr::k_unTrackedDeviceIndexInvalid;
    vr::TrackedDeviceIndex_t m_deviceOverlayIsAttachedTo = vr::k_unTrackedDeviceIndexInvalid;
    vr::TrackedDeviceIndex_t m_leftController = vr::k_unTrackedDeviceIndexInvalid;
    vr::TrackedDeviceIndex_t m_rightController = vr::k_unTrackedDeviceIndexInvalid;
    vr::EVRInitError m_eLastHmdError;
    vr::EVRInitError m_eCompositorError;
    vr::EVRInitError m_eOverlayError;
    vr::VROverlayHandle_t m_ulOverlayHandle;
    vr::VROverlayHandle_t m_ulOverlayThumbnailHandle;
    vr::IVRSystem *m_pVRSystem;

    bool ConnectToVRRuntime();
    void DisconnectFromVRRuntime();
    void saveSession();
    void restoreSession();
    void AttachToDevice(const vr::TrackedDeviceIndex_t& device);
    vr::HmdMatrix34_t calculateRelativeTransform(vr::TrackedDeviceIndex_t device);
    void mirrorMatrix();
    void updateOverlayWidthInMeters();
    void saveSize();
    void savePosition();
    void saveController();

    bool m_isMoving = false;
    float m_overlayWidthInMeters;

    vr::HmdMatrix34_t m_overlayPositionMatrix;
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

    QPointF m_tLastMouse;
    Qt::MouseButtons m_lastMouseButtons;
    QSettings m_settings;

signals:
    void saveOpacity();
    void restoreOpacity();
};


#endif //PERFORMANCEVR_STEAMVRLOGIC_H