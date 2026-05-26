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

#include "dashboardui.h"
#include "ui_dashboardui.h"
#include "steamvrcontrol/steamvrlogic.h"

DashboardUI::DashboardUI(float headsetRefreshRate, float targetFrameRate, QWidget *parent):
    QWidget(parent),
    ui(new Ui::DashboardUI),
    m_pAxisXFrameTimeConsistency(new QBarCategoryAxis()),
    m_pChartViewFrameTimeConsistency(new QChartView()),

    m_pAxisXCpuFrameTime(new QBarCategoryAxis()),
    m_pChartViewCpuFrameTime(new QChartView()),

    m_pAxisXGpuFrameTime(new QBarCategoryAxis()),
    m_pChartViewGpuFrameTime(new QChartView()),

    m_pAxisXFrameRate(new QBarCategoryAxis()),
    m_pChartViewFrameRate(new QChartView()),

    m_targetFrameRate(targetFrameRate),
    m_headsetRefreshRate(headsetRefreshRate),

    m_opacity(this->windowOpacity()),
    m_settings("Seamen", "PerformanceOverlay")
{
    this->setWindowFlags(Qt::FramelessWindowHint);
    this->setAttribute(Qt::WA_TranslucentBackground);

    ui->setupUi(this);

    ui->moveButtonFrame->setAttribute(Qt::WA_Hover, true);
    ui->moveButtonFrame->installEventFilter(this);

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

    // Hide the move bar by default
    ui->moveButton->hide();

    restoreOpacity();
}

DashboardUI::~DashboardUI() {
    delete ui;
}

void DashboardUI::hideMoveBar(bool hide) {
    if (hide) {
        ui->moveButton->hide();
    }
    else {
        ui->moveButton->show();
    }
}

bool DashboardUI::eventFilter(QObject *watched, QEvent *event){
    // Check if the event is coming from your specific frame
    if (watched == ui->moveButtonFrame) {
        if (event->type() == QEvent::Enter) {
            // Mouse entered the frame -> Zoom IN
            if (!m_geometryCached) {
                m_baseMoveBarGeometry = ui->moveButton->geometry();
                m_geometryCached = true;
            }
            animateButtonZoom(true);
            return true; // We handled the event
        }
        else if (event->type() == QEvent::Leave) {
            // Mouse left the frame -> Zoom OUT
            animateButtonZoom(false);
            return true;
        }
    }
    // Pass all other events to the base class
    return QWidget::eventFilter(watched, event);
}

