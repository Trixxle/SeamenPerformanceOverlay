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
    m_clocksTimer(NULL),
    m_pAxisXFrameTimeConsistency(new QBarCategoryAxis()),

    m_pAxisXCpuFrameTime(new QBarCategoryAxis()),

    m_pAxisXGpuFrameTime(new QBarCategoryAxis()),

    m_pAxisXFrameRate(new QBarCategoryAxis()),

    m_targetFrameRate(targetFrameRate),
    m_headsetRefreshRate(headsetRefreshRate),

    m_opacity(this->windowOpacity()),
    m_settings("Seamen", "PerformanceOverlay")
{
    this->setWindowFlags(Qt::FramelessWindowHint);
    this->setAttribute(Qt::WA_TranslucentBackground);

    ui->setupUi(this);


    // All of the below if to stop the UI from resizing itself when certain UI elements are hidden, like when the
    // SteamVR Dashboard is closed.
    QSizePolicy spOptionFrame = ui->optionBar->sizePolicy();
    QSizePolicy spMMoveFrame = ui->moveButtonFrame->sizePolicy();
    QSizePolicy spScaleFrame = ui->scaleFrame->sizePolicy();
    spOptionFrame.setRetainSizeWhenHidden(true);
    spMMoveFrame.setRetainSizeWhenHidden(true);
    spScaleFrame.setRetainSizeWhenHidden(true);
    ui->optionBar->setSizePolicy(spOptionFrame);
    ui->moveButtonFrame->setSizePolicy(spMMoveFrame);
    ui->scaleFrame->setSizePolicy(spScaleFrame);

    // Setup an event listener for the frames that hold the move and scale bars
    ui->moveButtonFrame->setAttribute(Qt::WA_Hover, true);
    ui->moveButtonFrame->installEventFilter(this);

    ui->scaleFrame->setAttribute(Qt::WA_Hover, true);
    ui->scaleFrame->installEventFilter(this);

    setUpCharts();

    connect(ui->exitButton, &QPushButton::clicked, this, &DashboardUI::onExitButtonClicked);
    connect(ui->increaseOpacityButton, &QPushButton::clicked, this, &DashboardUI::increaseOpacityButtonClicked);
    connect(ui->decreaseOpacityButton, &QPushButton::clicked, this, &DashboardUI::decreaseOpacityButtonClicked);
    connect(ui->switchControllerButton, &QPushButton::clicked, this, &DashboardUI::requestControllerSwitch);
    connect(ui->moveButton, &QPushButton::pressed, this, &DashboardUI::requestMoveBegin);
    connect(ui->scaleButton, &QPushButton::pressed, this, &DashboardUI::requestScaleBegin);
    connect(ui->distanceFadeCheck, &QCheckBox::toggled, this, &DashboardUI::distanceCheckboxToggled);
    connect(ui->plusFadeDistance, &QPushButton::clicked, this, &DashboardUI::requestDistanceFadeStartUp);
    connect(ui->minusFadeDistance, &QPushButton::clicked, this, &DashboardUI::requestDistanceFadeStartDown);

    //ui->mainGridLayout->setSizeConstraint(QLayout::SetMinimumSize); // Forces the existing layout to stretch to the whole window size
    ui->mainGridLayout->setAlignment(Qt::AlignCenter);

    restoreOpacity();
    restoreDistanceFadeState();
    restoreDistanceFadeValue();

    m_clocksTimer = new QTimer(this);
    m_clocksTimer->setTimerType(Qt::VeryCoarseTimer);
    connect(m_clocksTimer, &QTimer::timeout, this, &DashboardUI::updateClocks);
    // Check for new time every 5 seconds. Who cares. (Don't use this overlay for new years)
    m_clocksTimer->start(5000);
    updateClocks();
}

DashboardUI::~DashboardUI() {
    delete ui;
}

void DashboardUI::setAppLaunch(const QString &appName) {
    m_anAppIsActive = true;
    m_playTimer.start();
    ui->playTimeTitle->setText("Playtime in " + appName + ":");
}

void DashboardUI::setAppQuit(const QString &appName) {
    m_anAppIsActive = false;
    ui->playTimeTitle->setText("You have played " + appName + " for:");
}

void DashboardUI::updateClocks() {
    QTime time = QTime::currentTime();
    QString timeString = time.toString("hh:mm");
    ui->clock->setText(timeString);

    if (!m_anAppIsActive) return;

    qint64 elapsedMs = m_playTimer.elapsed();
    QTime playTime = QTime(0, 0).addMSecs(elapsedMs);
    QString playTimeString = playTime.toString("hh:mm");
    ui->playtimeClock->setText(playTimeString);
}

