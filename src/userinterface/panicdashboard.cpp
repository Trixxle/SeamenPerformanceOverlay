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

#include <QApplication>
#include <QPushButton>

#include "ui_panicDashboard.h"


panicDashboard::panicDashboard(QWidget *parent) : QWidget(parent), ui(new Ui::panicDashboard) {
    ui->setupUi(this);
    this->setWindowFlags(Qt::FramelessWindowHint);
    this->setAttribute(Qt::WA_TranslucentBackground);
    connect(ui->panicButton, &QPushButton::clicked, this, &panicDashboard::panicButtonClicked);
    connect(ui->panicQuitButton, &QPushButton::clicked, this, &QCoreApplication::quit);
    connect(ui->distanceFadeCheck, &QCheckBox::toggled, this, &panicDashboard::distanceCheckboxToggled);
}

void panicDashboard::setDistanceFadeChecked(bool checked) {
    ui->distanceFadeCheck->setChecked(checked);
}

panicDashboard::~panicDashboard() {
    delete ui;
}