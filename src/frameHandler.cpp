//
// Created by jornt on 10/03/2026.
//

#include "frameHandler.h"

FrameHandler::FrameHandler(float newRefreshRate, float newMaxFrameRate, QObject *parent):
    QObject(parent),
    m_lastFrameSampleIndex(0.0),
    m_targetFrameRate(newMaxFrameRate),
    m_targetRefreshRateMs(newRefreshRate)
{
}

FrameHandler::~FrameHandler() {
}

void FrameHandler::startProcessing() {
    if (!m_timer) {
        m_timer = new QTimer(this);

        // Use PreciseTimer to prevent standard OS scheduling drift
        m_timer->setTimerType(Qt::PreciseTimer);

        // QTimer takes integer milliseconds. 11.11ms becomes 11ms. Important note: It doesn't round the number, it just truncates it. So 6.9 becomes 6
        int intervalMs = static_cast<int>(m_targetRefreshRateMs);
        //if (intervalMs <= 0) intervalMs = 5; // Fallback

        connect(m_timer, &QTimer::timeout, this, &FrameHandler::processFrame);

        m_uiUpdateTimerGraphs.start();
        m_uiUpdateTimerLabels.start();
        m_timer->start(intervalMs);

        std::cout << "Frame timer started on thread: " << QThread::currentThreadId() << std::endl;
    }
}

void FrameHandler::stopProcessing() {
    if (m_timer) {
        m_timer->stop();
        m_timer->deleteLater();
        m_timer = nullptr;
    }
}

void FrameHandler::processFrame() {
    m_currentFrame.m_nSize = sizeof(vr::Compositor_FrameTiming);

    // Skip in-flight or fully dropped frames where Submit was never called
    //if (currentFrame.m_flNewFrameReadyMs == 0.0f) return;

    m_previousFrame.m_nSize = sizeof(vr::Compositor_FrameTiming);

    // Fetch the most recent frame just to get the latest index
    bool validCurrent = vr::VRCompositor()->GetFrameTiming(&m_currentFrame, 0);
    bool validPrev = vr::VRCompositor()->GetFrameTiming(&m_previousFrame, 1);

    // Break early if we've exhausted the compositor's frame history
    if (!validCurrent || !validPrev) return;

    // Check if the frame was already processed last function call
    uint32_t currentFrameIndex = m_currentFrame.m_nFrameIndex;
    if (currentFrameIndex == m_lastFrameSampleIndex) {
        return;
    }

    double gpuFrametimeMs = 0;
    double cpuFrametimeMs = 0;
    double frameDeliverySmoothnessMS = 0;

    // Gpu frame time is given (Thanks Valve)
    gpuFrametimeMs = m_currentFrame.m_flTotalRenderGpuMs;

    // CPU frame time calculation - (OLD, ONLY HERE FOR REFERENCE)
    /*
    cpuFrametimeMs = (m_currentFrame.m_flNewFrameReadyMs -
                        m_currentFrame.m_flNewPosesReadyMs +
                        m_currentFrame.m_flCompositorRenderCpuMs);
    */

    // Delta time between system timestamps.
    // Calculates the absolute time interval between the current frame and the previous one
    frameDeliverySmoothnessMS = (m_currentFrame.m_flSystemTimeInSeconds -
                        m_previousFrame.m_flSystemTimeInSeconds) * 1000.0;

    // if m_flClientFrameIntervalMs is 0 it means there is no active game being rendered and m_flNewFrameReadyMs would have garbage results
    // Negative results mean a frame drop
    if (m_currentFrame.m_flClientFrameIntervalMs == 0.0) cpuFrametimeMs = m_currentFrame.m_flCompositorRenderCpuMs;
    else cpuFrametimeMs = m_currentFrame.m_flNewFrameReadyMs - m_currentFrame.m_flNewPosesReadyMs + m_currentFrame.m_flCompositorRenderCpuMs;

    m_information.GpuFrametime = static_cast<float>(gpuFrametimeMs);
    m_information.CpuFrametime = static_cast<float>(cpuFrametimeMs);
    m_information.frameDeliverySmoothness = static_cast<float>(frameDeliverySmoothnessMS);

    // Use the absolute time between the current and previous frame to calculate frame rate
    float calculatedFps = (frameDeliverySmoothnessMS > 0) ? (1.0f / static_cast<float>(frameDeliverySmoothnessMS) * 1000.0f) : 0;

    m_information.Framerate = std::max(0.0f, std::min(m_targetFrameRate, calculatedFps));
    m_information.MaxFrametime = m_targetRefreshRateMs;
    m_information.MaxFramerate = m_targetFrameRate;

    smoothFrameRateArray[smoothIndex] = m_information.Framerate;
    smoothIndex = (smoothIndex + 1) % 20;
    if (smoothCount < 20) smoothCount++;

    float averagedFrames = 0;
    for (int i = 0; i < smoothCount; i++) {
        averagedFrames += smoothFrameRateArray[i];
    }
    m_information.smoothFrameRate = averagedFrames / smoothCount;

    m_frameBuffer.append(m_information);

    if (m_uiUpdateTimerLabels.hasExpired(UI_UPDATE_INTERVAL_MS_LABELS))
        if (!m_frameBuffer.isEmpty()) {
            emit updateLabels(m_frameBuffer.last());
            m_uiUpdateTimerLabels.restart(); // Reset the timer for the next batch
        }

    if (m_uiUpdateTimerGraphs.hasExpired(UI_UPDATE_INTERVAL_MS_GRAPHS)) {
        if (!m_frameBuffer.isEmpty()) {
            emit updateGraphs(m_frameBuffer);
            m_frameBuffer.clear();
        }
        m_uiUpdateTimerGraphs.restart(); // Reset the timer for the next batch
    }

    m_lastFrameSampleIndex = currentFrameIndex;
}
