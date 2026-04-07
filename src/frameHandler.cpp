//
// Created by jornt on 10/03/2026.
//

#include "frameHandler.h"

FrameHandler::FrameHandler(float newRefreshRate, float newMaxFrameRate,DashboardUI *dashboard, QObject *parent):
    QObject(parent),
    m_dashboard(dashboard),
    m_lastFrameSampleIndex(0.0),
    m_targetFrameRate(newMaxFrameRate),
    m_targetRefreshRateMs(newRefreshRate)
{
}

FrameHandler::~FrameHandler() {
}

void FrameHandler::run() {
    std::cout << "Frame thread is alive!! " << std::endl;
    vr::Compositor_FrameTiming currentFrame;
    vr::Compositor_FrameTiming previousFrame;
    frameStats information;

    // Calculate a sleep interval so to not overload the CPU
    unsigned long sleepTimeMs = static_cast<unsigned long>(m_targetRefreshRateMs / 2.0f);
    if (sleepTimeMs == 0) sleepTimeMs = 5; // Fallback to 5ms just in case

    while (true) {
        calculateFrameData(currentFrame, previousFrame, information);
        QThread::msleep(sleepTimeMs);
    }
}

void FrameHandler::calculateFrameData(vr::Compositor_FrameTiming &currentFrame, vr::Compositor_FrameTiming &previousFrame, frameStats &information) {

    currentFrame.m_nSize = sizeof(vr::Compositor_FrameTiming);
    previousFrame.m_nSize = sizeof(vr::Compositor_FrameTiming);

    // Fetch the most recent frame just to get the latest index
    if (!vr::VRCompositor()->GetFrameTiming(&currentFrame, 0)) return;

    uint32_t currentFrameIndex = currentFrame.m_nFrameIndex;
    uint32_t amountOfFramesSinceLast = currentFrameIndex - m_lastFrameSampleIndex;

    // Safety checks
    if (amountOfFramesSinceLast == 0) return;

    // OpenVR only stores 128 frames. Cap it so we don't query out of bounds.
    if (amountOfFramesSinceLast > 128) {
        amountOfFramesSinceLast = 128;
    }

    double gpuFrametimeMs = 0;
    double cpuFrametimeMs = 0;
    double totalFrametimeMs = 0;
    uint32_t validFramesProcessed = 0;

    for (uint32_t i = 0; i < amountOfFramesSinceLast; i++) {

        bool validCurrent = vr::VRCompositor()->GetFrameTiming(&currentFrame, i);
        bool validPrev = vr::VRCompositor()->GetFrameTiming(&previousFrame, i + 1);

        // Break early if we've exhausted the compositor's frame history
        if (!validCurrent || !validPrev) break;

        // TODO: FIX THIS GOD DAMN THING
        // SKIP in-flight or fully dropped frames where Submit was never called
        // if (currentFrame.m_flNewFrameReadyMs == 0.0f) continue;

        gpuFrametimeMs += currentFrame.m_flTotalRenderGpuMs;

        // CPU Timing calculation
        cpuFrametimeMs += (currentFrame.m_flNewFrameReadyMs -
                            currentFrame.m_flNewPosesReadyMs +
                            currentFrame.m_flCompositorRenderCpuMs);

        // Delta time between system timestamps
        totalFrametimeMs += (currentFrame.m_flSystemTimeInSeconds -
                            previousFrame.m_flSystemTimeInSeconds) * 1000.0;

        validFramesProcessed++;
    }

    m_lastFrameSampleIndex = currentFrameIndex;

    // If no valid frames were found exit
    if (validFramesProcessed == 0) return;

    // Averaging the results
    gpuFrametimeMs /= validFramesProcessed;
    cpuFrametimeMs /= validFramesProcessed;
    totalFrametimeMs /= validFramesProcessed;

    m_lastFrameSampleIndex = currentFrameIndex;

    information.GpuFrametime = static_cast<float>(gpuFrametimeMs);
    information.CpuFrametime = static_cast<float>(cpuFrametimeMs);
    information.TotalFrametime = static_cast<float>(totalFrametimeMs);

    // Calculate Framerate: Limit between 0 and targetRefreshRate
    float calculatedFps = (totalFrametimeMs > 0) ? (1.0f / static_cast<float>(totalFrametimeMs) * 1000.0f) : 0;

    information.Framerate = std::max(0.0f, std::min(m_targetFrameRate, calculatedFps));
    information.MaxFrametime = m_targetRefreshRateMs;
    information.MaxFramerate = (float) m_targetFrameRate;

    // Safely execute the UI updates on the main thread
    QMetaObject::invokeMethod(m_dashboard, [this, information]() {
        m_dashboard->setDashboardFrameRate(information.Framerate);
        m_dashboard->setDashboardFrameTime(information.TotalFrametime);
        m_dashboard->setDashboardCpuFrameTime(information.CpuFrametime);
        m_dashboard->setDashboardGpuFrameTime(information.GpuFrametime);
        m_dashboard->setDashboardTargetFrameRate(information.MaxFramerate);
        m_dashboard->setDashboardHeadsetRefreshRate(information.MaxFrametime);

        m_dashboard->updateTotalFrameTimeGraph(information.TotalFrametime);
        m_dashboard->updateGpuFrameTimeGraph(information.GpuFrametime);
        m_dashboard->updateCpuFrameTimeGraph(information.CpuFrametime);


    });

    /*
    // Update UI values
    m_dashboard->setDashboardFrameRate(information.Framerate);
    m_dashboard->setDashboardFrameTime(information.TotalFrametime);
    m_dashboard->setDashboardCpuFrameTime(information.CpuFrametime);
    m_dashboard->setDashboardGpuFrameTime(information.GpuFrametime);
    m_dashboard->setDashboardTargetFrameRate(information.MaxFramerate);
    m_dashboard->setDashboardHeadsetRefreshRate(information.MaxFrametime);
    */

    std::cout << "Framerate: " << information.Framerate << std::endl;
    std::cout << "GpuFrameTime: " << information.GpuFrametime << std::endl;
    std::cout << "TotalFrameTime: " << information.TotalFrametime << std::endl;
    std::cout << "CpuFrametime: " << information.CpuFrametime << std::endl;
}

