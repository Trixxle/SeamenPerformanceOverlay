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
    this->setWindowFlags(Qt::FramelessWindowHint);
    this->setAttribute(Qt::WA_TranslucentBackground);
    //this->setStyleSheet("background: transparent;");

    ui->setupUi(this);

    QString exitButtonStyle = R"(
    QPushButton {
        border: 2px solid #1092BF;
        border-radius: 10px;
        background-color: #1A1A1A;
        color: white;
        font-size: 15pt;
    }

    QPushButton:hover {
        background-color: #242424;
    }

    QPushButton:pressed {
        background-color: #1EAEED;
        border-color: #1EAEED;
        color: black;
    }
    )";

    ui->exitButton->setStyleSheet(exitButtonStyle);

    this->setObjectName("DashboardUI");

    this->setStyleSheet("#DashboardUI { "
                            "background-color: #232424; "
                            "border-radius: 45px; "
                            "border: 4px solid #1092BF; "
                            "margin: 4px; "
                            "}"
                            );

    setUpCharts();

    //this->resize(800, 1000); // Expands the widget to fit the graph

    connect(ui->exitButton, &QPushButton::clicked, this, &DashboardUI::onExitButtonClicked);

    ui->mainGridLayout->setSizeConstraint(QLayout::SetMinimumSize); // Forces the existing layout to stretch to the whole window size
    ui->mainGridLayout->setAlignment(Qt::AlignCenter);
    ui->mainGridLayout->setContentsMargins(15, 15, 15, 15);
}

DashboardUI::~DashboardUI() {
    delete ui;
}

void DashboardUI::onExitButtonClicked() {
    QApplication::exit();
}

void DashboardUI::updateGraphs(const FrameHandler::FrameStatsList& informationList) {

    if (informationList.isEmpty()) return;

    // After some research blocking signals here increases performance by a massive amoutn.
    // Qt will send a onSceneChanged signal for every single item inside the struct for every graph
    // Blocking the signal and only sending the last one (which is the only one that matters) decreased GPU by 5%
    m_chartTotalFrameTime->blockSignals(true);
    m_chartCpuFrameTime->blockSignals(true);
    m_chartGpuFrameTime->blockSignals(true);
    m_chartFrameRate->blockSignals(true);

    QList<float> totalTimes;
    QList<float> gpuTimes;
    QList<float> cpuTimes;
    QList<float> frameRates;

    int count = informationList.size();
    totalTimes.reserve(count);
    gpuTimes.reserve(count);
    cpuTimes.reserve(count);
    frameRates.reserve(count);

    for (const auto& information : informationList) {
        totalTimes.append(roundFloat(information.TotalFrametime, 2));
        gpuTimes.append(roundFloat(information.GpuFrametime, 2));
        cpuTimes.append(roundFloat(information.CpuFrametime, 2));
        frameRates.append(roundFloat(information.Framerate, 2));
    }

    updateTotalFrameTimeGraph(totalTimes);
    updateGpuFrameTimeGraph(gpuTimes);
    updateCpuFrameTimeGraph(cpuTimes);
    updateFrameRateGraph(frameRates);

    m_chartTotalFrameTime->blockSignals(false);
    m_chartCpuFrameTime->blockSignals(false);
    m_chartGpuFrameTime->blockSignals(false);
    m_chartFrameRate->blockSignals(false);

    // Because the signals were blocked it must now be manually forced
    m_chartTotalFrameTime->update();
    m_chartCpuFrameTime->update();
    m_chartGpuFrameTime->update();
    m_chartFrameRate->update();
}

void DashboardUI::updateLabels(const FrameHandler::frameStats &information) {
        setDashboardFrameRate(roundFloat(information.Framerate, 2));
        setDashboardFrameTime(roundFloat(information.TotalFrametime, 2));
        setDashboardCpuFrameTime(roundFloat(information.CpuFrametime, 2));
        setDashboardGpuFrameTime(roundFloat(information.GpuFrametime, 2));
        setDashboardTargetFrameRate(roundFloat(information.MaxFramerate, 2));
        setDashboardHeadsetRefreshRate(roundFloat(information.MaxFrametime, 2));
        setSmoothFrameRate(roundFloat(information.smoothFrameRate, 0));
}

