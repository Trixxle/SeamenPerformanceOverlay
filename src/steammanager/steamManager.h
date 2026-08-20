//
// Created by jornt on 19/08/2026.
//

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