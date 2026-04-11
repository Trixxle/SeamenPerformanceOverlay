//
// Created by jornt on 10/03/2026.
//

#ifndef PERFORMANCEVR_FRAMEHANDLER_H
#define PERFORMANCEVR_FRAMEHANDLER_H

#include <QObject>
#include <QWidget>
#include <openvr.h>
#include "userinterface/dashboardui.h"
#include <iostream>
#include <QThread>
#include <QtCharts/QtCharts>
#include <math.h>

class FrameHandler: public QObject {
    Q_OBJECT
public:
    FrameHandler(float newMaxFrameTime, float newMaxFrameRate, DashboardUI *dashboard, QObject *parent = nullptr);
    virtual ~FrameHandler();

public:
    struct frameStats {
        float GpuFrametime;
        float CpuFrametime;
        float TotalFrametime;
        float Framerate;
        float MaxFrametime;
        float MaxFramerate;
    };

public slots:
    void run();
    void calculateFrameData(vr::Compositor_FrameTiming &currentFrame, vr::Compositor_FrameTiming &previousFrame, frameStats &information);

private:
    float roundFloat(float number);

    uint32_t m_lastFrameSampleIndex;
    uint32_t m_renderedFrames;
    float m_targetFrameRate;
    float m_targetRefreshRateMs;
    DashboardUI *m_dashboard;
};

#endif //PERFORMANCEVR_FRAMEHANDLER_H