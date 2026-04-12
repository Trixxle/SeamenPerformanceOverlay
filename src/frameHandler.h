//
// Created by jornt on 10/03/2026.
//

#ifndef PERFORMANCEVR_FRAMEHANDLER_H
#define PERFORMANCEVR_FRAMEHANDLER_H

#include <QObject>
#include <QWidget>
#include <openvr.h>
#include <iostream>
#include <QThread>
#include <QtCharts/QtCharts>
#include <math.h>
#include <QTimer>
#include <QElapsedTimer>
#include <QList>
#include <QMetaType>

class FrameHandler: public QObject {
    Q_OBJECT
public:
    FrameHandler(float newMaxFrameTime, float newMaxFrameRate, QObject *parent = nullptr);
    virtual ~FrameHandler();

    struct frameStats {
        float GpuFrametime = 0;
        float CpuFrametime = 0;
        float TotalFrametime = 0;
        float Framerate = 0;
        float MaxFrametime = 0;
        float MaxFramerate = 0;
        float smoothFrameRate = 0;
    };
    typedef QList<frameStats> FrameStatsList;

public slots:
    //void run();
    void calculateFrameData(vr::Compositor_FrameTiming &currentFrame, vr::Compositor_FrameTiming &previousFrame, frameStats &information);
    void startProcessing();
    void stopProcessing();
    void processFrame();

private:

    float smoothFrameRateArray[20] = {0};
    int smoothIndex = 0;
    int smoothCount = 0;

    QElapsedTimer m_uiUpdateTimerGraphs;
    QElapsedTimer m_uiUpdateTimerLabels;
    QTimer* m_timer = nullptr;
    const qint64 UI_UPDATE_INTERVAL_MS_GRAPHS = 250; // Interval for UI graphs updating. Unit is ms
    const qint64 UI_UPDATE_INTERVAL_MS_LABELS = 100; // Interval for UI labels updating. Unit is ms
    QList<frameStats> m_frameBuffer;
    uint32_t m_lastFrameSampleIndex;
    uint32_t m_renderedFrames;
    float m_targetFrameRate;
    float m_targetRefreshRateMs;

    signals:
    void updateGraphs(const FrameStatsList &informationList);
    void updateLabels(const frameStats &information);

};

Q_DECLARE_METATYPE(FrameHandler::FrameStatsList)

#endif //PERFORMANCEVR_FRAMEHANDLER_H