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

#include "userSettings.h"

userSettings::userSettings() {
    loadSettings();
}

// All values are already initialized in the header. If any fail to load its not catastrophic
void userSettings::loadSettings() {
    qDebug() << "Loading user settings...";
    if (!m_settings.value("TransformMatrix").isNull()) {
        QList<QVariant> matrixValues = m_settings.value("TransformMatrix").toList();
        if (matrixValues.size() == 12) {
            int k = 0;
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 4; ++j) {
                    m_userMatrix.m[i][j] = matrixValues[k++].toFloat();
                }
            }
        }
        qDebug() << "Loaded transform matrix: " << matrixValues;
    }
    else qDebug() << "No transform matrix found.";

    // Restore controller attached to
    if (!m_settings.value("AttachedRole").isNull()) {
        int savedRole = m_settings.value("AttachedRole").toInt();
        m_userSavedRole = static_cast<vr::ETrackedControllerRole>(savedRole);
        qDebug() << "Loaded attached role: " << m_userSavedRole;
    }
    else qDebug() << "No attached role found.";

    // Restore size
    if (!m_settings.value("Size").isNull()) {
        m_userSize = m_settings.value("Size").toFloat();
        qDebug() << "Loaded size: " << m_userSize;
    }
    else qDebug() << "No size found.";

    // Restore distance fade start
    if (!m_settings.value("DistanceFadeStart").isNull()) {
        m_userDistanceFadeValue = m_settings.value("DistanceFadeStart").toFloat();
        qDebug() << "Loaded distance fade value: " << m_userDistanceFadeValue;
    }
    else qDebug() << "No distance fade value found.";

    // Restore distance fade
    if (!m_settings.value("DistanceFadeOn").isNull()) {
        m_userDistanceFadeState = m_settings.value("DistanceFadeOn").toBool();
        qDebug() << "Loaded distance fade state: " << m_userDistanceFadeState;
    }
    else qDebug() << "No distance fade state found.";

    // Restore opacity
    if (!m_settings.value("Opacity").isNull()) {
        m_userOpacity = m_settings.value("Opacity").toFloat();
        qDebug() << "Loaded opacity value: " << m_userOpacity;
    }
    else qDebug() << "No opacity value found.";

    // Restore colorblindness
    QVariant colorBlindSetting = m_settings.value("ColorBlindness");
    if (!colorBlindSetting.isNull()) {
        int temp = colorBlindSetting.toInt();
        if (temp >= 0 && temp <= 3) {
            m_userColorblindness = static_cast<colorBlindType>(temp);
        }
        qDebug() << "Loaded colorblind option.";
    }
    else qDebug() << "No colorblind option found.";

    // Restore showing tracker
    if (!m_settings.value("ShowTrackers").isNull()) {
        m_userShowTrackers = m_settings.value("ShowTrackers").toBool();
        qDebug() << "Loaded showing tracker state: " << m_userShowTrackers;
    }
    else qDebug() << "No showing tracker state found.";

    // Restore notification state
    if (!m_settings.value("ShowNotifications").isNull()) {
        m_userShowNotifications = m_settings.value("ShowNotifications").toBool();
        qDebug() << "Loaded showing notification state: " << m_userShowNotifications    ;
    }
    else qDebug() << "No showing notification state found.";

    // Restore notification volume
    if (!m_settings.value("Volume").isNull()) {
        m_userNotificationVolume = m_settings.value("Volume").toFloat();
        qDebug() << "Loaded volume: " << m_userNotificationVolume;
    }
    else qDebug() << "No volume value found.";
}

void userSettings::saveSettings() {
    // Saving colorblindness
    m_settings.setValue("ColorBlindness", static_cast<int>(m_userColorblindness));
    qDebug() << "Saved colorblind option.";

    // Saving matrix
    // Flatten the 3x4 matrix into a list for easy saving
    QList<QVariant> matrixValues;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 4; ++j) {
            matrixValues.append(m_userMatrix.m[i][j]);
        }
    }
    m_settings.setValue("TransformMatrix", matrixValues);
    qDebug() << "Saved transform matrix: " << matrixValues;

    // Saving size
    m_settings.setValue("Size", m_userSize);
    qDebug() << "Saved size: " << m_userSize;

    // Saving role
    m_settings.setValue("AttachedRole", m_userSavedRole);
    qDebug() << "Saved attached role: " << m_userSavedRole;

    // Saving distance fade value
    m_settings.setValue("DistanceFadeStart", m_userDistanceFadeValue);
    qDebug() << "Saved distance fade value: " << m_userDistanceFadeValue;

    // Saving distance fade
    m_settings.setValue("DistanceFadeOn", m_userDistanceFadeState);
    qDebug() << "Saved distance fade state: " << m_userDistanceFadeState;

    // Saving showing trackers
    m_settings.setValue("ShowTrackers", m_userShowTrackers);
    qDebug() << "Saved showing trackers state: " << m_userShowTrackers;

    // Saving opacity
    m_settings.setValue("Opacity", m_userOpacity);
    qDebug() << "Saved opacity value: " << m_userOpacity;

    // Saving notification state
    m_settings.setValue("ShowNotifications", m_userShowNotifications);
    qDebug() << "Saved showing notification state: " << m_userShowNotifications;

    // Saving notification volume
    m_settings.setValue("Volume", m_userNotificationVolume);
    qDebug() << "Saved notification volume: " << m_userNotificationVolume;
}

