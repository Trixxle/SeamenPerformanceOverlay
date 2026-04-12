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
        if (intervalMs <= 0) intervalMs = 5; // Fallback

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
    vr::Compositor_FrameTiming currentFrame = {};
    vr::Compositor_FrameTiming previousFrame = {};
    frameStats information;

    calculateFrameData(currentFrame, previousFrame, information);
}

void FrameHandler::calculateFrameData(vr::Compositor_FrameTiming &currentFrame, vr::Compositor_FrameTiming &previousFrame, frameStats &information) {

    currentFrame.m_nSize = sizeof(vr::Compositor_FrameTiming);

    // Skip in-flight or fully dropped frames where Submit was never called
    //if (currentFrame.m_flNewFrameReadyMs == 0.0f) return;

    previousFrame.m_nSize = sizeof(vr::Compositor_FrameTiming);

    // Fetch the most recent frame just to get the latest index
    bool validCurrent = vr::VRCompositor()->GetFrameTiming(&currentFrame, 0);
    bool validPrev = vr::VRCompositor()->GetFrameTiming(&previousFrame, 1);

    // Break early if we've exhausted the compositor's frame history
    if (!validCurrent || !validPrev) return;

    // Check if the frame was already processed last function call
    uint32_t currentFrameIndex = currentFrame.m_nFrameIndex;
    if (currentFrameIndex == m_lastFrameSampleIndex) {
        return;
    }

    double gpuFrametimeMs = 0;
    double cpuFrametimeMs = 0;
    double totalFrametimeMs = 0;

    gpuFrametimeMs = currentFrame.m_flTotalRenderGpuMs;

    // CPU Timing calculation
    cpuFrametimeMs = (currentFrame.m_flNewFrameReadyMs -
                        currentFrame.m_flNewPosesReadyMs +
                        currentFrame.m_flCompositorRenderCpuMs);

    // Delta time between system timestamps
    totalFrametimeMs = (currentFrame.m_flSystemTimeInSeconds -
                        previousFrame.m_flSystemTimeInSeconds) * 1000.0;

    information.GpuFrametime = static_cast<float>(gpuFrametimeMs);
    information.CpuFrametime = static_cast<float>(cpuFrametimeMs);
    information.TotalFrametime = static_cast<float>(totalFrametimeMs);

    // Calculate Framerate: Limit between 0 and targetRefreshRate
    float calculatedFps = (totalFrametimeMs > 0) ? (1.0f / static_cast<float>(totalFrametimeMs) * 1000.0f) : 0;

    information.Framerate = std::max(0.0f, std::min(m_targetFrameRate, calculatedFps));
    information.MaxFrametime = m_targetRefreshRateMs;
    information.MaxFramerate = m_targetFrameRate;

    smoothFrameRateArray[smoothIndex] = information.Framerate;
    smoothIndex = (smoothIndex + 1) % 20;
    if (smoothCount < 20) smoothCount++;

    float averagedFrames = 0;
    for (int i = 0; i < smoothCount; i++) {
        averagedFrames += smoothFrameRateArray[i];
    }
    information.smoothFrameRate = averagedFrames / smoothCount;

    m_frameBuffer.append(information);

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