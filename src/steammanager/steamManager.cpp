//
// Created by jornt on 19/08/2026.
//

#include "steamManager.h"

SteamManager::SteamManager() {
}

bool SteamManager::attemptSteamApiInit() {
    m_steamIsActive = SteamAPI_Init();
    m_steamIsActive ? qDebug() << "Connected to Steam API." : qDebug() << "Failed to connect Steam API. Running open "
                                                                          "source standalone mode.";
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
