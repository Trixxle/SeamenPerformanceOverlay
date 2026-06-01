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

#include "panicdashboard.h"
#include "ui_panicDashboard.h"
#include "steamvrcontrol/steamvrlogic.h"


panicDashboard::panicDashboard(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::panicDashboard),
    m_settings("Seamen", "PerformanceOverlay")
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::FramelessWindowHint);
    this->setAttribute(Qt::WA_TranslucentBackground);

    connect(ui->panicButton, &QPushButton::clicked, this, &panicDashboard::panicButtonClicked);
    connect(ui->panicQuitButton, &QPushButton::clicked, this, &QCoreApplication::quit);
    connect(ui->distanceFadeCheck, &QCheckBox::toggled, this, &panicDashboard::distanceCheckboxToggled);
    connect(ui->PlusScale, &QPushButton::clicked, this, &panicDashboard::requestScaleUp);
    connect(ui->MinusScale, &QPushButton::clicked, this, &panicDashboard::requestScaleDown);
    connect(ui->PlusOpacity, &QPushButton::clicked, this, &panicDashboard::requestOpacityUp);
    connect(ui->MinusOpacity, &QPushButton::clicked, this, &panicDashboard::requestOpacityDown);
    connect(ui->PlusDistanceFade, &QPushButton::clicked, this, &panicDashboard::requestDistanceFadeStartUp);
    connect(ui->MinusDistanceFade, &QPushButton::clicked, this, &panicDashboard::requestDistanceFadeStartDown);
    connect(ui->RightControllerButton, &QPushButton::clicked, this, &panicDashboard::requestRightControllerAttach);
    connect(ui->LeftControllerButton, &QPushButton::clicked, this, &panicDashboard::requestLeftControllerAttach);
    connect(ui->HMDButton, &QPushButton::clicked, this, &panicDashboard::requestHmdAttach);
    connect(ui->resetPositionButton, &QPushButton::clicked, this, &panicDashboard::requestResetPosition);

    restoreDistanceFadeState();
    restoreScale();
    restoreOpacity();
    restoreDistanceFadeValue();
}

void panicDashboard::restoreScale() {
    if (!m_settings.value("Size").isNull()) {
        setScaleValue(m_settings.value("Size").toFloat());
    }
    else setScaleValue(0.2);
}

void panicDashboard::restoreOpacity() {
    if (!m_settings.value("Opacity").isNull()) {
        setOpacityValue(m_settings.value("Opacity").toFloat());
    }
    else setOpacityValue(1);
}

void panicDashboard::restoreDistanceFadeValue() {
    if (!m_settings.value("DistanceFadeStart").isNull()) {
        setDistanceFadeValue(m_settings.value("DistanceFadeStart").toFloat());
    }
    else setDistanceFadeValue(0.4);
}

void panicDashboard::restoreDistanceFadeState() {
    if (!m_settings.value("DistanceFadeOn").isNull()) {
        ui->distanceFadeCheck->setChecked(m_settings.value("DistanceFadeOn").toBool());
    }
    else setDistanceFadeState(false);
}

void panicDashboard::setOpacityValue(float newOpacity) {
    int opacityPercent = qRound( newOpacity * 100.0f);
    ui->OpacityVar->setText(QString::number(opacityPercent) + "%");
}

void panicDashboard::setScaleValue(float newScale) {
    ui->ScaleVar->setText(QString::number(newScale, 'f', 2));
}

void panicDashboard::setDistanceFadeValue(float newDistanceFadeValue) {
    float distanceInCms = newDistanceFadeValue * 100.0;
    ui->DistanceFadeVar->setText(QString::number(distanceInCms) + "cm");
}

void panicDashboard::saveDistanceFadeState(bool state) {
    m_settings.setValue("DistanceFadeOn", state);
}

void panicDashboard::setDistanceFadeState(bool checked) {
    ui->distanceFadeCheck->setChecked(checked);
}

panicDashboard::~panicDashboard() {
    delete ui;
}