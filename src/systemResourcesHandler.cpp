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
#include <QLibrary>

#include "steamvrcontrol/steamvrlogic.h"

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
    // Clean up the PDH query when the handler is destroyed
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
        if (SUCCEEDED(factory->EnumAdapters(0, &adapter))) {
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

    // Only collect and format the data in the hot loop
    PdhCollectQueryData(m_pdhQuery);

    DWORD bufferSize = 0;
    DWORD itemCount = 0;

    // First call to get the required buffer size.
    PDH_STATUS status = PdhGetFormattedCounterArrayA(m_vramCounter, PDH_FMT_LARGE, &bufferSize, &itemCount, nullptr);

    int retries = 3;

    while ((status == static_cast<PDH_STATUS>(PDH_MORE_DATA) || status == ERROR_SUCCESS) && bufferSize > 0 && retries > 0) {
        std::vector<BYTE> buffer(bufferSize);
        auto* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_A*>(buffer.data());

        // Second call to actually get the data
        status = PdhGetFormattedCounterArrayA(m_vramCounter, PDH_FMT_LARGE, &bufferSize, &itemCount, items);

        if (status == ERROR_SUCCESS) {
            LONGLONG totalBytes = 0;
            for (DWORD i = 0; i < itemCount; i++) {
                if (items[i].FmtValue.CStatus == PDH_CSTATUS_VALID_DATA || items[i].FmtValue.CStatus == PDH_CSTATUS_NEW_DATA) {
                    totalBytes += items[i].FmtValue.largeValue;
                }
            }
            m_systemResourceUsage.vramUsage = static_cast<float>(totalBytes) / (1024.0f * 1024.0f * 1024.0f);
            break;
        }
        else if (status != static_cast<PDH_STATUS>(PDH_MORE_DATA)) {
            break;
        }

        retries--;
    }
}

void SystemResourcesHandler::getSystemGpuTemperature() {
    m_systemResourceUsage.gpuTemperature = -1;

    // WDDM exposes the same temperature query for every GPU.
    struct OpenAdapterFromLuid {
        LUID AdapterLuid;
        UINT hAdapter;
    };
    struct CloseAdapter {
        UINT hAdapter;
    };
    struct QueryAdapterInfo {
        UINT hAdapter;
        UINT Type;
        void* pPrivateDriverData;
        UINT PrivateDriverDataSize;
    };
    struct AdapterPerfData {
        UINT PhysicalAdapterIndex;
        alignas(8) ULONGLONG MemoryFrequency;
        alignas(8) ULONGLONG MaxMemoryFrequency;
        alignas(8) ULONGLONG MaxMemoryFrequencyOC;
        alignas(8) ULONGLONG MemoryBandwidth;
        alignas(8) ULONGLONG PCIEBandwidth;
        ULONG FanRPM;
        ULONG Power;
        ULONG Temperature;
        UCHAR PowerStateOverride;
    };
    static_assert(sizeof(AdapterPerfData) == 64);
    constexpr UINT KMTQAITYPE_ADAPTERPERFDATA = 62;

    static const auto openAdapter = reinterpret_cast<LONG (WINAPI*)(OpenAdapterFromLuid*)>(
        QLibrary::resolve(QStringLiteral("gdi32"), "D3DKMTOpenAdapterFromLuid"));
    static const auto queryAdapter = reinterpret_cast<LONG (WINAPI*)(const QueryAdapterInfo*)>(
        QLibrary::resolve(QStringLiteral("gdi32"), "D3DKMTQueryAdapterInfo"));
    static const auto closeAdapter = reinterpret_cast<LONG (WINAPI*)(const CloseAdapter*)>(
        QLibrary::resolve(QStringLiteral("gdi32"), "D3DKMTCloseAdapter"));
    if (!openAdapter || !queryAdapter || !closeAdapter) return;

    // Match the GPU used by SteamVR, including systems with multiple GPUs.
    int32_t adapterIndex = 0;
    if (vr::VRSystem()) vr::VRSystem()->GetDXGIOutputInfo(&adapterIndex);
    if (adapterIndex < 0) return;

    IDXGIFactory* factory = nullptr;
    if (FAILED(CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&factory)))) return;

    IDXGIAdapter* adapter = nullptr;
    const HRESULT adapterResult = factory->EnumAdapters(static_cast<UINT>(adapterIndex), &adapter);
    factory->Release();
    if (FAILED(adapterResult)) return;

    DXGI_ADAPTER_DESC desc{};
    const HRESULT descResult = adapter->GetDesc(&desc);
    adapter->Release();
    if (FAILED(descResult)) return;

    OpenAdapterFromLuid openInfo{desc.AdapterLuid, 0};
    if (openAdapter(&openInfo) < 0) return;

    AdapterPerfData perfData{};
    QueryAdapterInfo queryInfo{openInfo.hAdapter, KMTQAITYPE_ADAPTERPERFDATA, &perfData, sizeof(perfData)};
    const LONG status = queryAdapter(&queryInfo);
    CloseAdapter closeInfo{openInfo.hAdapter};
    closeAdapter(&closeInfo);

    if (status >= 0 && perfData.Temperature > 0 && perfData.Temperature <= 2000) {
        m_systemResourceUsage.gpuTemperature = static_cast<int>((perfData.Temperature + 5) / 10);
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
    getSystemGpuTemperature();
    if (m_systemResources.systemVram - m_systemResourceUsage.vramUsage < 0.5 && !m_vramWarningTriggered) {
        emit notifyUser("VRAM is almost full. - Prevent performance degradation, lower "
                            "render resolution, texture settings, or hide avatars.", SteamVRLogic::notificationType::alert, vr::k_unTrackedDeviceIndexInvalid);
        m_vramWarningTriggered = true;
    }
    else if (m_systemResources.systemVram - m_systemResourceUsage.vramUsage > 0.5 && m_vramWarningTriggered) m_vramWarningTriggered = false;

    if (m_systemResources.systemRam - m_systemResourceUsage.ramUsage < 0.5 && !m_ramWarningTriggered) {
        emit notifyUser("RAM is almost full. - Prevent performance degradation, close "
                            "background apps such as browsers, Discord, etc.", SteamVRLogic::notificationType::alert, vr::k_unTrackedDeviceIndexInvalid);
        m_ramWarningTriggered = true;
    }
    else if (m_systemResources.systemRam - m_systemResourceUsage.ramUsage > 0.5 && m_ramWarningTriggered) m_ramWarningTriggered = false;

    emit updateSystemResourceUsage(m_systemResourceUsage);
}