void DashboardUI::animateButtonZoom(bool zoomIn) {
    QPropertyAnimation *animation = new QPropertyAnimation(ui->moveButton, "geometry");
    animation->setDuration(150);
    animation->setEasingCurve(QEasingCurve::OutQuad); // Adds a nice smooth deceleration

    // How many pixels you want the bar to grow by
    int growWidth = 900;
    int growHeight = 80;

    // Calculate the zoomed rectangle, keeping the center point exactly the same
    QRect zoomedRect(
        m_baseMoveBarGeometry.x() - (growWidth / 2),
        m_baseMoveBarGeometry.y() - (growHeight / 2),
        m_baseMoveBarGeometry.width() + growWidth,
        m_baseMoveBarGeometry.height() + growHeight
    );

    // Always start from the CURRENT geometry so it doesn't stutter
    // if the user moves the mouse in and out quickly mid-animation
    animation->setStartValue(ui->moveButton->geometry());

    if (zoomIn) {
        animation->setEndValue(zoomedRect);
    } else {
        animation->setEndValue(m_baseMoveBarGeometry);
    }

    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void DashboardUI::resetOpacityToDefault() {
    m_opacity = 1.0;
    updateOpacity();
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
    // Workaround to properly save opacity.
    saveOpacity();
    emit opacityChanged(m_opacity);
}

void DashboardUI::increaseOpacityButtonClicked() {
    if (m_opacity >= 1.0) return;
    m_opacity = qBound(0.0, m_opacity + 0.05, 1.0);
    updateOpacity();
}

void DashboardUI::decreaseOpacityButtonClicked() {
    if (m_opacity <= 0.0) return;
    m_opacity = qBound(0.0, m_opacity - 0.05, 1.0);
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
    m_pChartFrameTimeConsistency->blockSignals(true);
    m_pChartCpuFrameTime->blockSignals(true);
    m_pChartGpuFrameTime->blockSignals(true);
    m_pChartFrameRate->blockSignals(true);

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

    m_pChartFrameTimeConsistency->blockSignals(false);
    m_pChartCpuFrameTime->blockSignals(false);
    m_pChartGpuFrameTime->blockSignals(false);
    m_pChartFrameRate->blockSignals(false);

    // Because the signals were blocked it must now be manually forced
    m_pChartFrameTimeConsistency->update();
    m_pChartCpuFrameTime->update();
    m_pChartGpuFrameTime->update();
    m_pChartFrameRate->update();
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

// This is required as for some reason Qt is refusing to atumatocally apply the underlying QPaintEvent function and thus
// renders the overlay wrong.
void DashboardUI::paintEvent(QPaintEvent *event) {
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

// This function was made as I was not at all comfortable using Qt's Widget Designer program. It would be
// 100 times easier to just add these in the widget designer instead of through code but this works and I am too lazy
// to reimplement the same functionality through the designer. I'll do it some other time, it is low priority
// TODO: What is explained above
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

    m_pBarSetFrameTimeConsistencyFast->remove(0, count);
    m_pBarSetFrameTimeConsistencyMedium->remove(0, count);
    m_pBarSetFrameTimeConsistencySlow->remove(0, count);

    m_pBarSetFrameTimeConsistencyFast->append(fastList);
    m_pBarSetFrameTimeConsistencyMedium->append(mediumList);
    m_pBarSetFrameTimeConsistencySlow->append(slowList);
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

    m_pBarSetGpuFrameTimeFast->remove(0, count);
    m_pBarSetGpuFrameTimeMedium->remove(0, count);
    m_pBarSetGpuFrameTimeSlow->remove(0, count);

    m_pBarSetGpuFrameTimeFast->append(fastList);
    m_pBarSetGpuFrameTimeMedium->append(mediumList);
    m_pBarSetGpuFrameTimeSlow->append(slowList);
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

    m_pBarSetCpuFrameTimeFast->remove(0, count);
    m_pBarSetCpuFrameTimeMedium->remove(0, count);
    m_pBarSetCpuFrameTimeSlow->remove(0, count);
    m_pBarSetCpuFrameTimeDropped->remove(0, count);

    m_pBarSetCpuFrameTimeFast->append(fastList);
    m_pBarSetCpuFrameTimeMedium->append(mediumList);
    m_pBarSetCpuFrameTimeSlow->append(slowList);
    m_pBarSetCpuFrameTimeDropped->append(droppedList);
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

    m_pBarSetFrameRateFast->remove(0, count);
    m_pBarSetFrameRateMedium->remove(0, count);
    m_pBarSetFrameRateSlow->remove(0, count);

    m_pBarSetFrameRateFast->append(fastList);
    m_pBarSetFrameRateMedium->append(mediumList);
    m_pBarSetFrameRateSlow->append(slowList);
}

void DashboardUI::updateSystemResources(const SystemResourcesHandler::systemResources& systemResources) {
    setSystemRam(systemResources.systemRam);
    setSystemVram(systemResources.systemVram);
}

void DashboardUI::updateSystemResourceUsage(const SystemResourcesHandler::systemResourceUsage& systemResourceUsage) {
    setSystemRamUsage(systemResourceUsage.ramUsage);
    setSystemVramUsage(systemResourceUsage.vramUsage);
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

void DashboardUI::setSystemRamUsage(float ramUsage) {
    ui->ramUsageLabel->setText(QString::number(roundFloat(ramUsage, 2)));
}

void DashboardUI::setSystemVramUsage(float vramUsage) {
    ui->vramUsageLabel->setText(QString::number(roundFloat(vramUsage, 2)));
}

void DashboardUI::setSystemRam(float systemRam) {
    ui->totalRamLabel->setText(tr("/") + QString::number(roundFloat(systemRam, 2)) + tr(" GB"));
}

void DashboardUI::setSystemVram(float systemVram) {
    ui->totalVramLabel->setText(tr("/") + QString::number(roundFloat(systemVram, 2)) + tr(" GB"));
}

void DashboardUI::setUpCharts() {
    for (int i = 0; i < MAX_GRAPH_POINTS; ++i) {
        m_pCategoriesFrameTimeConsistency.append(QString::number(i));
        m_categoriesCpuFrameTime.append(QString::number(i));
        m_categoriesGpuFrameTime.append(QString::number(i));
        m_categoriesFrameRate.append(QString::number(i));
    }

    m_pAxisXFrameTimeConsistency->append(m_pCategoriesFrameTimeConsistency);
    m_pAxisXFrameTimeConsistency->setVisible(false); // This hides the labels

    m_pAxisXCpuFrameTime->append(m_pCategoriesFrameTimeConsistency);
    m_pAxisXCpuFrameTime->setVisible(false);

    m_pAxisXGpuFrameTime->append(m_pCategoriesFrameTimeConsistency);
    m_pAxisXGpuFrameTime->setVisible(false);

    m_pAxisXFrameRate->append(m_pCategoriesFrameTimeConsistency);
    m_pAxisXFrameRate->setVisible(false);

    m_pBarSetFrameTimeConsistencyFast = new QBarSet("NormalTOTAL");
    m_pBarSetFrameTimeConsistencyFast->setColor(QColor(16, 146, 191));
    m_pBarSetFrameTimeConsistencyFast->setBorderColor(Qt::transparent);

    m_pBarSetFrameTimeConsistencyMedium = new QBarSet("MediumTOTAL");
    m_pBarSetFrameTimeConsistencyMedium->setColor(QColorConstants::Svg::orange);
    m_pBarSetFrameTimeConsistencyMedium->setBorderColor(Qt::transparent);

    m_pBarSetFrameTimeConsistencySlow = new QBarSet("SlowTOTAL");
    m_pBarSetFrameTimeConsistencySlow->setColor(QColorConstants::Svg::red);
    m_pBarSetFrameTimeConsistencySlow->setBorderColor(Qt::transparent);

    m_pSeriesFrameTimeConsistency = new QStackedBarSeries();
    m_pSeriesFrameTimeConsistency->setBarWidth(1.0);
    m_pSeriesFrameTimeConsistency->append(m_pBarSetFrameTimeConsistencyFast);
    m_pSeriesFrameTimeConsistency->append(m_pBarSetFrameTimeConsistencySlow);
    m_pSeriesFrameTimeConsistency->append(m_pBarSetFrameTimeConsistencyMedium);

    m_pBarSetCpuFrameTimeFast = new QBarSet("NormalCPU");
    m_pBarSetCpuFrameTimeFast->setColor(QColor(16, 146, 191));
    m_pBarSetCpuFrameTimeFast->setBorderColor(Qt::transparent);

    m_pBarSetCpuFrameTimeMedium = new QBarSet("MediumCPU");
    m_pBarSetCpuFrameTimeMedium->setColor(QColorConstants::Svg::orange);
    m_pBarSetCpuFrameTimeMedium->setBorderColor(Qt::transparent);

    m_pBarSetCpuFrameTimeSlow = new QBarSet("SlowCPU");
    m_pBarSetCpuFrameTimeSlow->setColor(QColorConstants::Svg::red);
    m_pBarSetCpuFrameTimeSlow->setBorderColor(Qt::transparent);

    m_pBarSetCpuFrameTimeDropped = new QBarSet("SlowCPU");
    m_pBarSetCpuFrameTimeDropped->setColor(QColorConstants::Svg::purple);
    m_pBarSetCpuFrameTimeDropped->setBorderColor(Qt::transparent);

    m_pSeriesCpuFrameTime = new QStackedBarSeries();
    m_pSeriesCpuFrameTime->setBarWidth(1.0);
    m_pSeriesCpuFrameTime->append(m_pBarSetCpuFrameTimeFast);
    m_pSeriesCpuFrameTime->append(m_pBarSetCpuFrameTimeSlow);
    m_pSeriesCpuFrameTime->append(m_pBarSetCpuFrameTimeMedium);
    m_pSeriesCpuFrameTime->append(m_pBarSetCpuFrameTimeDropped);

    m_pBarSetGpuFrameTimeFast = new QBarSet("NormalGPU");
    m_pBarSetGpuFrameTimeFast->setColor(QColor(16, 146, 191));
    m_pBarSetGpuFrameTimeFast->setBorderColor(Qt::transparent);

    m_pBarSetGpuFrameTimeMedium = new QBarSet("MediumGPU");
    m_pBarSetGpuFrameTimeMedium->setColor(QColorConstants::Svg::orange);
    m_pBarSetGpuFrameTimeMedium->setBorderColor(Qt::transparent);

    m_pBarSetGpuFrameTimeSlow = new QBarSet("SlowGPU");
    m_pBarSetGpuFrameTimeSlow->setColor(QColorConstants::Svg::red);
    m_pBarSetGpuFrameTimeSlow->setBorderColor(Qt::transparent);

    m_pSeriesGpuFrameTime = new QStackedBarSeries();
    m_pSeriesGpuFrameTime->setBarWidth(1.0);
    m_pSeriesGpuFrameTime->append(m_pBarSetGpuFrameTimeFast);
    m_pSeriesGpuFrameTime->append(m_pBarSetGpuFrameTimeSlow);
    m_pSeriesGpuFrameTime->append(m_pBarSetGpuFrameTimeMedium);

    m_pBarSetFrameRateFast = new QBarSet("NormalFPS");
    m_pBarSetFrameRateFast->setColor(QColor(16, 146, 191));
    m_pBarSetFrameRateFast->setBorderColor(Qt::transparent);

    m_pBarSetFrameRateMedium = new QBarSet("MediumFPS");
    m_pBarSetFrameRateMedium->setColor(QColorConstants::Svg::orange);
    m_pBarSetFrameRateMedium->setBorderColor(Qt::transparent);

    m_pBarSetFrameRateSlow = new QBarSet("SlowFPS");
    m_pBarSetFrameRateSlow->setColor(QColorConstants::Svg::red);
    m_pBarSetFrameRateSlow->setBorderColor(Qt::transparent);

    m_pSeriesFrameRate = new QStackedBarSeries();
    m_pSeriesFrameRate->setBarWidth(1.0);
    m_pSeriesFrameRate->append(m_pBarSetFrameRateFast);
    m_pSeriesFrameRate->append(m_pBarSetFrameRateSlow);
    m_pSeriesFrameRate->append(m_pBarSetFrameRateMedium);

    m_pChartFrameTimeConsistency = new QChart();
    m_pChartFrameTimeConsistency->addSeries(m_pSeriesFrameTimeConsistency);
    m_pChartFrameTimeConsistency->addAxis(m_pAxisXFrameTimeConsistency, Qt::AlignBottom);
    m_pSeriesFrameTimeConsistency->attachAxis(m_pAxisXFrameTimeConsistency);

    m_pChartCpuFrameTime = new QChart();
    m_pChartCpuFrameTime->addSeries(m_pSeriesCpuFrameTime);
    m_pChartCpuFrameTime->addAxis(m_pAxisXCpuFrameTime, Qt::AlignBottom);
    m_pSeriesCpuFrameTime->attachAxis(m_pAxisXCpuFrameTime);

    m_pChartGpuFrameTime = new QChart();
    m_pChartGpuFrameTime->addSeries(m_pSeriesGpuFrameTime);
    m_pChartGpuFrameTime->addAxis(m_pAxisXGpuFrameTime, Qt::AlignBottom);
    m_pSeriesGpuFrameTime->attachAxis(m_pAxisXGpuFrameTime);

    m_pChartFrameRate = new QChart();
    m_pChartFrameRate->addSeries(m_pSeriesFrameRate);
    m_pChartFrameRate->addAxis(m_pAxisXFrameRate, Qt::AlignBottom);
    m_pSeriesFrameRate->attachAxis(m_pAxisXFrameRate);

    m_pChartFrameTimeConsistency->legend()->hide();
    m_pChartFrameTimeConsistency->setAnimationOptions(QChart::NoAnimation);
    m_pChartFrameTimeConsistency->setBackgroundRoundness(0); // Remove internal rounding
    m_pChartFrameTimeConsistency->setBackgroundBrush(Qt::transparent); // Let the view's background show

    m_pChartCpuFrameTime->legend()->hide();
    m_pChartCpuFrameTime->setAnimationOptions(QChart::NoAnimation);
    m_pChartCpuFrameTime->setBackgroundRoundness(0);
    m_pChartCpuFrameTime->setBackgroundBrush(Qt::transparent);

    m_pChartGpuFrameTime->legend()->hide();
    m_pChartGpuFrameTime->setAnimationOptions(QChart::NoAnimation);
    m_pChartGpuFrameTime->setBackgroundRoundness(0);
    m_pChartGpuFrameTime->setBackgroundBrush(Qt::transparent);

    m_pChartFrameRate->legend()->hide();
    m_pChartFrameRate->setAnimationOptions(QChart::NoAnimation);
    m_pChartFrameRate->setBackgroundRoundness(0);
    m_pChartFrameRate->setBackgroundBrush(Qt::transparent);

    // Makes the charts sit at the bottom of the chartView
    m_pChartFrameTimeConsistency->setMargins(QMargins(0, 20, 0, 0));
    m_pChartCpuFrameTime->setMargins(QMargins(0, 20, 0, 0));
    m_pChartGpuFrameTime->setMargins(QMargins(0, 20, 0, 0));
    m_pChartFrameRate->setMargins(QMargins(0, 20, 0, 0));

    m_pAxisYFrameTimeConsistency = new QValueAxis();
    m_pAxisYFrameTimeConsistency->setRange(0, m_headsetRefreshRate + 10);
    m_pAxisYFrameTimeConsistency->setLabelsVisible(false);
    m_pAxisYFrameTimeConsistency->setLineVisible(false);
    m_pAxisYFrameTimeConsistency->setGridLineVisible(false);
    m_pChartFrameTimeConsistency->addAxis(m_pAxisYFrameTimeConsistency, Qt::AlignLeft);
    m_pSeriesFrameTimeConsistency->attachAxis(m_pAxisYFrameTimeConsistency);

    m_pAxisYCpuFrameTime = new QValueAxis();
    m_pAxisYCpuFrameTime->setRange(0, m_headsetRefreshRate + 10);
    m_pAxisYCpuFrameTime->setLabelsVisible(false);
    m_pAxisYCpuFrameTime->setLineVisible(false);
    m_pAxisYCpuFrameTime->setGridLineVisible(false);
    m_pChartCpuFrameTime->addAxis(m_pAxisYCpuFrameTime, Qt::AlignLeft);
    m_pSeriesCpuFrameTime->attachAxis(m_pAxisYCpuFrameTime);

    m_pAxisYGpuFrameTime = new QValueAxis();
    m_pAxisYGpuFrameTime->setRange(0, m_headsetRefreshRate + 10);
    m_pAxisYGpuFrameTime->setLabelsVisible(false);
    m_pAxisYGpuFrameTime->setLineVisible(false);
    m_pAxisYGpuFrameTime->setGridLineVisible(false);
    m_pChartGpuFrameTime->addAxis(m_pAxisYGpuFrameTime, Qt::AlignLeft);
    m_pSeriesGpuFrameTime->attachAxis(m_pAxisYGpuFrameTime);

    m_pAxisYFrameRate = new QValueAxis();
    m_pAxisYFrameRate->setRange(0, m_targetFrameRate + 10);
    m_pAxisYFrameRate->setLabelsVisible(false);
    m_pAxisYFrameRate->setLineVisible(false);
    m_pAxisYFrameRate->setGridLineVisible(false);
    m_pChartFrameRate->addAxis(m_pAxisYFrameRate, Qt::AlignLeft);
    m_pSeriesFrameRate->attachAxis(m_pAxisYFrameRate);

    QPen targetPen(QColor("#232424"));
    targetPen.setWidth(2);
    QList<qreal> dashPattern;
    dashPattern << 2 << 4; // pixels of line, followed by pixels of empty space
    targetPen.setDashPattern(dashPattern);

    QLineSeries *targetLineTotal = new QLineSeries();
    targetLineTotal->append(0, m_headsetRefreshRate);
    targetLineTotal->append(MAX_GRAPH_POINTS - 1, m_headsetRefreshRate);
    targetLineTotal->setPen(targetPen);
    m_pChartFrameTimeConsistency->addSeries(targetLineTotal);
    targetLineTotal->attachAxis(m_pAxisXFrameTimeConsistency);
    targetLineTotal->attachAxis(m_pAxisYFrameTimeConsistency);

    QLineSeries *targetLineCpu = new QLineSeries();
    targetLineCpu->append(0, m_headsetRefreshRate);
    targetLineCpu->append(MAX_GRAPH_POINTS - 1, m_headsetRefreshRate);
    targetLineCpu->setPen(targetPen);
    m_pChartCpuFrameTime->addSeries(targetLineCpu);
    targetLineCpu->attachAxis(m_pAxisXCpuFrameTime);
    targetLineCpu->attachAxis(m_pAxisYCpuFrameTime);

    QLineSeries *targetLineGpu = new QLineSeries();
    targetLineGpu->append(0, m_headsetRefreshRate);
    targetLineGpu->append(MAX_GRAPH_POINTS - 1, m_headsetRefreshRate);
    targetLineGpu->setPen(targetPen);
    m_pChartGpuFrameTime->addSeries(targetLineGpu);
    targetLineGpu->attachAxis(m_pAxisXGpuFrameTime);
    targetLineGpu->attachAxis(m_pAxisYGpuFrameTime);

    // Initialize with empty data
    for(int i = 0; i < MAX_GRAPH_POINTS; ++i) {
        *m_pBarSetFrameTimeConsistencyFast << 0;
        *m_pBarSetFrameTimeConsistencyMedium << 0;
        *m_pBarSetFrameTimeConsistencySlow << 0;
        *m_pBarSetCpuFrameTimeFast << 0;
        *m_pBarSetCpuFrameTimeMedium << 0;
        *m_pBarSetCpuFrameTimeDropped << 0;
        *m_pBarSetCpuFrameTimeSlow << 0;
        *m_pBarSetGpuFrameTimeFast << 0;
        *m_pBarSetGpuFrameTimeMedium << 0;
        *m_pBarSetGpuFrameTimeSlow << 0;
        *m_pBarSetFrameRateFast << 0;
        *m_pBarSetFrameRateMedium << 0;
        *m_pBarSetFrameRateSlow << 0;
    }

    m_pChartViewFrameTimeConsistency->setChart(m_pChartFrameTimeConsistency);
    m_pChartViewFrameTimeConsistency->setObjectName("chartViewTotalFrameTime");
    m_pChartViewFrameTimeConsistency->setStyleSheet("#chartViewTotalFrameTime { "
                            "background-color: #FFFFFF; "
                            "border-top-left-radius: 45px; "
                            "border-top-right-radius: 45px; "
                            "border-bottom-left-radius: 0px; "
                            "border-bottom-right-radius: 0px; "
                            "}"
                            );

    m_pChartViewCpuFrameTime->setChart(m_pChartCpuFrameTime);
    m_pChartViewCpuFrameTime->setObjectName("chartViewCpuFrameTime");
    m_pChartViewCpuFrameTime->setStyleSheet("#chartViewCpuFrameTime { "
                            "background-color: #FFFFFF; "
                            "border-top-left-radius: 45px; "
                            "border-top-right-radius: 45px; "
                            "border-bottom-left-radius: 0px; "
                            "border-bottom-right-radius: 0px; "
                            "}"
                            );

    m_pChartViewGpuFrameTime->setChart(m_pChartGpuFrameTime);
    m_pChartViewGpuFrameTime->setObjectName("chartViewGpuFrameTime");
    m_pChartViewGpuFrameTime->setStyleSheet("#chartViewGpuFrameTime { "
                            "background-color: #FFFFFF; "
                            "border-top-left-radius: 45px; "
                            "border-top-right-radius: 45px; "
                            "border-bottom-left-radius: 0px; "
                            "border-bottom-right-radius: 0px; "
                            "}"
                            );

    m_pChartViewFrameRate->setChart(m_pChartFrameRate);
    m_pChartViewFrameRate->setObjectName("chartViewFrameRate");
    m_pChartViewFrameRate->setStyleSheet("#chartViewFrameRate { "
                            "background-color: #FFFFFF; "
                            "border-top-left-radius: 45px; "
                            "border-top-right-radius: 45px; "
                            "border-bottom-left-radius: 0px; "
                            "border-bottom-right-radius: 0px; "
                            "}"
                            );


    m_pChartViewCpuFrameTime->setParent(this);
    insertWidgetAtRow(ui->mainGridLayout, m_pChartViewCpuFrameTime, 0, 1, true);

    m_pChartViewGpuFrameTime->setParent(this);
    insertWidgetAtRow(ui->mainGridLayout, m_pChartViewGpuFrameTime, 0,0, false);

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

    // Spacer height
    ui->mainGridLayout->setRowMinimumHeight(2, 20);

    m_pChartViewFrameTimeConsistency->setParent(this);
    insertWidgetAtRow(ui->mainGridLayout, m_pChartViewFrameTimeConsistency, 3, 0, true);

    m_pChartViewFrameRate->setParent(this);
    insertWidgetAtRow(ui->mainGridLayout, m_pChartViewFrameRate, 3, 1, false);
}