float DashboardUI::roundFloat(float number, int decimalCases) {
    if (decimalCases == 0) return roundf(number);
    return roundf(number * 100) / (decimalCases * 50);
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
    newWidget->setMinimumHeight(150);
    newWidget->setMaximumHeight(150);

    // If the spot is already empty, no need to shift, simply add the widget
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

    // Collects all the items that need shifting
    for (int i = 0; i < layout->count(); ++i) {
        int r, c, rs, cs;
        layout->getItemPosition(i, &r, &c, &rs, &cs);

        if (r >= targetRow) {
            itemsToShift.append({layout->itemAt(i), r, c, rs, cs});
        }
    }

    // Removes the collected items
    for (const ShiftItem& si : itemsToShift) {
        layout->removeItem(si.item);
    }

    // Re-adds the collected items but shifts them
    for (const ShiftItem& si : itemsToShift) {
        layout->addItem(si.item, si.r + 1, si.c, si.rs, si.cs);
    }

    // Adds the widget in the row that is now empty
    layout->addWidget(newWidget, targetRow, targetColumn);
}

void DashboardUI::updateTotalFrameTimeGraph(const QList<float>& newFrameTimes) {
    int count = newFrameTimes.size();
    if (count == 0) return;

    QList<qreal> fastList, mediumList, slowList;
    fastList.reserve(count);
    mediumList.reserve(count);
    slowList.reserve(count);

    for (float newFrameTimeMs : newFrameTimes) {
        // Very slow
        if (newFrameTimeMs > m_headsetRefreshRate + 3.0) {
            mediumList.append(0.0);
            fastList.append(0.0);
            slowList.append(newFrameTimeMs);
        }
        // Slow
        else if (newFrameTimeMs > m_headsetRefreshRate) {
            fastList.append(0.0);
            slowList.append(0.0);
            mediumList.append(newFrameTimeMs);
        }
        // On time
        else {
            slowList.append(0.0);
            mediumList.append(0.0);
            fastList.append(newFrameTimeMs);
        }
    }

    m_barSetTotalFrameTimeFast->remove(0, count);
    m_barSetTotalFrameTimeMedium->remove(0, count);
    m_barSetTotalFrameTimeSlow->remove(0, count);

    m_barSetTotalFrameTimeFast->append(fastList);
    m_barSetTotalFrameTimeMedium->append(mediumList);
    m_barSetTotalFrameTimeSlow->append(slowList);
}

void DashboardUI::updateGpuFrameTimeGraph(const QList<float>& newFrameTimes) {
    int count = newFrameTimes.size();
    if (count == 0) return;

    QList<qreal> fastList, mediumList, slowList;
    fastList.reserve(count);
    mediumList.reserve(count);
    slowList.reserve(count);

    for (float newFrameTimeMs : newFrameTimes) {
        // Very slow
        if (newFrameTimeMs > m_headsetRefreshRate + 3.0) {
            mediumList.append(0.0);
            fastList.append(0.0);
            slowList.append(newFrameTimeMs);
        }
        // Slow
        else if (newFrameTimeMs > m_headsetRefreshRate) {
            fastList.append(0.0);
            slowList.append(0.0);
            mediumList.append(newFrameTimeMs);
        }
        // On time
        else {
            slowList.append(0.0);
            mediumList.append(0.0);
            fastList.append(newFrameTimeMs);
        }
    }

    m_barSetGpuFrameTimeFast->remove(0, count);
    m_barSetGpuFrameTimeMedium->remove(0, count);
    m_barSetGpuFrameTimeSlow->remove(0, count);

    m_barSetGpuFrameTimeFast->append(fastList);
    m_barSetGpuFrameTimeMedium->append(mediumList);
    m_barSetGpuFrameTimeSlow->append(slowList);
}

void DashboardUI::updateCpuFrameTimeGraph(const QList<float>& newFrameTimes) {
    int count = newFrameTimes.size();
    if (count == 0) return;

    QList<qreal> fastList, mediumList, slowList;
    fastList.reserve(count);
    mediumList.reserve(count);
    slowList.reserve(count);

    for (float newFrameTimeMs : newFrameTimes) {
        // Very slow
        if (newFrameTimeMs > m_headsetRefreshRate + 3.0) {
            mediumList.append(0.0);
            fastList.append(0.0);
            slowList.append(newFrameTimeMs);
        }
        // Slow
        else if (newFrameTimeMs > m_headsetRefreshRate) {
            fastList.append(0.0);
            slowList.append(0.0);
            mediumList.append(newFrameTimeMs);
        }
        // On time
        else {
            slowList.append(0.0);
            mediumList.append(0.0);
            fastList.append(newFrameTimeMs);
        }
    }

    m_barSetCpuFrameTimeFast->remove(0, count);
    m_barSetCpuFrameTimeMedium->remove(0, count);
    m_barSetCpuFrameTimeSlow->remove(0, count);

    m_barSetCpuFrameTimeFast->append(fastList);
    m_barSetCpuFrameTimeMedium->append(mediumList);
    m_barSetCpuFrameTimeSlow->append(slowList);
}

