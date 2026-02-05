//
// Created by jornt on 03/02/2026.
//

#include "dashboardui.h"
#include "ui_dashboardui.h"


DashboardUI::DashboardUI(QWidget *parent) : QWidget(parent), ui(new Ui::DashboardUI) {
    ui->setupUi(this);
}

DashboardUI::~DashboardUI() {
    delete ui;
}