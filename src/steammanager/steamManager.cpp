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

#include "steamManager.h"

SteamManager::SteamManager() {
}

bool SteamManager::attemptSteamApiInit() {
    m_steamIsActive = SteamAPI_Init();
    if (m_steamIsActive) qDebug() << "Connected to Steam API.";
    else qDebug() << "Failed to connect Steam API. Running open source standalone mode.";
    return m_steamIsActive;
}

bool SteamManager::checkDlcActive(int dlcID) {
    if (!m_steamIsActive) return false;
    bool hasDlc = false;
    hasDlc = SteamApps()->BIsSubscribedApp(dlcID);
    return hasDlc;
}

void SteamManager::setUserAchievement(const char* achievementID) {
    if (!m_steamIsActive) {
        qDebug() << "Could not give achievement " << achievementID << " to user. Steam API not connected.";
        return;
    }
    bool hasAchievement = false;
    SteamUserStats()->GetAchievement(achievementID, &hasAchievement);
    if (!hasAchievement) {
        SteamUserStats()->SetAchievement(achievementID);
        SteamUserStats()->StoreStats();
        qDebug() << "Gave achievement " << achievementID << " to user.";
    }
    else qDebug() << "User already has achievement " << achievementID << ".";
}

// Getters
bool SteamManager::getSteamActive() {
    return m_steamIsActive;
}
