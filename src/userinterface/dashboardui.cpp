//
// Created by jornt on 03/02/2026.
//

#include "dashboardui.h"
#include "ui_dashboardui.h"
#include "steamvrcontrol/steamvrlogic.h"

DashboardUI::DashboardUI(float headsetRefreshRate, float targetFrameRate, QWidget *parent):
    QWidget(parent),
    ui(new Ui::DashboardUI),
    m_axisXFrameTimeConsistency(new QBarCategoryAxis()),
    m_chartViewFrameTimeConsistency(new QChartView()),

    m_axisXCpuFrameTime(new QBarCategoryAxis()),
    m_chartViewCpuFrameTime(new QChartView()),

    m_axisXGpuFrameTime(new QBarCategoryAxis()),
    m_chartViewGpuFrameTime(new QChartView()),

    m_axisXFrameRate(new QBarCategoryAxis()),
    m_chartViewFrameRate(new QChartView()),

    m_targetFrameRate(targetFrameRate),
    m_headsetRefreshRate(headsetRefreshRate),

    m_opacity(this->windowOpacity()),
    m_settings("Seamen", "PerformanceOverlay")
{
    this->setWindowFlags(Qt::FramelessWindowHint);
    this->setAttribute(Qt::WA_TranslucentBackground);
    //this->setStyleSheet("background: transparent;");

    ui->setupUi(this);

    this->setObjectName("DashboardUI");

    this->setStyleSheet("#DashboardUI { "
                            "background-color: #232424; "
                            "border-radius: 60px; "
                            "border: 4px solid #1092BF; "
                            "margin: 4px; "
                            "}"
                            );

    setUpCharts();

    connect(ui->exitButton, &QPushButton::clicked, this, &DashboardUI::onExitButtonClicked);
    connect(ui->increaseOpacityButton, &QPushButton::clicked, this, &DashboardUI::increaseOpacityButtonClicked);
    connect(ui->decreaseOpacityButton, &QPushButton::clicked, this, &DashboardUI::decreaseOpacityButtonClicked);
    connect(ui->switchControllerButton, &QPushButton::clicked, this, &DashboardUI::requestControllerSwitch);
    connect(ui->moveButton, &QPushButton::pressed, this, &DashboardUI::requestMoveBegin);
    connect(ui->increaseScaleButton, &QPushButton::clicked, this, &DashboardUI::requestScaleUp);
    connect(ui->decreaseScaleButton, &QPushButton::clicked, this, &DashboardUI::requestScaleDown);

    //ui->mainGridLayout->setSizeConstraint(QLayout::SetMinimumSize); // Forces the existing layout to stretch to the whole window size
    ui->mainGridLayout->setAlignment(Qt::AlignCenter);

    restoreOpacity();
}

DashboardUI::~DashboardUI() {
    delete ui;
}

void DashboardUI::saveOpacity() {
    m_settings.setValue("Opacity", m_opacity);
}

void DashboardUI::restoreOpacity() {
    if (!m_settings.value("Opacity", m_opacity).isNull()) {
        m_opacity = m_settings.value("Opacity", m_opacity).toFloat();
        updateOpacity();
    }
}

void DashboardUI::updateOpacity() {
    this->setWindowOpacity(m_opacity);
    saveOpacity();
}

void DashboardUI::increaseOpacityButtonClicked() {
    if (m_opacity == 1.0) return;
    m_opacity += 0.05;
    updateOpacity();
}

void DashboardUI::decreaseOpacityButtonClicked() {
    if (m_opacity == 0.0) return;
    m_opacity -= 0.05;
    updateOpacity();
}

void DashboardUI::onExitButtonClicked() {
    QApplication::exit();
}