void DashboardUI::restoreDistanceFadeValue() {
    if (!m_settings.value("DistanceFadeStart").isNull()) {
        setDistanceFadeValue(m_settings.value("DistanceFadeStart").toFloat());
    }
    else setDistanceFadeValue(0.4);
}

void DashboardUI::restoreDistanceFadeState() {
    if (!m_settings.value("DistanceFadeOn").isNull()) {
        setDistanceFadeState(m_settings.value("DistanceFadeOn").toBool());
    }
    else setDistanceFadeState(false);
}

void DashboardUI::setOpacityValue(float newOpacity) {
    float opacityPercent = newOpacity * 100.0;
    ui->OpacityVar->setText(QString::number(opacityPercent) + "%");
}

void DashboardUI::setDistanceFadeValue(float newDistanceFadeValue) {
    float distanceInCms = newDistanceFadeValue * 100.0;
    ui->DistanceFadeVar->setText(QString::number(distanceInCms) + "cm");
}

void DashboardUI::setDistanceFadeState(bool state) {
    ui->distanceFadeCheck->setChecked(state);
}

void DashboardUI::hideUi(bool hide) {
    if (hide) {
        ui->moveButtonFrame->hide();
        ui->optionBar->hide();
        ui->scaleFrame->hide();
    }
    else {
        ui->moveButtonFrame->show();
        ui->optionBar->show();
        ui->scaleFrame->show();
    }
}

bool DashboardUI::eventFilter(QObject *watched, QEvent *event) {
    // Only process Enter and Leave events to save CPU cycles
    if (event->type() == QEvent::Enter || event->type() == QEvent::Leave) {
        QPushButton *targetButton = nullptr;
        int growW = 0, growH = 0;

        // Map the watched frame to its specific button and zoom parameters
        if (watched == ui->moveButtonFrame) {
            targetButton = ui->moveButton;
            growW = 450;
            growH = 40;
        }
        else if (watched == ui->scaleFrame) {
            targetButton = ui->scaleButton;
            growW = 35;
            growH = 132;
        }

        // If the event came from one of our mapped frames, trigger the animation
        if (targetButton) {
            bool zoomIn = (event->type() == QEvent::Enter);
            animateButtonZoom(zoomIn, targetButton, growW, growH);
            return true;
        }
    }

    // Pass all other events to the base class
    return QWidget::eventFilter(watched, event);
}

void DashboardUI::animateButtonZoom(bool zoomIn, QPushButton *button, int growWidth, int growHeight) {
    // 1. Cache the base geometry dynamically on the button itself (only happens once)
    if (!button->property("baseGeometry").isValid()) {
        button->setProperty("baseGeometry", button->geometry());
    }

    // Retrieve the cached base geometry specific to this button
    QRect baseGeometry = button->property("baseGeometry").toRect();

    // 2. Setup Animation
    QPropertyAnimation *animation = new QPropertyAnimation(button, "geometry");
    animation->setDuration(150);
    animation->setEasingCurve(QEasingCurve::OutQuad);

    // 3. Calculate the zoomed rectangle based on the button's specific base geometry
    QRect zoomedRect(
        baseGeometry.x() - (growWidth / 2),
        baseGeometry.y() - (growHeight / 2),
        baseGeometry.width() + growWidth,
        baseGeometry.height() + growHeight
    );

    // 4. Start from current geometry to prevent stuttering, animate to target
    animation->setStartValue(button->geometry());
    animation->setEndValue(zoomIn ? zoomedRect : baseGeometry);

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
        setOpacityValue(m_settings.value("Opacity").toFloat());
        updateOpacity();
    }
    else setOpacityValue(1);
    updateOpacity();
}

void DashboardUI::updateOpacity() {
    this->setWindowOpacity(m_opacity);
    // Workaround to properly save opacity.
    setOpacityValue(m_opacity);
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
    m_pChartFrameTimeConsistency->setMargins(QMargins(0, 10, 0, 0));
    m_pChartCpuFrameTime->setMargins(QMargins(0, 10, 0, 0));
    m_pChartGpuFrameTime->setMargins(QMargins(0, 10, 0, 0));
    m_pChartFrameRate->setMargins(QMargins(0, 10, 0, 0));

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

    ui->frameTimeConsistencyView->setChart(m_pChartFrameTimeConsistency);

    ui->cpuFrameTimeView->setChart(m_pChartCpuFrameTime);

    ui->gpuFrameTimeView->setChart(m_pChartGpuFrameTime);

    ui->liveFrameRateView->setChart(m_pChartFrameRate);
}
