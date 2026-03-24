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
#include <QtGraphs/QtGraphs>

class FrameHandler: public QObject {
    Q_OBJECT
public:
    FrameHandler(float newMaxFrameTime, float newMaxFrameRate, DashboardUI *dashboard, QObject *parent = nullptr);
    virtual ~FrameHandler();
public slots:
    void run();
    void calculateFrameData();

public:
    struct frameStats {
        float GpuFrametime;
        float CpuFrametime;
        float TotalFrametime;
        float Framerate;
        float MaxFrametime;
        float MaxFramerate;
    };
private:
    uint32_t m_lastFrameSampleIndex;
    float m_targetFrameRate;
    float m_targetRefreshRateMs;
    DashboardUI *m_dashboard;
};

#endif //PERFORMANCEVR_FRAMEHANDLER_H