void DashboardUI::updateGraphs(const FrameHandler::FrameStatsList& informationList) {

    if (informationList.isEmpty()) return;

    // After some research blocking signals here increases performance.
    // Qt will send a onSceneChanged signal for every single item inside the struct for every graph
    // Blocking the signal and only sending the last one (which is the only one that matters) decreased GPU usage by 5%
    m_chartFrameTimeConsistency->blockSignals(true);
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
        totalTimes.append(roundFloat(information.frameDeliverySmoothness, 2));
        gpuTimes.append(roundFloat(information.GpuFrametime, 2));
        cpuTimes.append(roundFloat(information.CpuFrametime, 2));
        frameRates.append(roundFloat(information.Framerate, 2));
    }

    updateFrameTimeConsistencyGraph(totalTimes);
    updateGpuFrameTimeGraph(gpuTimes);
    updateCpuFrameTimeGraph(cpuTimes);
    updateFrameRateGraph(frameRates);

    m_chartFrameTimeConsistency->blockSignals(false);
    m_chartCpuFrameTime->blockSignals(false);
    m_chartGpuFrameTime->blockSignals(false);
    m_chartFrameRate->blockSignals(false);

    // Because the signals were blocked it must now be manually forced
    m_chartFrameTimeConsistency->update();
    m_chartCpuFrameTime->update();
    m_chartGpuFrameTime->update();
    m_chartFrameRate->update();
}

