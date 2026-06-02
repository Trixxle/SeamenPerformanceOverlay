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

panicDashboard::panicDashboard(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::panicDashboard)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::FramelessWindowHint);
    this->setAttribute(Qt::WA_TranslucentBackground);

    connect(ui->panicButton, &QPushButton::clicked, this, &panicDashboard::resetValues);
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
}

void panicDashboard::resetValues() {
    userSettings::instance().setMatrix({
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.866f, 0.5f, 0.1f,
    0.0f, -0.5f, 0.866f, -0.08f
    });
    userSettings::instance().setOpacity(1.0f);
    userSettings::instance().setSize(0.2f);
    userSettings::instance().setDistanceFadeValue(0.4f);

    userSettings::instance().setColorblindness(userSettings::colorBlindType::none);

    userSettings::instance().setDistanceFadeState(false);

    userSettings::instance().setShowTrackers(true);

    userSettings::instance().setSavedRole(vr::TrackedControllerRole_LeftHand);
}


void panicDashboard::updateOpacityValue() const {
    int opacityPercent = qRound( userSettings::instance().getOpacity() * 100.0f);
    ui->OpacityVar->setText(QString::number(opacityPercent) + "%");
}

void panicDashboard::updateScaleValue() const {
    ui->ScaleVar->setText(QString::number(userSettings::instance().getSize(), 'f', 2));
}

void panicDashboard::updateDistanceFadeValue() const {
    float distanceInCms = userSettings::instance().getDistanceFadeValue() * 100.0;
    ui->DistanceFadeVar->setText(QString::number(distanceInCms) + "cm");
}

void panicDashboard::updateDistanceFadeState() const {
    ui->distanceFadeCheck->setChecked(userSettings::instance().getDistanceFadeState());
}

panicDashboard::~panicDashboard() {
    delete ui;
}