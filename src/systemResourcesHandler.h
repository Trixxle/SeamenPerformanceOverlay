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

#ifndef PERFORMANCEVR_SYSTEMRESOURCESHANDLER_H
#define PERFORMANCEVR_SYSTEMRESOURCESHANDLER_H

#include <windows.h>
#include <dxgi.h>
#include <vector>
#include <pdh.h>
#include <pdhmsg.h>
#include <iostream>
#include <QObject>
#include <QThread>
#include <QTimer>
#include <QElapsedTimer>
#include "src/steamvrcontrol/steamvrlogic.h"

class SystemResourcesHandler: public QObject{
    Q_OBJECT

public:
    SystemResourcesHandler(QObject* parent = nullptr);
    virtual ~SystemResourcesHandler();

    struct systemResourceUsage {
        float vramUsage = 0.0;
        float ramUsage = 0.0;
    };

    struct systemResources {
        float systemVram = 0.0;
        float systemRam = 0.0;
    };

public slots:
    void startSystemResourcesProcessing();
    void processSystemResources();

private:
    void getSystemTotalRam();
    void getSystemTotalVram();

    void getSystemRamUsage();
    void getSystemVramUsage();

    QTimer* m_pUpdateTimer = nullptr;
    const qint64 UI_UPDATE_INTERVAL_MS = 2000; // Interval for UI updating
    systemResourceUsage m_systemResourceUsage;
    systemResources m_systemResources;
    PDH_HQUERY m_pdhQuery = nullptr;
    PDH_HCOUNTER m_vramCounter = nullptr;

    bool m_vramWarningTriggered = false;
    bool m_ramWarningTriggered = false;

signals:
    void updateSystemResources(systemResources newSystemResources);
    void updateSystemResourceUsage(systemResourceUsage newSystemResourceUsage);
    void notifyUser(QString message, SteamVRLogic::notificationType type, vr::TrackedDeviceIndex_t device);
};

Q_DECLARE_METATYPE(SystemResourcesHandler::systemResourceUsage);

#endif //PERFORMANCEVR_SYSTEMRESOURCESHANDLER_H