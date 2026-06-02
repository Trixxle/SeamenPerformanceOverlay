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
    connect(ui->panicButton, &QPushButton::clicked, this, &panicDashboard::requestResetPosition);
    connect(ui->panicQuitButton, &QPushButton::clicked, this, &QCoreApplication::quit);
    connect(ui->distanceFadeCheck, &QCheckBox::toggled, &userSettings::instance(), &userSettings::setDistanceFadeState);
    connect(ui->showTrackersCheckbox, &QCheckBox::toggled, &userSettings::instance(), &userSettings::setShowTrackers);
    connect(ui->PlusScale, &QPushButton::clicked, &userSettings::instance(), &userSettings::increaseSize);
    connect(ui->MinusScale, &QPushButton::clicked, &userSettings::instance(), &userSettings::decreaseSize);
    connect(ui->PlusOpacity, &QPushButton::clicked, &userSettings::instance(), &userSettings::increaseOpacity);
    connect(ui->MinusOpacity, &QPushButton::clicked, &userSettings::instance(), &userSettings::decreaseOpacity);
    connect(ui->PlusDistanceFade, &QPushButton::clicked, &userSettings::instance(), &userSettings::increaseDistanceFadeValue);
    connect(ui->MinusDistanceFade, &QPushButton::clicked, &userSettings::instance(), &userSettings::decreaseDistanceFadeValue);
    connect(ui->RightControllerButton, &QPushButton::clicked, this, &panicDashboard::requestRightControllerAttach);
    connect(ui->LeftControllerButton, &QPushButton::clicked, this, &panicDashboard::requestLeftControllerAttach);
    connect(ui->HMDButton, &QPushButton::clicked, this, &panicDashboard::requestHmdAttach);
    connect(ui->resetPositionButton, &QPushButton::clicked, this, &panicDashboard::requestResetPosition);
    connect(ui->noneColorblindOption, &QRadioButton::toggled, this, [](bool checked){
        if(checked) {
            userSettings::instance().setColorblindness(userSettings::colorBlindType::none);
        }
    });
    connect(ui->protanopiaColorblindOption, &QRadioButton::toggled, this, [](bool checked){
        if(checked) {
            userSettings::instance().setColorblindness(userSettings::colorBlindType::protanopia);
        }
    });
    connect(ui->deuteranopiaColorblindOption, &QRadioButton::toggled, this, [](bool checked){
        if(checked) {
            userSettings::instance().setColorblindness(userSettings::colorBlindType::deuteranopia);
        }
    });
    connect(ui->tritanopiaColorblindOption, &QRadioButton::toggled, this, [](bool checked){
        if(checked) {
            userSettings::instance().setColorblindness(userSettings::colorBlindType::tritanopia);
        }
    });
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
    float distanceInCms = userSettings::instance().getDistanceFadeValue() * 100.0f;
    ui->DistanceFadeVar->setText(QString::number(distanceInCms) + "cm");
}

void panicDashboard::updateDistanceFadeState() const {
    ui->distanceFadeCheck->setChecked(userSettings::instance().getDistanceFadeState());
}

void panicDashboard::updateShowTrackerState() const {
    ui->showTrackersCheckbox->setChecked(userSettings::instance().getShowTrackers());
}

void panicDashboard::updateColorblindness() const {
    // Block signals for all relevant radio buttons while updating
    const QSignalBlocker blockerNone(ui->noneColorblindOption);
    const QSignalBlocker blockerPro(ui->protanopiaColorblindOption);
    const QSignalBlocker blockerDeu(ui->deuteranopiaColorblindOption);
    const QSignalBlocker blockerTri(ui->tritanopiaColorblindOption);

    switch(userSettings::instance().getColorBlindness()) {
        case userSettings::colorBlindType::none: {
            ui->noneColorblindOption->setChecked(true);
        }
            break;
        case userSettings::colorBlindType::protanopia: {
            ui->protanopiaColorblindOption->setChecked(true);
        }
            break;
        case userSettings::colorBlindType::deuteranopia: {
            ui->deuteranopiaColorblindOption->setChecked(true);
        }
            break;
        case userSettings::colorBlindType::tritanopia: {
            ui->tritanopiaColorblindOption->setChecked(true);
        }
            break;
        default: {}
    }
}

panicDashboard::~panicDashboard() {
    delete ui;
}