void DashboardUI::updateLabels(const FrameHandler::frameStats &information) {
        setDashboardFrameRate(roundFloat(information.Framerate, 2));
        setDashboardFrameTimeConsistency(roundFloat(information.frameDeliverySmoothness, 2));
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

void DashboardUI::updateFrameTimeConsistencyGraph(const QList<float>& newFrameTimes) {
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

    m_barSetFrameTimeConsistencyFast->remove(0, count);
    m_barSetFrameTimeConsistencyMedium->remove(0, count);
    m_barSetFrameTimeConsistencySlow->remove(0, count);

    m_barSetFrameTimeConsistencyFast->append(fastList);
    m_barSetFrameTimeConsistencyMedium->append(mediumList);
    m_barSetFrameTimeConsistencySlow->append(slowList);
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

    QList<qreal> fastList, mediumList, slowList, droppedList;
    fastList.reserve(count);
    mediumList.reserve(count);
    slowList.reserve(count);
    droppedList.reserve(count);

    for (float newFrameTimeMs : newFrameTimes) {
        // Very slow
        if (newFrameTimeMs > m_headsetRefreshRate + 3.0) {
            droppedList.append(0.0);
            mediumList.append(0.0);
            fastList.append(0.0);
            slowList.append(newFrameTimeMs);
        }
        // Slow
        else if (newFrameTimeMs > m_headsetRefreshRate) {
            droppedList.append(0.0);
            fastList.append(0.0);
            slowList.append(0.0);
            mediumList.append(newFrameTimeMs);
        }
        // Dropped frame
        else if (newFrameTimeMs < 0.0) {
            fastList.append(0.0);
            slowList.append(0.0);
            mediumList.append(0.0);
            droppedList.append(-newFrameTimeMs);
        }
        // On time
        else {
            droppedList.append(0.0);
            slowList.append(0.0);
            mediumList.append(0.0);
            fastList.append(newFrameTimeMs);
        }
    }

    m_barSetCpuFrameTimeFast->remove(0, count);
    m_barSetCpuFrameTimeMedium->remove(0, count);
    m_barSetCpuFrameTimeSlow->remove(0, count);
    m_barSetCpuFrameTimeDropped->remove(0, count);

    m_barSetCpuFrameTimeFast->append(fastList);
    m_barSetCpuFrameTimeMedium->append(mediumList);
    m_barSetCpuFrameTimeSlow->append(slowList);
    m_barSetCpuFrameTimeDropped->append(droppedList);
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

void DashboardUI::setDashboardFrameTimeConsistency(float frametime) {
    ui->frameTimeConsistencyLabel->setText(QString::number(frametime));
}

void DashboardUI::setDashboardCpuFrameTime(float cpuFrameTime) {
    ui->cpuFrameTimeLabel->setText(QString::number(cpuFrameTime));
}

void DashboardUI::setDashboardGpuFrameTime(float gpuFrameTime) {
    ui->gpuFrameTimeLabel->setText(QString::number(gpuFrameTime));
}

void DashboardUI::setDashboardHeadsetRefreshRate(float headsetRefreshRate) {
    ui->headsetRefreshRateLabel->setText("=< " + QString::number(headsetRefreshRate));
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
        m_categoriesFrameTimeConsistency.append(QString::number(i));
        m_categoriesCpuFrameTime.append(QString::number(i));
        m_categoriesGpuFrameTime.append(QString::number(i));
        m_categoriesFrameRate.append(QString::number(i));
    }

    m_axisXFrameTimeConsistency->append(m_categoriesFrameTimeConsistency);
    m_axisXFrameTimeConsistency->setVisible(false); // This hides the labels

    m_axisXCpuFrameTime->append(m_categoriesFrameTimeConsistency);
    m_axisXCpuFrameTime->setVisible(false);

    m_axisXGpuFrameTime->append(m_categoriesFrameTimeConsistency);
    m_axisXGpuFrameTime->setVisible(false);

    m_axisXFrameRate->append(m_categoriesFrameTimeConsistency);
    m_axisXFrameRate->setVisible(false);

    m_barSetFrameTimeConsistencyFast = new QBarSet("NormalTOTAL");
    m_barSetFrameTimeConsistencyFast->setColor(QColor("#1092BF"));
    m_barSetFrameTimeConsistencyFast->setBorderColor(Qt::transparent);

    m_barSetFrameTimeConsistencyMedium = new QBarSet("MediumTOTAL");
    m_barSetFrameTimeConsistencyMedium->setColor(QColorConstants::Svg::orange);
    m_barSetFrameTimeConsistencyMedium->setBorderColor(Qt::transparent);

    m_barSetFrameTimeConsistencySlow = new QBarSet("SlowTOTAL");
    m_barSetFrameTimeConsistencySlow->setColor(QColorConstants::Svg::red);
    m_barSetFrameTimeConsistencySlow->setBorderColor(Qt::transparent);

    m_seriesFrameTimeConsistency = new QStackedBarSeries();
    m_seriesFrameTimeConsistency->setBarWidth(1.0);
    m_seriesFrameTimeConsistency->append(m_barSetFrameTimeConsistencyFast);
    m_seriesFrameTimeConsistency->append(m_barSetFrameTimeConsistencySlow);
    m_seriesFrameTimeConsistency->append(m_barSetFrameTimeConsistencyMedium);

    m_barSetCpuFrameTimeFast = new QBarSet("NormalCPU");
    m_barSetCpuFrameTimeFast->setColor(QColor("#1092BF"));
    m_barSetCpuFrameTimeFast->setBorderColor(Qt::transparent);

    m_barSetCpuFrameTimeMedium = new QBarSet("MediumCPU");
    m_barSetCpuFrameTimeMedium->setColor(QColorConstants::Svg::orange);
    m_barSetCpuFrameTimeMedium->setBorderColor(Qt::transparent);

    m_barSetCpuFrameTimeSlow = new QBarSet("SlowCPU");
    m_barSetCpuFrameTimeSlow->setColor(QColorConstants::Svg::red);
    m_barSetCpuFrameTimeSlow->setBorderColor(Qt::transparent);

    m_barSetCpuFrameTimeDropped = new QBarSet("SlowCPU");
    m_barSetCpuFrameTimeDropped->setColor(QColorConstants::Svg::purple);
    m_barSetCpuFrameTimeDropped->setBorderColor(Qt::transparent);

    m_seriesCpuFrameTime = new QStackedBarSeries();
    m_seriesCpuFrameTime->setBarWidth(1.0);
    m_seriesCpuFrameTime->append(m_barSetCpuFrameTimeFast);
    m_seriesCpuFrameTime->append(m_barSetCpuFrameTimeSlow);
    m_seriesCpuFrameTime->append(m_barSetCpuFrameTimeMedium);
    m_seriesCpuFrameTime->append(m_barSetCpuFrameTimeDropped);

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

    m_chartFrameTimeConsistency = new QChart();
    m_chartFrameTimeConsistency->addSeries(m_seriesFrameTimeConsistency);
    m_chartFrameTimeConsistency->addAxis(m_axisXFrameTimeConsistency, Qt::AlignBottom);
    m_seriesFrameTimeConsistency->attachAxis(m_axisXFrameTimeConsistency);

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

    m_chartFrameTimeConsistency->legend()->hide();
    m_chartFrameTimeConsistency->setAnimationOptions(QChart::NoAnimation);
    m_chartFrameTimeConsistency->setBackgroundRoundness(0); // Remove internal rounding
    m_chartFrameTimeConsistency->setBackgroundBrush(Qt::transparent); // Let the view's background show

    m_chartCpuFrameTime->legend()->hide();
    m_chartCpuFrameTime->setAnimationOptions(QChart::NoAnimation);
    m_chartCpuFrameTime->setBackgroundRoundness(0);
    m_chartCpuFrameTime->setBackgroundBrush(Qt::transparent);

    m_chartGpuFrameTime->legend()->hide();
    m_chartGpuFrameTime->setAnimationOptions(QChart::NoAnimation);
    m_chartGpuFrameTime->setBackgroundRoundness(0);
    m_chartGpuFrameTime->setBackgroundBrush(Qt::transparent);

    m_chartFrameRate->legend()->hide();
    m_chartFrameRate->setAnimationOptions(QChart::NoAnimation);
    m_chartFrameRate->setBackgroundRoundness(0);
    m_chartFrameRate->setBackgroundBrush(Qt::transparent);

    // Makes the charts sit at the bottom of the chartView
    m_chartFrameTimeConsistency->setMargins(QMargins(0, 20, 0, 0));
    m_chartCpuFrameTime->setMargins(QMargins(0, 20, 0, 0));
    m_chartGpuFrameTime->setMargins(QMargins(0, 20, 0, 0));
    m_chartFrameRate->setMargins(QMargins(0, 20, 0, 0));

    m_axisYFrameTimeConsistency = new QValueAxis();
    m_axisYFrameTimeConsistency->setRange(0, m_headsetRefreshRate + 10);
    m_axisYFrameTimeConsistency->setLabelsVisible(false);
    m_axisYFrameTimeConsistency->setLineVisible(false);
    m_axisYFrameTimeConsistency->setGridLineVisible(false);
    m_chartFrameTimeConsistency->addAxis(m_axisYFrameTimeConsistency, Qt::AlignLeft);
    m_seriesFrameTimeConsistency->attachAxis(m_axisYFrameTimeConsistency);

    m_axisYCpuFrameTime = new QValueAxis();
    m_axisYCpuFrameTime->setRange(0, m_headsetRefreshRate + 10);
    m_axisYCpuFrameTime->setLabelsVisible(false);
    m_axisYCpuFrameTime->setLineVisible(false);
    m_axisYCpuFrameTime->setGridLineVisible(false);
    m_chartCpuFrameTime->addAxis(m_axisYCpuFrameTime, Qt::AlignLeft);
    m_seriesCpuFrameTime->attachAxis(m_axisYCpuFrameTime);

    m_axisYGpuFrameTime = new QValueAxis();
    m_axisYGpuFrameTime->setRange(0, m_headsetRefreshRate + 10);
    m_axisYGpuFrameTime->setLabelsVisible(false);
    m_axisYGpuFrameTime->setLineVisible(false);
    m_axisYGpuFrameTime->setGridLineVisible(false);
    m_chartGpuFrameTime->addAxis(m_axisYGpuFrameTime, Qt::AlignLeft);
    m_seriesGpuFrameTime->attachAxis(m_axisYGpuFrameTime);

    m_axisYFrameRate = new QValueAxis();
    m_axisYFrameRate->setRange(0, m_targetFrameRate + 10);
    m_axisYFrameRate->setLabelsVisible(false);
    m_axisYFrameRate->setLineVisible(false);
    m_axisYFrameRate->setGridLineVisible(false);
    m_chartFrameRate->addAxis(m_axisYFrameRate, Qt::AlignLeft);
    m_seriesFrameRate->attachAxis(m_axisYFrameRate);

    QPen targetPen(QColor("#232424"));
    targetPen.setWidth(2);
    QList<qreal> dashPattern;
    dashPattern << 2 << 4; // pixels of line, followed by pixels of empty space
    targetPen.setDashPattern(dashPattern);

    QLineSeries *targetLineTotal = new QLineSeries();
    targetLineTotal->append(0, m_headsetRefreshRate);
    targetLineTotal->append(MAX_GRAPH_POINTS - 1, m_headsetRefreshRate);
    targetLineTotal->setPen(targetPen);
    m_chartFrameTimeConsistency->addSeries(targetLineTotal);
    targetLineTotal->attachAxis(m_axisXFrameTimeConsistency);
    targetLineTotal->attachAxis(m_axisYFrameTimeConsistency);

    QLineSeries *targetLineCpu = new QLineSeries();
    targetLineCpu->append(0, m_headsetRefreshRate);
    targetLineCpu->append(MAX_GRAPH_POINTS - 1, m_headsetRefreshRate);
    targetLineCpu->setPen(targetPen);
    m_chartCpuFrameTime->addSeries(targetLineCpu);
    targetLineCpu->attachAxis(m_axisXCpuFrameTime);
    targetLineCpu->attachAxis(m_axisYCpuFrameTime);

    QLineSeries *targetLineGpu = new QLineSeries();
    targetLineGpu->append(0, m_headsetRefreshRate);
    targetLineGpu->append(MAX_GRAPH_POINTS - 1, m_headsetRefreshRate);
    targetLineGpu->setPen(targetPen);
    m_chartGpuFrameTime->addSeries(targetLineGpu);
    targetLineGpu->attachAxis(m_axisXGpuFrameTime);
    targetLineGpu->attachAxis(m_axisYGpuFrameTime);

    // Initialize with empty data
    for(int i = 0; i < MAX_GRAPH_POINTS; ++i) {
        *m_barSetFrameTimeConsistencyFast << 0;
        *m_barSetFrameTimeConsistencyMedium << 0;
        *m_barSetFrameTimeConsistencySlow << 0;
        *m_barSetCpuFrameTimeFast << 0;
        *m_barSetCpuFrameTimeMedium << 0;
        *m_barSetCpuFrameTimeDropped << 0;
        *m_barSetCpuFrameTimeSlow << 0;
        *m_barSetGpuFrameTimeFast << 0;
        *m_barSetGpuFrameTimeMedium << 0;
        *m_barSetGpuFrameTimeSlow << 0;
        *m_barSetFrameRateFast << 0;
        *m_barSetFrameRateMedium << 0;
        *m_barSetFrameRateSlow << 0;
    }

    m_chartViewFrameTimeConsistency->setChart(m_chartFrameTimeConsistency);
    //m_chartViewFrameTimeConsistency->setRenderHint(QPainter::Antialiasing);
    m_chartViewFrameTimeConsistency->setObjectName("chartViewTotalFrameTime");
    m_chartViewFrameTimeConsistency->setStyleSheet("#chartViewTotalFrameTime { "
                            "background-color: #FFFFFF; "
                            "border-top-left-radius: 45px; "
                            "border-top-right-radius: 45px; "
                            "border-bottom-left-radius: 0px; "
                            "border-bottom-right-radius: 0px; "
                            "}"
                            );

    m_chartViewCpuFrameTime->setChart(m_chartCpuFrameTime);
    //m_chartViewCpuFrameTime->setRenderHint(QPainter::Antialiasing);
    m_chartViewCpuFrameTime->setObjectName("chartViewCpuFrameTime");
    m_chartViewCpuFrameTime->setStyleSheet("#chartViewCpuFrameTime { "
                            "background-color: #FFFFFF; "
                            "border-top-left-radius: 45px; "
                            "border-top-right-radius: 45px; "
                            "border-bottom-left-radius: 0px; "
                            "border-bottom-right-radius: 0px; "
                            "}"
                            );

    m_chartViewGpuFrameTime->setChart(m_chartGpuFrameTime);
    //m_chartViewGpuFrameTime->setRenderHint(QPainter::Antialiasing);
    m_chartViewGpuFrameTime->setObjectName("chartViewGpuFrameTime");
    m_chartViewGpuFrameTime->setStyleSheet("#chartViewGpuFrameTime { "
                            "background-color: #FFFFFF; "
                            "border-top-left-radius: 45px; "
                            "border-top-right-radius: 45px; "
                            "border-bottom-left-radius: 0px; "
                            "border-bottom-right-radius: 0px; "
                            "}"
                            );

    m_chartViewFrameRate->setChart(m_chartFrameRate);
    //m_chartViewFrameRate->setRenderHint(QPainter::Antialiasing);
    m_chartViewFrameRate->setObjectName("chartViewFrameRate");
    m_chartViewFrameRate->setStyleSheet("#chartViewFrameRate { "
                            "background-color: #FFFFFF; "
                            "border-top-left-radius: 45px; "
                            "border-top-right-radius: 45px; "
                            "border-bottom-left-radius: 0px; "
                            "border-bottom-right-radius: 0px; "
                            "}"
                            );


    m_chartViewCpuFrameTime->setParent(this);
    insertWidgetAtRow(ui->mainGridLayout, m_chartViewCpuFrameTime, 0, 1, true);

    m_chartViewGpuFrameTime->setParent(this);
    insertWidgetAtRow(ui->mainGridLayout, m_chartViewGpuFrameTime, 0,0, false);

    /*
    m_chartViewFrameTimeConsistency->setParent(this);
    insertWidgetAtRow(ui->mainGridLayout, m_chartViewFrameTimeConsistency, 2, 0, true);

    m_chartViewFrameRate->setParent(this);
    insertWidgetAtRow(ui->mainGridLayout, m_chartViewFrameRate, 2, 1, false);
    */

    // Add spacers
    struct LayoutShiftItem {
        QLayoutItem* item;
        int r, c, rs, cs;
    };
    QList<LayoutShiftItem> itemsToShift;

    for (int i = 0; i < ui->mainGridLayout->count(); ++i) {
        int r, c, rs, cs;
        ui->mainGridLayout->getItemPosition(i, &r, &c, &rs, &cs);
        // Grab everything from Row 2 downwards
        if (r >= 2) {
            itemsToShift.append({ui->mainGridLayout->itemAt(i), r, c, rs, cs});
        }
    }

    for (const LayoutShiftItem& si : itemsToShift) {
        ui->mainGridLayout->removeItem(si.item);
    }

    for (const LayoutShiftItem& si : itemsToShift) {
        ui->mainGridLayout->addItem(si.item, si.r + 1, si.c, si.rs, si.cs);
    }

    // SPacer height
    ui->mainGridLayout->setRowMinimumHeight(2, 20);

    m_chartViewFrameTimeConsistency->setParent(this);
    insertWidgetAtRow(ui->mainGridLayout, m_chartViewFrameTimeConsistency, 3, 0, true);

    m_chartViewFrameRate->setParent(this);
    insertWidgetAtRow(ui->mainGridLayout, m_chartViewFrameRate, 3, 1, false);
}
