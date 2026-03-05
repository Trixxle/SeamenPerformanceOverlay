//
// Created by jornt on 05/02/2026.
//

#ifndef PERFORMANCEVR_STEAMVRLOGIC_H
#define PERFORMANCEVR_STEAMVRLOGIC_H
/*
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
#include <iostream>*/

/*#include <QtCore/QtCore>
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
#include <QOpenGLPaintDevice>
#include <QPainter>
#include <iostream>
#include <QWidget>
#include <QApplication>
#include <QMouseEvent>
#include <QtWidgets/QGraphicsSceneMouseEvent>*/

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
    bool BHMDAvailable();
    void SetWidget( QWidget *pWidget);
    vr::IVRSystem *GetVRSystem();
    vr::HmdError GetLastHmdError();
    QString GetVRDriverString();
    QString GetVRDisplayString();
    QString GetName() { return m_strOverlayName; }

    QString GetTrackedDeviceString(vr::IVRSystem *pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop);

public slots:
    void OnSceneChanged(const QList<QRectF>&);
    void OnTimeoutPumpEvents();

private:

    vr::EVRInitError m_eLastHmdError;
    vr::EVRInitError m_eCompositorError;
    vr::EVRInitError m_eOverlayError;
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

    QPointF m_tLastMouse;
    Qt::MouseButtons m_lastMouseButtons;
};


#endif //PERFORMANCEVR_STEAMVRLOGIC_H