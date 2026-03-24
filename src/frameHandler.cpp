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
    while (true) {
        calculateFrameData();
    }
}

void FrameHandler::calculateFrameData() {
    vr::Compositor_FrameTiming currentFrame;
        vr::Compositor_FrameTiming previousFrame;
        frameStats information;

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

        std::cout << "target refresh rate: " << m_targetFrameRate << std::endl;
        std::cout << "target refresh rate in ms: " << m_targetRefreshRateMs << std::endl;

        std::cout << "Calculated fps: " << calculatedFps << std::endl;

        information.MaxFrametime = m_targetRefreshRateMs;
        information.MaxFramerate = (float)m_targetFrameRate;
        m_dashboard->setGpuFrameRate(information.Framerate);
        std::cout << "Framerate: " << information.Framerate << std::endl;
        std::cout << "GpuFrameTime: " << information.GpuFrametime << std::endl;
        std::cout << "TotalFrameTime: " << information.TotalFrametime << std::endl;
        std::cout << "CpuFrametime: " << information.CpuFrametime << std::endl;
}