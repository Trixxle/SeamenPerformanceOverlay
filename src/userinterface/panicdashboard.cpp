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

    restoreDistanceFadeState();
}

void panicDashboard::setOpacityValue(float newOpacity) {
    float opacityPercent = newOpacity * 100.0;
    ui->OpacityVar->setText(QString::number(opacityPercent) + "%");
}

void panicDashboard::setScaleValue(float newScale) {
    ui->ScaleVar->setText(QString::number(newScale));
}

void panicDashboard::setDistanceFadeValue(float newDistanceFadeValue) {
    ui->DistanceFadeVar->setText(QString::number(newDistanceFadeValue));
}

void panicDashboard::restoreDistanceFadeState() {
    bool savedSetting = false;
    if (!m_settings.value("DistanceFadeOn", savedSetting).isNull()) {
            ui->distanceFadeCheck->setChecked(m_settings.value("DistanceFadeOn", savedSetting).toBool());
    }
}

void panicDashboard::saveDistanceFadeState(bool state) {
    m_settings.setValue("DistanceFadeOn", state);
}

void panicDashboard::setDistanceFadeChecked(bool checked) {
    ui->distanceFadeCheck->setChecked(checked);
}

panicDashboard::~panicDashboard() {
    delete ui;
}