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
        float frameDeliverySmoothness = 0;
        float Framerate = 0;
        float MaxFrametime = 0;
        float MaxFramerate = 0;
        float smoothFrameRate = 0;
    };
    typedef QList<frameStats> FrameStatsList;

public slots:
    //void run();
    void startProcessing();
    void stopProcessing();
    void processFrame();

private:

    float smoothFrameRateArray[20] = {0};
    int smoothIndex = 0;
    int smoothCount = 0;

    vr::Compositor_FrameTiming m_currentFrame = {};
    vr::Compositor_FrameTiming m_previousFrame = {};
    frameStats m_information;

    QElapsedTimer m_uiUpdateTimerGraphs;
    QElapsedTimer m_uiUpdateTimerLabels;
    QTimer* m_pTimer = nullptr;
    const qint64 UI_UPDATE_INTERVAL_MS_GRAPHS = 500; // Interval for UI graphs updating. Unit is ms
    const qint64 UI_UPDATE_INTERVAL_MS_LABELS = 250; // Interval for UI labels updating. Unit is ms
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