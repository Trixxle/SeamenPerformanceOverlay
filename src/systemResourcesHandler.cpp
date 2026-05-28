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

#include "systemResourcesHandler.h"

SystemResourcesHandler::SystemResourcesHandler(QObject* parent):
    QObject(parent),
    m_pdhQuery(nullptr),
    m_vramCounter(nullptr)
{
    getSystemTotalVram();
    getSystemTotalRam();

    if (PdhOpenQuery(nullptr, 0, &m_pdhQuery) == ERROR_SUCCESS) {
        if (PdhAddEnglishCounterA(m_pdhQuery, "\\GPU Adapter Memory(*)\\Dedicated Usage", 0, &m_vramCounter) != ERROR_SUCCESS) {
            PdhCloseQuery(m_pdhQuery);
            m_pdhQuery = nullptr;
            m_vramCounter = nullptr;
        } else {
            PdhCollectQueryData(m_pdhQuery);
        }
    }
}

SystemResourcesHandler::~SystemResourcesHandler() {
    // 3. Clean up the PDH query when the handler is destroyed
    if (m_pdhQuery) {
        PdhCloseQuery(m_pdhQuery);
        m_pdhQuery = nullptr;
    }
}

void SystemResourcesHandler::getSystemTotalRam() {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        m_systemResources.systemRam = static_cast<float>(memInfo.ullTotalPhys) / (1024.0f * 1024.0f * 1024.0f);
    }
}

void SystemResourcesHandler::getSystemTotalVram() {
    IDXGIFactory* factory = nullptr;

    // Create the DXGI Factory
    if (SUCCEEDED(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory))) {
        IDXGIAdapter* adapter = nullptr;

        // Index 0 is typically the primary display adapter
        if (factory->EnumAdapters(0, &adapter) != DXGI_ERROR_NOT_FOUND) {
            DXGI_ADAPTER_DESC desc;

            if (SUCCEEDED(adapter->GetDesc(&desc))) {
                // Convert from Bytes to GB.
                m_systemResources.systemVram = static_cast<float>(desc.DedicatedVideoMemory) / (1024.0f * 1024.0f * 1024.0f);
                std::cout << "System total VRAM: " << m_systemResources.systemVram << " GB" << std::endl;
            }
            adapter->Release();
        }
        factory->Release();
    }
}

void SystemResourcesHandler::getSystemRamUsage() {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        m_systemResourceUsage.ramUsage = static_cast<float>(memInfo.ullTotalPhys - memInfo.ullAvailPhys) / (1024.0f * 1024.0f * 1024.0f);
    }
}

void SystemResourcesHandler::getSystemVramUsage() {
    // Ensure PDH initialized successfully before attempting to collect
    if (!m_pdhQuery || !m_vramCounter) return;

    // 2. Only collect and format the data in the hot loop
    PdhCollectQueryData(m_pdhQuery);

    DWORD bufferSize = 0;
    DWORD itemCount = 0;

    // First call to get the required buffer size
    PdhGetFormattedCounterArrayA(m_vramCounter, PDH_FMT_LARGE, &bufferSize, &itemCount, nullptr);

    if (bufferSize > 0 && itemCount > 0) {
        std::vector<BYTE> buffer(bufferSize);
        auto* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_A*>(buffer.data());

        // Second call to actually get the data
        if (PdhGetFormattedCounterArrayA(m_vramCounter, PDH_FMT_LARGE, &bufferSize, &itemCount, items) == ERROR_SUCCESS) {
            LONGLONG totalBytes = 0;
            for (DWORD i = 0; i < itemCount; i++) {
                if (items[i].FmtValue.CStatus == PDH_CSTATUS_VALID_DATA || items[i].FmtValue.CStatus == PDH_CSTATUS_NEW_DATA) {
                    totalBytes += items[i].FmtValue.largeValue;
                }
            }
            m_systemResourceUsage.vramUsage = static_cast<float>(totalBytes) / (1024.0f * 1024.0f * 1024.0f);
        }
    }
}

void SystemResourcesHandler::startSystemResourcesProcessing() {
    if (!m_pUpdateTimer) {
        m_pUpdateTimer = new QTimer(this);

        // As the interval is 2 seconds there is no need to waste resources on a precise timer
        m_pUpdateTimer->setTimerType(Qt::VeryCoarseTimer);

        connect(m_pUpdateTimer, &QTimer::timeout, this, &SystemResourcesHandler::processSystemResources);

        emit updateSystemResources(m_systemResources);

        m_pUpdateTimer->start(UI_UPDATE_INTERVAL_MS);

        std::cout << "System resources timer started on thread: " << QThread::currentThreadId() << std::endl;
    }
}

void SystemResourcesHandler::processSystemResources() {
    getSystemRamUsage();
    getSystemVramUsage();
    emit updateSystemResourceUsage(m_systemResourceUsage);
}