void userSettings::increaseVolume() {
    setNotificationVolume(m_userNotificationVolume + 0.1f);
}

void userSettings::decreaseVolume() {
    setNotificationVolume(m_userNotificationVolume - 0.1f);
}

void userSettings::increaseSize(){
    setSize(m_userSize + 0.01f);
}

void userSettings::decreaseSize() {
    if (m_userSize <= 0.0) return;
    setSize(qBound(0.0f, m_userSize - 0.01f, 100.0f));
}

void userSettings::increaseOpacity() {
    float newOpacity = qBound(0.0f, m_userOpacity + 0.05f, 1.0f);
    if (newOpacity > 0.99f) newOpacity = 1.0f;
    setOpacity(newOpacity);
}

void userSettings::decreaseOpacity() {
    float newOpacity = qBound(0.0f, m_userOpacity - 0.05f, 1.0f);
    if (newOpacity < 0.01f) newOpacity = 0.0f;
    setOpacity(newOpacity);
}

void userSettings::increaseDistanceFadeValue() {
    setDistanceFadeValue(m_userDistanceFadeValue + 0.01f);
}

void userSettings::decreaseDistanceFadeValue() {
    if (m_userDistanceFadeValue <= 0.0) return;
    setDistanceFadeValue(qBound(0.0f, m_userDistanceFadeValue - 0.01f, 100.0f));
}

// Setters
void userSettings::setSize(const float newSize) {
    m_userSize = qBound(0.0f, newSize, 100.0f);
    //saveSettings();
    emit sizeChanged(m_userSize);
}

void userSettings::setOpacity(const float newOpacity) {
    m_userOpacity = qBound(0.0f, newOpacity, 1.0f);
    //saveSettings();
    emit opacityChanged(m_userOpacity);
}

void userSettings::setDistanceFadeValue(const float newDistanceValue) {
    m_userDistanceFadeValue = qBound(0.0f, newDistanceValue, 100.0f);
    //saveSettings();
    emit distanceFadeValueChanged(m_userDistanceFadeValue);
}

void userSettings::setDistanceFadeState(const bool newState) {
    m_userDistanceFadeState = newState;
    //saveSettings();
    emit distanceFadeStateChanged(newState);
}

void userSettings::setColorblindness(const colorBlindType newType) {
    m_userColorblindness = newType;
    //saveSettings();
    emit colorblindnessChanged(newType);
}

void userSettings::setShowTrackers(const bool newState) {
    m_userShowTrackers = newState;
    //saveSettings();
    emit showTrackersChanged(newState);
}

void userSettings::setSavedRole(const vr::ETrackedControllerRole newRole) {
    m_userSavedRole = newRole;
    //saveSettings();
    emit savedRoleChanged(newRole);
}

void userSettings::setMatrix(const vr::HmdMatrix34_t& newMatrix) {
    m_userMatrix = newMatrix;
    //saveSettings();
    emit matrixChanged(newMatrix);
}

void userSettings::setNotificationVolume(float newVolume) {
    m_userNotificationVolume = qBound(0.0f, newVolume, 1.0f);
    emit notificationVolumeChanged(m_userNotificationVolume);
}

void userSettings::setNotificationState(bool newState) {
    m_userShowNotifications = newState;
    emit notificationStateChanged(m_userShowNotifications);
}

// Getters
float userSettings::getSize() const {
    return m_userSize;
}

float userSettings::getOpacity() const {
    return m_userOpacity;
}

float userSettings::getDistanceFadeValue() const {
    return m_userDistanceFadeValue;
}

bool userSettings::getDistanceFadeState() const {
    return m_userDistanceFadeState;
}

bool userSettings::getShowTrackers() const {
    return m_userShowTrackers;
}

vr::ETrackedControllerRole userSettings::getSavedRole() const {
    return m_userSavedRole;
}

vr::HmdMatrix34_t userSettings::getMatrix() const {
    return m_userMatrix;
}

userSettings::colorBlindType userSettings::getColorBlindness() const {
    return m_userColorblindness;
}

float userSettings::getNotificationVolume() const {
    return m_userNotificationVolume;
}

bool userSettings::getShowNotifications() const {
    return m_userShowNotifications;
}


