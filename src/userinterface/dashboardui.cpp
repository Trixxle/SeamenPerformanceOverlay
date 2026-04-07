//
// Created by jornt on 03/02/2026.
//

#include "dashboardui.h"
#include "ui_dashboardui.h"

DashboardUI::DashboardUI(QWidget *parent):
    QWidget(parent),
    ui(new Ui::DashboardUI),
    m_axisXTotalFrameTime(new QBarCategoryAxis()),
    m_chartViewTotalFrameTime(new QChartView())
{
    ui->setupUi(this);

    for (int i = 0; i < MAX_GRAPH_POINTS; ++i) {
        m_categoriesTotalFrameTime.append(QString::number(i));
    }

    m_axisXTotalFrameTime->append(m_categoriesTotalFrameTime);
    m_axisXTotalFrameTime->setVisible(false); // This hides the labels

    m_barSetTotalFrameTime = new QBarSet("Total Frame Time");
    m_seriesTotalFrameTime = new QBarSeries();
    m_seriesTotalFrameTime->append(m_barSetTotalFrameTime);

    m_chartTotalFrameTime = new QChart();
    m_chartTotalFrameTime->addSeries(m_seriesTotalFrameTime);
    m_chartTotalFrameTime->addAxis(m_axisXTotalFrameTime, Qt::AlignBottom);
    m_seriesTotalFrameTime->attachAxis(m_axisXTotalFrameTime);

    m_chartTotalFrameTime->legend()->hide();
    m_chartTotalFrameTime->setAnimationOptions(QChart::NoAnimation); // Improves performance

    m_axisYTotalFrameTime = new QValueAxis();
    m_axisYTotalFrameTime->setRange(0, 100);
    m_axisYTotalFrameTime->setTitleText("ms");
    m_chartTotalFrameTime->addAxis(m_axisYTotalFrameTime, Qt::AlignLeft);
    m_seriesTotalFrameTime->attachAxis(m_axisYTotalFrameTime);

    // Initialize with empty data
    for(int i = 0; i < MAX_GRAPH_POINTS; ++i) *m_barSetTotalFrameTime << 0;

    m_chartViewTotalFrameTime->setChart(m_chartTotalFrameTime);
    m_chartViewTotalFrameTime->setRenderHint(QPainter::Antialiasing);

    m_chartViewTotalFrameTime->setParent(this);
    m_chartViewTotalFrameTime->setGeometry(10, 220, 340, 150);
    this->resize(360, 380); // Expand the widget to fit the graph
}

DashboardUI::~DashboardUI() {
    delete ui;
}

void DashboardUI::updateTotalFrameTimeGraph(float newFrameTimeMs) {
    // Remove the oldest bar (at index 0) and add the new one
    m_barSetTotalFrameTime->remove(0, 1);
    *m_barSetTotalFrameTime << newFrameTimeMs;
}

void DashboardUI::updateGpuFrameTimeGraph(float newFrameTimeMs) {
    // Remove the oldest bar (at index 0) and add the new one
    m_barSetGpuFrameTime->remove(0, 1);
    *m_barSetGpuFrameTime << newFrameTimeMs;
}

void DashboardUI::updateCpuFrameTimeGraph(float newFrameTimeMs) {
    // Remove the oldest bar (at index 0) and add the new one
    m_barSetCpuFrameTime->remove(0, 1);
    *m_barSetCpuFrameTime << newFrameTimeMs;
}

void DashboardUI::setDashboardFrameRate(float framerate) {
    ui->frameRateLabel->setText(QString::number(framerate));
}

void DashboardUI::setDashboardFrameTime(float frametime) {
    ui->frameTimeLabel->setText(QString::number(frametime));
}

void DashboardUI::setDashboardCpuFrameTime(float cpuFrameTime) {
    ui->cpuFrameTimeLabel->setText(QString::number(cpuFrameTime));
}

void DashboardUI::setDashboardGpuFrameTime(float gpuFrameTime) {
    ui->gpuFrameTimeLabel->setText(QString::number(gpuFrameTime));
}

void DashboardUI::setDashboardHeadsetRefreshRate(float headsetRefreshRate) {
    ui->headsetRefreshRateLabel->setText(QString::number(headsetRefreshRate));
}

void DashboardUI::setDashboardTargetFrameRate(float targetFrameRate) {
    ui->targetFrameRateLabel->setText(QString::number(targetFrameRate));
}

// TODO: Delete this but check if it is used anywhere first
Ui::DashboardUI *DashboardUI::getUi() const {
    return ui;
}
