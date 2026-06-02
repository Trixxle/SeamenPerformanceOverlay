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

#ifndef SEAMENPERFORMANCEOVERLAY_USERSETTINGS_H
#define SEAMENPERFORMANCEOVERLAY_USERSETTINGS_H

#include <QObject>
#include <QSettings>
#include "openvr.h"

class userSettings : public QObject {
    Q_OBJECT

public:
    enum class colorBlindType {
        none = 0,
        protanopia = 1,
        deuteranopia = 2,
        tritanopia = 3
    };

    static userSettings &instance() {
        static userSettings _instance;
        return _instance;
    };

    userSettings(const userSettings&) = delete;
    userSettings& operator=(const userSettings&) = delete;

    void increaseDistanceFadeValue();
    void decreaseDistanceFadeValue();
    void increaseSize();
    void decreaseSize();
    void increaseOpacity();
    void decreaseOpacity();

    // Setters
    void setSize(float newSize);
    void setOpacity(float newOpacity);
    void setDistanceFadeValue(float newDistanceValue);
    void setDistanceFadeState(bool newState);
    void setColorblindness(colorBlindType newType);
    void setShowTrackers(bool newState);
    void setSavedRole(vr::ETrackedControllerRole newRole);
    void setMatrix(const vr::HmdMatrix34_t& newMatrix);

    // Getters
    [[nodiscard]] float getSize() const;
    [[nodiscard]] float getOpacity() const;
    [[nodiscard]] float getDistanceFadeValue() const;
    [[nodiscard]] bool getDistanceFadeState() const;
    [[nodiscard]] bool getShowTrackers() const;
    [[nodiscard]] vr::ETrackedControllerRole getSavedRole() const;
    [[nodiscard]] vr::HmdMatrix34_t getMatrix() const;
    [[nodiscard]] colorBlindType getColorBlindness() const;

private:

    userSettings();
    ~userSettings() override = default;

    void loadSettings();
    void saveSettings();

    QSettings m_settings{"Seamen", "PerformanceOverlay"};

    float m_userOpacity = 1.0f;
    float m_userSize = 0.2f;
    float m_userDistanceFadeValue = 0.4f;

    colorBlindType m_userColorblindness = colorBlindType::none;

    bool m_userDistanceFadeState = false;
    bool m_userShowTrackers = true;

    vr::HmdMatrix34_t m_userMatrix = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.866f, 0.5f, 0.1f,
        0.0f, -0.5f, 0.866f, -0.08f
    };

    vr::ETrackedControllerRole m_userSavedRole = vr::TrackedControllerRole_LeftHand;

signals:
    void sizeChanged(float newSize);
    void opacityChanged(float newOpacity);
    void distanceFadeValueChanged(float newDistanceValue);
    void showTrackersChanged(bool newState);
    void savedRoleChanged(vr::ETrackedControllerRole newRole);
    void matrixChanged(const vr::HmdMatrix34_t& newMatrix);
    void colorblindnessChanged(colorBlindType newType);
    void distanceFadeStateChanged(bool newState);
};

#endif //SEAMENPERFORMANCEOVERLAY_USERSETTINGS_H