void DashboardUI::updateFrameRateGraph(const QList<float>& newFrameRates) {
    int count = newFrameRates.size();
    if (count == 0) return;

    QList<qreal> fastList, mediumList, slowList;
    fastList.reserve(count);
    mediumList.reserve(count);
    slowList.reserve(count);

    for (float newFrameRate : newFrameRates) {
        if (newFrameRate < m_targetFrameRate - 10.0) {
            mediumList.append(0.0f);
            fastList.append(0.0f);
            slowList.append(newFrameRate);
        }
        else if (newFrameRate < m_targetFrameRate) {
            fastList.append(0.0f);
            slowList.append(0.0f);
            mediumList.append(newFrameRate);
        }
        else {
            slowList.append(0.0f);
            mediumList.append(0.0f);
            fastList.append(newFrameRate);
        }
    }

    m_barSetFrameRateFast->remove(0, count);
    m_barSetFrameRateMedium->remove(0, count);
    m_barSetFrameRateSlow->remove(0, count);

    m_barSetFrameRateFast->append(fastList);
    m_barSetFrameRateMedium->append(mediumList);
    m_barSetFrameRateSlow->append(slowList);
}

void DashboardUI::setSmoothFrameRate(float smoothFrameRate) {
    ui->smoothFrameRateLabel->setText(QString::number(smoothFrameRate));
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
    ui->headsetRefreshRateLabel->setText("< " + QString::number(headsetRefreshRate));
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
    m_barSetTotalFrameTimeFast->setColor(QColor("#1092BF"));
    m_barSetTotalFrameTimeFast->setBorderColor(Qt::transparent);

    m_barSetTotalFrameTimeMedium = new QBarSet("MediumTOTAL");
    m_barSetTotalFrameTimeMedium->setColor(QColorConstants::Svg::orange);
    m_barSetTotalFrameTimeMedium->setBorderColor(Qt::transparent);

    m_barSetTotalFrameTimeSlow = new QBarSet("SlowTOTAL");
    m_barSetTotalFrameTimeSlow->setColor(QColorConstants::Svg::red);
    m_barSetTotalFrameTimeSlow->setBorderColor(Qt::transparent);

    m_seriesTotalFrameTime = new QStackedBarSeries();
    m_seriesTotalFrameTime->setBarWidth(1.0);
    m_seriesTotalFrameTime->append(m_barSetTotalFrameTimeFast);
    m_seriesTotalFrameTime->append(m_barSetTotalFrameTimeSlow);
    m_seriesTotalFrameTime->append(m_barSetTotalFrameTimeMedium);

    m_barSetCpuFrameTimeFast = new QBarSet("NormalCPU");
    m_barSetCpuFrameTimeFast->setColor(QColor("#1092BF"));
    m_barSetCpuFrameTimeFast->setBorderColor(Qt::transparent);

    m_barSetCpuFrameTimeMedium = new QBarSet("MediumCPU");
    m_barSetCpuFrameTimeMedium->setColor(QColorConstants::Svg::orange);
    m_barSetCpuFrameTimeMedium->setBorderColor(Qt::transparent);

    m_barSetCpuFrameTimeSlow = new QBarSet("SlowCPU");
    m_barSetCpuFrameTimeSlow->setColor(QColorConstants::Svg::red);
    m_barSetCpuFrameTimeSlow->setBorderColor(Qt::transparent);

    m_seriesCpuFrameTime = new QStackedBarSeries();
    m_seriesCpuFrameTime->setBarWidth(1.0);
    m_seriesCpuFrameTime->append(m_barSetCpuFrameTimeFast);
    m_seriesCpuFrameTime->append(m_barSetCpuFrameTimeSlow);
    m_seriesCpuFrameTime->append(m_barSetCpuFrameTimeMedium);

    m_barSetGpuFrameTimeFast = new QBarSet("NormalGPU");
    m_barSetGpuFrameTimeFast->setColor(QColor("#1092BF"));
    m_barSetGpuFrameTimeFast->setBorderColor(Qt::transparent);

    m_barSetGpuFrameTimeMedium = new QBarSet("MediumGPU");
    m_barSetGpuFrameTimeMedium->setColor(QColorConstants::Svg::orange);
    m_barSetGpuFrameTimeMedium->setBorderColor(Qt::transparent);

    m_barSetGpuFrameTimeSlow = new QBarSet("SlowGPU");
    m_barSetGpuFrameTimeSlow->setColor(QColorConstants::Svg::red);
    m_barSetGpuFrameTimeSlow->setBorderColor(Qt::transparent);

    m_seriesGpuFrameTime = new QStackedBarSeries();
    m_seriesGpuFrameTime->setBarWidth(1.0);
    m_seriesGpuFrameTime->append(m_barSetGpuFrameTimeFast);
    m_seriesGpuFrameTime->append(m_barSetGpuFrameTimeSlow);
    m_seriesGpuFrameTime->append(m_barSetGpuFrameTimeMedium);

    m_barSetFrameRateFast = new QBarSet("NormalFPS");
    m_barSetFrameRateFast->setColor(QColor("#1092BF"));
    m_barSetFrameRateFast->setBorderColor(Qt::transparent);

    m_barSetFrameRateMedium = new QBarSet("MediumFPS");
    m_barSetFrameRateMedium->setColor(QColorConstants::Svg::orange);
    m_barSetFrameRateMedium->setBorderColor(Qt::transparent);

    m_barSetFrameRateSlow = new QBarSet("SlowFPS");
    m_barSetFrameRateSlow->setColor(QColorConstants::Svg::red);
    m_barSetFrameRateSlow->setBorderColor(Qt::transparent);

    m_seriesFrameRate = new QStackedBarSeries();
    m_seriesFrameRate->setBarWidth(1.0);
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
    m_chartTotalFrameTime->setBackgroundRoundness(28);

    m_chartCpuFrameTime->legend()->hide();
    m_chartCpuFrameTime->setAnimationOptions(QChart::NoAnimation); // Improves performance
    m_chartCpuFrameTime->setBackgroundRoundness(28);

    m_chartGpuFrameTime->legend()->hide();
    m_chartGpuFrameTime->setAnimationOptions(QChart::NoAnimation); // Improves performance
    m_chartGpuFrameTime->setBackgroundRoundness(28);

    m_chartFrameRate->legend()->hide();
    m_chartFrameRate->setAnimationOptions(QChart::NoAnimation); // Improves performance
    m_chartFrameRate->setBackgroundRoundness(28);

    m_axisYTotalFrameTime = new QValueAxis();
    m_axisYTotalFrameTime->setRange(0, m_headsetRefreshRate + 10);
    m_axisYTotalFrameTime->setTitleText("ms");
    m_chartTotalFrameTime->addAxis(m_axisYTotalFrameTime, Qt::AlignLeft);
    m_seriesTotalFrameTime->attachAxis(m_axisYTotalFrameTime);

    m_axisYCpuFrameTime = new QValueAxis();
    m_axisYCpuFrameTime->setRange(0, m_headsetRefreshRate + 10);
    m_axisYCpuFrameTime->setTitleText("ms");
    m_chartCpuFrameTime->addAxis(m_axisYCpuFrameTime, Qt::AlignLeft);
    m_seriesCpuFrameTime->attachAxis(m_axisYCpuFrameTime);

    m_axisYGpuFrameTime = new QValueAxis();
    m_axisYGpuFrameTime->setRange(0, m_headsetRefreshRate + 10);
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
    //m_chartViewTotalFrameTime->setRenderHint(QPainter::Antialiasing);
    m_chartViewTotalFrameTime->setObjectName("chartViewTotalFrameTime");
    m_chartViewTotalFrameTime->setStyleSheet("#chartViewTotalFrameTime { "
                        "background-color: #232424; "
                        "border-radius: 45px; "
                        "}"
                        );

    m_chartViewCpuFrameTime->setChart(m_chartCpuFrameTime);
    //m_chartViewCpuFrameTime->setRenderHint(QPainter::Antialiasing);
    m_chartViewCpuFrameTime->setObjectName("chartViewCpuFrameTime");
    m_chartViewCpuFrameTime->setStyleSheet("#chartViewCpuFrameTime { "
                        "background-color: #232424; "
                        "border-radius: 45px; "
                        "}"
                        );

    m_chartViewGpuFrameTime->setChart(m_chartGpuFrameTime);
    //m_chartViewGpuFrameTime->setRenderHint(QPainter::Antialiasing);
    m_chartViewGpuFrameTime->setObjectName("chartViewGpuFrameTime");
    m_chartViewGpuFrameTime->setStyleSheet("#chartViewGpuFrameTime { "
                        "background-color: #232424; "
                        "border-radius: 45px; "
                        "}"
                        );

    m_chartViewFrameRate->setChart(m_chartFrameRate);
    //m_chartViewFrameRate->setRenderHint(QPainter::Antialiasing);
    m_chartViewFrameRate->setObjectName("chartViewFrameRate");
    m_chartViewFrameRate->setStyleSheet("#chartViewFrameRate { "
                        "background-color: #232424; "
                        "border-radius: 45px; "
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
