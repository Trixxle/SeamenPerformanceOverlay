//
// Created by jornt on 03/02/2026.
//

#include "dashboardui.h"
#include "ui_dashboardui.h"

DashboardUI::DashboardUI(QWidget *parent):
    QWidget(parent),
    ui(new Ui::DashboardUI)
{
    ui->setupUi(this);
}

DashboardUI::~DashboardUI() {
    delete ui;
}

void DashboardUI::setGpuFrameRate(float frametime) {
    ui->frameRateLabel->setText(QString::number(frametime));
}

// TODO: Delete this but check if it is used anywhere first
Ui::DashboardUI *DashboardUI::getUi() const {
    return ui;
}