/*
void FrameHandler::calculateFrameData(vr::Compositor_FrameTiming &currentFrame, vr::Compositor_FrameTiming &previousFrame, frameStats &information) {

    currentFrame.m_nSize = sizeof(vr::Compositor_FrameTiming);
    previousFrame.m_nSize = sizeof(vr::Compositor_FrameTiming);

    vr::VRCompositor()->GetFrameTiming(&currentFrame, 0);

    uint32_t currentFrameIndex = currentFrame.m_nFrameIndex;
    uint32_t amountOfFramesSinceLast = currentFrameIndex - m_lastFrameSampleIndex;

    // Safety check to avoid division by zero or massive loops if first frame
    if (amountOfFramesSinceLast == 0) return;

    double gpuFrametimeMs = 0;
    double cpuFrametimeMs = 0;
    double totalFrametimeMs = 0;

    for (uint32_t i = 0; i < amountOfFramesSinceLast; i++) {
        // Fetch timings for the specific frame offset
        vr::VRCompositor()->GetFrameTiming(&currentFrame, i);
        vr::VRCompositor()->GetFrameTiming(&previousFrame, i + 1);

        gpuFrametimeMs += currentFrame.m_flTotalRenderGpuMs;

        // CPU Timing calculation
        cpuFrametimeMs += (currentFrame.m_flNewFrameReadyMs -
                            currentFrame.m_flNewPosesReadyMs +
                            currentFrame.m_flCompositorRenderCpuMs);

        // Delta time between system timestamps (converted to ms)
        totalFrametimeMs += (currentFrame.m_flSystemTimeInSeconds -
                            previousFrame.m_flSystemTimeInSeconds) * 1000.0;
    }

    // Averaging the results
    gpuFrametimeMs /= amountOfFramesSinceLast;
    cpuFrametimeMs /= amountOfFramesSinceLast;
    totalFrametimeMs /= amountOfFramesSinceLast;

    m_lastFrameSampleIndex = currentFrameIndex;

    information.GpuFrametime = static_cast<float>(gpuFrametimeMs);
    information.CpuFrametime = static_cast<float>(cpuFrametimeMs);
    information.TotalFrametime = static_cast<float>(totalFrametimeMs);

    // Calculate Framerate: Limit between 0 and targetRefreshRate
    float calculatedFps = (totalFrametimeMs > 0) ? (1.0f / static_cast<float>(totalFrametimeMs) * 1000.0f) : 0;

    information.Framerate = std::max(0.0f, std::min(m_targetFrameRate, calculatedFps));
    information.MaxFrametime = m_targetRefreshRateMs;
    information.MaxFramerate = (float) m_targetFrameRate;

    // Update UI values
    m_dashboard->setDashboardFrameRate(information.Framerate);
    m_dashboard->setDashboardFrameTime(information.TotalFrametime);
    m_dashboard->setDashboardCpuFrameTime(information.CpuFrametime);
    m_dashboard->setDashboardGpuFrameTime(information.GpuFrametime);
    m_dashboard->setDashboardTargetFrameRate(information.MaxFramerate);
    m_dashboard->setDashboardHeadsetRefreshRate(information.MaxFrametime);

    std::cout << "Framerate: " << information.Framerate << std::endl;
    std::cout << "GpuFrameTime: " << information.GpuFrametime << std::endl;
    std::cout << "TotalFrameTime: " << information.TotalFrametime << std::endl;
    std::cout << "CpuFrametime: " << information.CpuFrametime << std::endl;
}*/