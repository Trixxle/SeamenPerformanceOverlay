#include "dashboardui.h"
#include "ui_dashboardui.h"

dashboardui::dashboardui(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::dashboardui)
{
    ui->setupUi(this);
}

dashboardui::~dashboardui()
{
    delete ui;
}
