//
// Created by jornt on 03/02/2026.
//

#include "dashboardui.h"
#include "ui_dashboardui.h"

DashboardUI::DashboardUI(float headsetRefreshRate, float targetFrameRate, QWidget *parent):
    QWidget(parent),
    ui(new Ui::DashboardUI),
    m_axisXTotalFrameTime(new QBarCategoryAxis()),
    m_chartViewTotalFrameTime(new QChartView()),

    m_axisXCpuFrameTime(new QBarCategoryAxis()),
    m_chartViewCpuFrameTime(new QChartView()),

    m_axisXGpuFrameTime(new QBarCategoryAxis()),
    m_chartViewGpuFrameTime(new QChartView()),

    m_axisXFrameRate(new QBarCategoryAxis()),
    m_chartViewFrameRate(new QChartView()),

    m_targetFrameRate(targetFrameRate),
    m_headsetRefreshRate(headsetRefreshRate)
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_StyledBackground, true);

    ui->setupUi(this);

    this->setObjectName("DashboardUI");

    this->setStyleSheet("#DashboardUI { "
                            "background-color: #2D2D2D; " // Set your desired background
                            "border-radius: 15px; "       // Adjust radius here
                            "}"
                            );

    setUpCharts();

    //this->resize(800, 1000); // Expands the widget to fit the graph

    ui->mainGridLayout->setSizeConstraint(QLayout::SetMinimumSize); // Forces the existing layout to stretch to the whole window size
    ui->mainGridLayout->setAlignment(Qt::AlignCenter);
    ui->mainGridLayout->setContentsMargins(15, 15, 15, 15);
}

DashboardUI::~DashboardUI() {
    delete ui;
    delete m_axisXTotalFrameTime;
    delete m_chartViewTotalFrameTime;
    delete m_axisXCpuFrameTime;
    delete m_chartViewCpuFrameTime;
    delete m_axisXGpuFrameTime;
    delete m_chartViewGpuFrameTime;
    delete m_axisXFrameRate;
    delete m_chartViewFrameRate;
}

