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

#ifndef PERFORMANCEVR_DASHBOARDUI_H
#define PERFORMANCEVR_DASHBOARDUI_H

#include <QWidget>
#include <QChartView>
#include <QBarSeries>
#include <QBarSet>
#include <QValueAxis>
#include <QList>
#include <QtCharts/QtCharts>
#include <QtCharts/QChartView>
#include <QtCharts/QBarSeries>
#include <QtCharts/QStackedBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QValueAxis>
#include <QtCharts/QBarCategoryAxis>
#include <QLineSeries>
#include <QPainter>
#include <QStyleOption>
#include <QPropertyAnimation>
#include <QMainWindow>
#include <QEvent>
#include <cmath>
#include "frameHandler.h"
#include "systemResourcesHandler.h"
#include "userSettings.h"

QT_BEGIN_NAMESPACE

namespace Ui {
    class DashboardUI;
}

QT_END_NAMESPACE

class DashboardUI : public QWidget {
    Q_OBJECT

public:
    explicit DashboardUI(float headsetRefreshRate, float targetFrameRate, QWidget *parent = nullptr);

    ~DashboardUI() override;

    void setDashboardFrameRate(float framerate);
    void setDashboardFrameTimeConsistency(float frametime);
    void setDashboardCpuFrameTime(float cpuFrameTime);
    void setDashboardGpuFrameTime(float gpuFrameTime);
    void setDashboardHeadsetRefreshRate(float headsetRefreshRate);
    void setDashboardTargetFrameRate(float targetFrameRate);
    void setSmoothFrameRate(float smoothFrameRate);
    void setSystemRam(float systemRam);
    void setSystemVram(float systemVram);
    void setSystemRamUsage(float ramUasge);
    void setSystemVramUsage(float vramUasge);

    void updateFrameTimeConsistencyGraph(const QList<float>& newFrameTimes);
    void updateGpuFrameTimeGraph(const QList<float>& newFrameTimes);
    void updateCpuFrameTimeGraph(const QList<float>& newFrameTimes);
    void updateFrameRateGraph(const QList<float>& newFrameRates);

public slots:
    void updateGraphs(const FrameHandler::FrameStatsList& informationList);
    void updateLabels(const FrameHandler::frameStats& information);
    void updateSystemResources(const SystemResourcesHandler::systemResources& systemResources);
    void updateSystemResourceUsage(const SystemResourcesHandler::systemResourceUsage& systemResourceUsage);
    void updateOpacityValue();
    void updateOpacity();
    void updateDistanceFadeValue();
    void updateDistanceFadeState();
    void updateTrackersShown();
    void updateUpdateChartsColors();
    void hideUi(bool hide);
    void setAppLaunch(const QString &appName);
    void setAppQuit(const QString &appName);
    void setLeftControllerBatteryLevel(float level, bool charging);
    void setRightControllerBatteryLevel(float level, bool charging);
    void setHeadseyBatteryLevel(float level, bool charging);
    void setTrackersBatteryLevel(float level, uint32_t index, bool charging);
    void addTrackerToUi(uint32_t index);
    void removeTrackedFromUI(uint32_t index);

private slots:
    void updateClocks();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:

    struct chartColors {
        QColor fast;
        QColor medium;
        QColor slow;
        QColor dropped;
    };

    struct colorBlindColors {
        static chartColors getColors(userSettings::colorBlindType type) {
            switch (type) {
                case userSettings::colorBlindType::protanopia:
                    return {QColor(16,146,191), QColor(255,165,0), QColor(186, 0,0), QColor(34, 13, 46)};
                case userSettings::colorBlindType::deuteranopia:
                    return {QColor(16,146,191), QColor(255,165,0), QColor(108, 0,0), QColor(3, 0, 46)};
                case userSettings::colorBlindType::tritanopia:
                    return {QColor(16,146,191), QColor(255,223,0), QColor(255, 0,0), QColor(11, 0, 78)};
                case userSettings::colorBlindType::none:
                default:
                    return {QColor(16,146,191), QColor(255,165,0), QColor(255, 0,0), QColor(59, 0, 59)};
            }
        }
    };

    void setUpCharts(chartColors colors);
    void paintEvent(QPaintEvent *event) override;
    float roundFloat(float number, int decimalCases);
    void animateButtonZoom(bool zoomIn, QPushButton *button, int growWidth, int growHeight);

    Ui::DashboardUI *ui;
    const int MAX_GRAPH_POINTS = 250;

    QTimer *m_clocksTimer;
    bool m_anAppIsActive = false;
    QElapsedTimer m_playTimer;

    QRect m_baseMoveBarGeometry;
    bool m_geometryCachedMove = false;
    bool m_geometryCachedBar = false;

    float m_headsetRefreshRate;
    float m_targetFrameRate;

    QChart *m_pChartFrameTimeConsistency;
    QBarSet *m_pBarSetFrameTimeConsistencyFast;
    QBarSet *m_pBarSetFrameTimeConsistencyMedium;
    QBarSet *m_pBarSetFrameTimeConsistencySlow;
    QStackedBarSeries *m_pSeriesFrameTimeConsistency;
    QValueAxis *m_pAxisYFrameTimeConsistency;
    QBarCategoryAxis *m_pAxisXFrameTimeConsistency;
    QStringList m_pCategoriesFrameTimeConsistency;

    QChart *m_pChartCpuFrameTime;
    QBarSet *m_pBarSetCpuFrameTimeFast;
    QBarSet *m_pBarSetCpuFrameTimeMedium;
    QBarSet *m_pBarSetCpuFrameTimeSlow;
    QBarSet *m_pBarSetCpuFrameTimeDropped;
    QStackedBarSeries *m_pSeriesCpuFrameTime;
    QValueAxis *m_pAxisYCpuFrameTime;
    QBarCategoryAxis *m_pAxisXCpuFrameTime;
    QStringList m_categoriesCpuFrameTime;

    QChart *m_pChartGpuFrameTime;
    QBarSet *m_pBarSetGpuFrameTimeFast;
    QBarSet *m_pBarSetGpuFrameTimeMedium;
    QBarSet *m_pBarSetGpuFrameTimeSlow;
    QStackedBarSeries *m_pSeriesGpuFrameTime;
    QValueAxis *m_pAxisYGpuFrameTime;
    QBarCategoryAxis *m_pAxisXGpuFrameTime;
    QStringList m_categoriesGpuFrameTime;

    QChart *m_pChartFrameRate;
    QBarSet *m_pBarSetFrameRateFast;
    QBarSet *m_pBarSetFrameRateMedium;
    QBarSet *m_pBarSetFrameRateSlow;
    QStackedBarSeries *m_pSeriesFrameRate;
    QValueAxis *m_pAxisYFrameRate;
    QBarCategoryAxis *m_pAxisXFrameRate;
    QStringList m_categoriesFrameRate;

signals:
    void requestControllerSwitch();
    void requestMoveBegin();
    void requestScaleBegin();
};


#endif //PERFORMANCEVR_DASHBOARDUI_H