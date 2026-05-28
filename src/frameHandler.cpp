/*
Original work Copyright (C) 2026 Jorn ten Kate, The Seamen.

This program is also licensed under the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version.

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
    if (!m_pTimer) {
        m_pTimer = new QTimer(this);

        // PreciseTimer to prevent standard OS scheduling drift
        m_pTimer->setTimerType(Qt::PreciseTimer);

        // QTimer takes integer milliseconds. 11.11ms becomes 11ms. Important note: It doesn't round the number, it just truncates it. So 6.9 becomes 6
        int intervalMs = static_cast<int>(m_targetRefreshRateMs);
        if (intervalMs <= 0) intervalMs = 5; // Fallback

        connect(m_pTimer, &QTimer::timeout, this, &FrameHandler::processFrame);

        m_uiUpdateTimerGraphs.start();
        m_uiUpdateTimerLabels.start();
        m_pTimer->start(intervalMs);

        std::cout << "Frame timer started on thread: " << QThread::currentThreadId() << std::endl;
    }
}

void FrameHandler::stopProcessing() {
    if (m_pTimer) {
        m_pTimer->stop();
        m_pTimer->deleteLater();
        m_pTimer = nullptr;
    }
}


/*
Original work Copyright (c) 2015, Valve Corporation. All rights reserved.
Modified work Copyright (C) 2026 Jorn ten Kate, The Seamen.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

void FrameHandler::processFrame() {
    m_currentFrame.m_nSize = sizeof(vr::Compositor_FrameTiming);

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

    // Delta time between system timestamps.
    // Calculates the absolute time interval between the current frame and the previous one
    frameDeliverySmoothnessMS = (m_currentFrame.m_flSystemTimeInSeconds -
                        m_previousFrame.m_flSystemTimeInSeconds) * 1000.0;

    // if m_flClientFrameIntervalMs is 0 it means there is no active game being rendered and m_flNewFrameReadyMs would have garbage results
    // Negative result mean a frame drop
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
    m_information.smoothFrameRate = (smoothCount > 0) ? (averagedFrames / smoothCount) : 0;
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
