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

#ifndef SEAMENPERFORMANCEOVERLAY_STEAMMANAGER_H
#define SEAMENPERFORMANCEOVERLAY_STEAMMANAGER_H

#include <QObject>
#include <QDebug>
#include "steam_api.h"

class SteamManager: public QObject {
    Q_OBJECT
public:

    static SteamManager &instance() {
        static SteamManager _instance;
        return _instance;
    };

    SteamManager(const SteamManager&) = delete;
    SteamManager& operator=(const SteamManager&) = delete;

    void setUserAchievement(const char* achievementID);
    bool attemptSteamApiInit();
    bool checkDlcActive(int dlcID);

    // Getters
    bool getSteamActive();

private:
    SteamManager();
    ~SteamManager() override = default;

    bool m_steamIsActive = false;
};


#endif //SEAMENPERFORMANCEOVERLAY_STEAMMANAGER_H