// This is required as for some reason Qt is refusing to atumatocally apply the underlying QPaintEvent function
void DashboardUI::paintEvent(QPaintEvent *event) {
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void DashboardUI::insertWidgetAtRow(QGridLayout* layout, QWidget* newWidget, int targetRow, int targetColumn, bool toShiftDown) {
    newWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    newWidget->setMinimumHeight(250);
    newWidget->setMaximumHeight(250);

    if (!toShiftDown) {
        layout->addWidget(newWidget, targetRow, targetColumn);
        return;
    }
    // Temporary structure to hold the items
    struct ShiftItem {
        QLayoutItem* item;
        int r, c, rs, cs;
    };
    QList<ShiftItem> itemsToShift;

    // Collect all the items that need shifting
    for (int i = 0; i < layout->count(); ++i) {
        int r, c, rs, cs;
        layout->getItemPosition(i, &r, &c, &rs, &cs);

        if (r >= targetRow) {
            itemsToShift.append({layout->itemAt(i), r, c, rs, cs});
        }
    }

    // Remove the collected items
    for (const ShiftItem& si : itemsToShift) {
        layout->removeItem(si.item);
    }

    // Re-add the collected items but shift them
    for (const ShiftItem& si : itemsToShift) {
        layout->addItem(si.item, si.r + 1, si.c, si.rs, si.cs);
    }

    // Add the widget in the row that is now empty
    layout->addWidget(newWidget, targetRow, targetColumn);
}

void DashboardUI::updateTotalFrameTimeGraph(float newFrameTimeMs) {
    m_barSetTotalFrameTimeFast->remove(0, 1);
    m_barSetTotalFrameTimeSlow->remove(0, 1);
    m_barSetTotalFrameTimeMedium->remove(0, 1);

    // Frame time is very high
    if (newFrameTimeMs > m_headsetRefreshRate + 3.0) {
        *m_barSetTotalFrameTimeMedium << 0.0f;
        *m_barSetTotalFrameTimeFast << 0.0f;
        *m_barSetTotalFrameTimeSlow << newFrameTimeMs;
    }
    // Frame time is slightly higher than ideal
    else if (newFrameTimeMs > m_headsetRefreshRate + 0.5) {
        *m_barSetTotalFrameTimeFast << 0.0f;
        *m_barSetTotalFrameTimeSlow << 0.0f;
        *m_barSetTotalFrameTimeMedium << newFrameTimeMs;
    }
    // Frame time is under the maximum value
    else {
        *m_barSetTotalFrameTimeSlow << 0.0f;
        *m_barSetTotalFrameTimeMedium << 0.0f;
        *m_barSetTotalFrameTimeFast << newFrameTimeMs;
    }
}

void DashboardUI::updateGpuFrameTimeGraph(float newFrameTimeMs) {
    m_barSetGpuFrameTimeFast->remove(0, 1);
    m_barSetGpuFrameTimeSlow->remove(0, 1);
    m_barSetGpuFrameTimeMedium->remove(0, 1);

    if (newFrameTimeMs > m_headsetRefreshRate + 3.0) {
        *m_barSetGpuFrameTimeMedium << 0.0f;
        *m_barSetGpuFrameTimeFast << 0.0f;
        *m_barSetGpuFrameTimeSlow << newFrameTimeMs;
    }
    else if (newFrameTimeMs > m_headsetRefreshRate + 0.5) {
        *m_barSetGpuFrameTimeFast << 0.0f;
        *m_barSetGpuFrameTimeSlow << 0.0f;
        *m_barSetGpuFrameTimeMedium << newFrameTimeMs;
    }
    else {
        *m_barSetGpuFrameTimeSlow << 0.0f;
        *m_barSetGpuFrameTimeMedium << 0.0f;
        *m_barSetGpuFrameTimeFast << newFrameTimeMs;
    }
}

void DashboardUI::updateCpuFrameTimeGraph(float newFrameTimeMs) {
    m_barSetCpuFrameTimeFast->remove(0, 1);
    m_barSetCpuFrameTimeSlow->remove(0, 1);
    m_barSetCpuFrameTimeMedium->remove(0, 1);

    if (newFrameTimeMs > m_headsetRefreshRate + 3.0) {
        *m_barSetCpuFrameTimeMedium << 0.0f;
        *m_barSetCpuFrameTimeFast << 0.0f;
        *m_barSetCpuFrameTimeSlow << newFrameTimeMs;

    }
    else if (newFrameTimeMs > m_headsetRefreshRate + 0.5) {
        *m_barSetCpuFrameTimeFast << 0.0f;
        *m_barSetCpuFrameTimeSlow << 0.0f;
        *m_barSetCpuFrameTimeMedium << newFrameTimeMs;
    }
    else {
        *m_barSetCpuFrameTimeSlow << 0.0f;
        *m_barSetCpuFrameTimeMedium << 0.0f;
        *m_barSetCpuFrameTimeFast << newFrameTimeMs;
    }
}

void DashboardUI::updateFrameRateGraph(float newFrameRate) {
    m_barSetFrameRateFast->remove(0, 1);
    m_barSetFrameRateSlow->remove(0, 1);
    m_barSetFrameRateMedium->remove(0, 1);

    // Frame rate is very low
    if (newFrameRate < m_targetFrameRate - 10.0) {
        *m_barSetFrameRateMedium << 0.0f;
        *m_barSetFrameRateFast << 0.0f;
        *m_barSetFrameRateSlow << newFrameRate;
    }
    // Frame rate is slightly under the target
    else if (newFrameRate < m_targetFrameRate - 1) {
        *m_barSetFrameRateFast << 0.0f;
        *m_barSetFrameRateSlow << 0.0f;
        *m_barSetFrameRateMedium << newFrameRate;
    }
    // Frame rate is expected value
    else {
        *m_barSetFrameRateSlow << 0.0f;
        *m_barSetFrameRateMedium << 0.0f;
        *m_barSetFrameRateFast << newFrameRate;
    }
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

void DashboardUI::setUpCharts() {
    for (int i = 0; i < MAX_GRAPH_POINTS; ++i) {
        m_categoriesTotalFrameTime.append(QString::number(i));
        m_categoriesCpuFrameTime.append(QString::number(i));
        m_categoriesGpuFrameTime.append(QString::number(i));
        m_categoriesFrameRate.append(QString::number(i));
    }

    m_axisXTotalFrameTime->append(m_categoriesTotalFrameTime);
    m_axisXTotalFrameTime->setVisible(false); // This hides the labels

    m_axisXCpuFrameTime->append(m_categoriesTotalFrameTime);
    m_axisXCpuFrameTime->setVisible(false);

    m_axisXGpuFrameTime->append(m_categoriesTotalFrameTime);
    m_axisXGpuFrameTime->setVisible(false);

    m_axisXFrameRate->append(m_categoriesTotalFrameTime);
    m_axisXFrameRate->setVisible(false);

    m_barSetTotalFrameTimeFast = new QBarSet("NormalTOTAL");
    m_barSetTotalFrameTimeFast->setColor(QColorConstants::Svg::lightblue);
    m_barSetTotalFrameTimeFast->setBorderColor(Qt::transparent);

    m_barSetTotalFrameTimeMedium = new QBarSet("MediumTOTAL");
    m_barSetTotalFrameTimeMedium->setColor(QColorConstants::Svg::orange);
    m_barSetTotalFrameTimeMedium->setBorderColor(Qt::transparent);

    m_barSetTotalFrameTimeSlow = new QBarSet("SlowTOTAL");
    m_barSetTotalFrameTimeSlow->setColor(QColorConstants::Svg::red);
    m_barSetTotalFrameTimeSlow->setBorderColor(Qt::transparent);

    m_seriesTotalFrameTime = new QStackedBarSeries();
    m_seriesTotalFrameTime->append(m_barSetTotalFrameTimeFast);
    m_seriesTotalFrameTime->append(m_barSetTotalFrameTimeSlow);
    m_seriesTotalFrameTime->append(m_barSetTotalFrameTimeMedium);

    m_barSetCpuFrameTimeFast = new QBarSet("NormalCPU");
    m_barSetCpuFrameTimeFast->setColor(QColorConstants::Svg::lightblue);
    m_barSetCpuFrameTimeFast->setBorderColor(Qt::transparent);

    m_barSetCpuFrameTimeMedium = new QBarSet("MediumCPU");
    m_barSetCpuFrameTimeMedium->setColor(QColorConstants::Svg::orange);
    m_barSetCpuFrameTimeMedium->setBorderColor(Qt::transparent);

    m_barSetCpuFrameTimeSlow = new QBarSet("SlowCPU");
    m_barSetCpuFrameTimeSlow->setColor(QColorConstants::Svg::red);
    m_barSetCpuFrameTimeSlow->setBorderColor(Qt::transparent);

    m_seriesCpuFrameTime = new QStackedBarSeries();
    m_seriesCpuFrameTime->append(m_barSetCpuFrameTimeFast);
    m_seriesCpuFrameTime->append(m_barSetCpuFrameTimeSlow);
    m_seriesCpuFrameTime->append(m_barSetCpuFrameTimeMedium);

    m_barSetGpuFrameTimeFast = new QBarSet("NormalGPU");
    m_barSetGpuFrameTimeFast->setColor(QColorConstants::Svg::lightblue);
    m_barSetGpuFrameTimeFast->setBorderColor(Qt::transparent);

    m_barSetGpuFrameTimeMedium = new QBarSet("MediumGPU");
    m_barSetGpuFrameTimeMedium->setColor(QColorConstants::Svg::orange);
    m_barSetGpuFrameTimeMedium->setBorderColor(Qt::transparent);

    m_barSetGpuFrameTimeSlow = new QBarSet("SlowGPU");
    m_barSetGpuFrameTimeSlow->setColor(QColorConstants::Svg::red);
    m_barSetGpuFrameTimeSlow->setBorderColor(Qt::transparent);

    m_seriesGpuFrameTime = new QStackedBarSeries();
    m_seriesGpuFrameTime->append(m_barSetGpuFrameTimeFast);
    m_seriesGpuFrameTime->append(m_barSetGpuFrameTimeSlow);
    m_seriesGpuFrameTime->append(m_barSetGpuFrameTimeMedium);

    m_barSetFrameRateFast = new QBarSet("NormalFPS");
    m_barSetFrameRateFast->setColor(QColorConstants::Svg::lightblue);
    m_barSetFrameRateFast->setBorderColor(Qt::transparent);

    m_barSetFrameRateMedium = new QBarSet("MediumFPS");
    m_barSetFrameRateMedium->setColor(QColorConstants::Svg::orange);
    m_barSetFrameRateMedium->setBorderColor(Qt::transparent);

    m_barSetFrameRateSlow = new QBarSet("SlowFPS");
    m_barSetFrameRateSlow->setColor(QColorConstants::Svg::red);
    m_barSetFrameRateSlow->setBorderColor(Qt::transparent);

    m_seriesFrameRate = new QStackedBarSeries();
    m_seriesFrameRate->append(m_barSetFrameRateFast);
    m_seriesFrameRate->append(m_barSetFrameRateSlow);
    m_seriesFrameRate->append(m_barSetFrameRateMedium);

    m_chartTotalFrameTime = new QChart();
    m_chartTotalFrameTime->addSeries(m_seriesTotalFrameTime);
    m_chartTotalFrameTime->addAxis(m_axisXTotalFrameTime, Qt::AlignBottom);
    m_seriesTotalFrameTime->attachAxis(m_axisXTotalFrameTime);

    m_chartCpuFrameTime = new QChart();
    m_chartCpuFrameTime->addSeries(m_seriesCpuFrameTime);
    m_chartCpuFrameTime->addAxis(m_axisXCpuFrameTime, Qt::AlignBottom);
    m_seriesCpuFrameTime->attachAxis(m_axisXCpuFrameTime);

    m_chartGpuFrameTime = new QChart();
    m_chartGpuFrameTime->addSeries(m_seriesGpuFrameTime);
    m_chartGpuFrameTime->addAxis(m_axisXGpuFrameTime, Qt::AlignBottom);
    m_seriesGpuFrameTime->attachAxis(m_axisXGpuFrameTime);

    m_chartFrameRate = new QChart();
    m_chartFrameRate->addSeries(m_seriesFrameRate);
    m_chartFrameRate->addAxis(m_axisXFrameRate, Qt::AlignBottom);
    m_seriesFrameRate->attachAxis(m_axisXFrameRate);

    m_chartTotalFrameTime->legend()->hide();
    m_chartTotalFrameTime->setAnimationOptions(QChart::NoAnimation); // Improves performance

    m_chartCpuFrameTime->legend()->hide();
    m_chartCpuFrameTime->setAnimationOptions(QChart::NoAnimation); // Improves performance

    m_chartGpuFrameTime->legend()->hide();
    m_chartGpuFrameTime->setAnimationOptions(QChart::NoAnimation); // Improves performance

    m_chartFrameRate->legend()->hide();
    m_chartFrameRate->setAnimationOptions(QChart::NoAnimation); // Improves performance

    m_axisYTotalFrameTime = new QValueAxis();
    m_axisYTotalFrameTime->setRange(0, 50);
    m_axisYTotalFrameTime->setTitleText("ms");
    m_chartTotalFrameTime->addAxis(m_axisYTotalFrameTime, Qt::AlignLeft);
    m_seriesTotalFrameTime->attachAxis(m_axisYTotalFrameTime);

    m_axisYCpuFrameTime = new QValueAxis();
    m_axisYCpuFrameTime->setRange(0, 50);
    m_axisYCpuFrameTime->setTitleText("ms");
    m_chartCpuFrameTime->addAxis(m_axisYCpuFrameTime, Qt::AlignLeft);
    m_seriesCpuFrameTime->attachAxis(m_axisYCpuFrameTime);

    m_axisYGpuFrameTime = new QValueAxis();
    m_axisYGpuFrameTime->setRange(0, 50);
    m_axisYGpuFrameTime->setTitleText("ms");
    m_chartGpuFrameTime->addAxis(m_axisYGpuFrameTime, Qt::AlignLeft);
    m_seriesGpuFrameTime->attachAxis(m_axisYGpuFrameTime);

    m_axisYFrameRate = new QValueAxis();
    m_axisYFrameRate->setRange(0, m_targetFrameRate + 10);
    m_axisYFrameRate->setTitleText("FPS");
    m_chartFrameRate->addAxis(m_axisYFrameRate, Qt::AlignLeft);
    m_seriesFrameRate->attachAxis(m_axisYFrameRate);

    // Initialize with empty data
    for(int i = 0; i < MAX_GRAPH_POINTS; ++i) {
        *m_barSetTotalFrameTimeFast << 0;
        *m_barSetTotalFrameTimeMedium << 0;
        *m_barSetTotalFrameTimeSlow << 0;
        *m_barSetCpuFrameTimeFast << 0;
        *m_barSetCpuFrameTimeMedium << 0;
        *m_barSetCpuFrameTimeSlow << 0;
        *m_barSetGpuFrameTimeFast << 0;
        *m_barSetGpuFrameTimeMedium << 0;
        *m_barSetGpuFrameTimeSlow << 0;
        *m_barSetFrameRateFast << 0;
        *m_barSetFrameRateMedium << 0;
        *m_barSetFrameRateSlow << 0;
    }

    m_chartViewTotalFrameTime->setChart(m_chartTotalFrameTime);
    m_chartViewTotalFrameTime->setRenderHint(QPainter::Antialiasing);
    m_chartViewTotalFrameTime->setObjectName("chartViewTotalFrameTime");
    m_chartViewTotalFrameTime->setStyleSheet("#chartViewTotalFrameTime { "
                        "background-color: #2D2D2D; "
                        "}"
                        );

    m_chartViewCpuFrameTime->setChart(m_chartCpuFrameTime);
    m_chartViewCpuFrameTime->setRenderHint(QPainter::Antialiasing);
    m_chartViewCpuFrameTime->setObjectName("chartViewCpuFrameTime");
    m_chartViewCpuFrameTime->setStyleSheet("#chartViewCpuFrameTime { "
                        "background-color: #2D2D2D; "
                        "}"
                        );

    m_chartViewGpuFrameTime->setChart(m_chartGpuFrameTime);
    m_chartViewGpuFrameTime->setRenderHint(QPainter::Antialiasing);
    m_chartViewGpuFrameTime->setObjectName("chartViewGpuFrameTime");
    m_chartViewGpuFrameTime->setStyleSheet("#chartViewGpuFrameTime { "
                        "background-color: #2D2D2D; "
                        "}"
                        );

    m_chartViewFrameRate->setChart(m_chartFrameRate);
    m_chartViewFrameRate->setRenderHint(QPainter::Antialiasing);
    m_chartViewFrameRate->setObjectName("chartViewFrameRate");
    m_chartViewFrameRate->setStyleSheet("#chartViewFrameRate { "
                        "background-color: #2D2D2D; "
                        "}"
                        );

    m_chartViewCpuFrameTime->setParent(this);
    insertWidgetAtRow(ui->mainGridLayout, m_chartViewCpuFrameTime, 0, 1, true);

    m_chartViewGpuFrameTime->setParent(this);
    insertWidgetAtRow(ui->mainGridLayout, m_chartViewGpuFrameTime, 0,0, false);

    m_chartViewTotalFrameTime->setParent(this);
    insertWidgetAtRow(ui->mainGridLayout, m_chartViewTotalFrameTime, 2, 0, true);

    m_chartViewFrameRate->setParent(this);
    insertWidgetAtRow(ui->mainGridLayout, m_chartViewFrameRate, 2, 1, false);
}
