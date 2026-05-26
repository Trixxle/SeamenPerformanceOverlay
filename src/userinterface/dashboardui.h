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
#include "frameHandler.h"
#include "systemResourcesHandler.h"

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

    void insertWidgetAtRow(QGridLayout* layout, QWidget* newWidget, int targetRow, int targetColumn, bool toShiftDown);

public slots:
    void updateGraphs(const FrameHandler::FrameStatsList& informationList);
    void updateLabels(const FrameHandler::frameStats& information);
    void updateSystemResources(const SystemResourcesHandler::systemResources& systemResources);
    void updateSystemResourceUsage(const SystemResourcesHandler::systemResourceUsage& systemResourceUsage);
    void saveOpacity();
    void restoreOpacity();
    void restoreDistanceFadeValue();
    void restoreDistanceFadeState();
    void setOpacityValue(float newValue);
    void setDistanceFadeValue(float newValue);
    void resetOpacityToDefault();
    void increaseOpacityButtonClicked();
    void decreaseOpacityButtonClicked();
    void hideUi(bool hide);

private slots:
    void onExitButtonClicked();
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void setUpCharts();
    void paintEvent(QPaintEvent *event) override;
    float roundFloat(float number, int decimalCases);
    void updateOpacity();
    void animateButtonZoom(bool zoomIn, QPushButton *button, int growWidth, int growHeight);

    Ui::DashboardUI *ui;
    const int MAX_GRAPH_POINTS = 250;

    QRect m_baseMoveBarGeometry;
    bool m_geometryCachedMove = false;
    bool m_geometryCachedBar = false;

    float m_headsetRefreshRate;
    float m_targetFrameRate;

    float m_opacity;
    QSettings m_settings;

    QChart *m_pChartFrameTimeConsistency;
    QBarSet *m_pBarSetFrameTimeConsistencyFast;
    QBarSet *m_pBarSetFrameTimeConsistencyMedium;
    QBarSet *m_pBarSetFrameTimeConsistencySlow;
    QStackedBarSeries *m_pSeriesFrameTimeConsistency;
    QValueAxis *m_pAxisYFrameTimeConsistency;
    QBarCategoryAxis *m_pAxisXFrameTimeConsistency;
    QStringList m_pCategoriesFrameTimeConsistency;
    QChartView *m_pChartViewFrameTimeConsistency;

    QChart *m_pChartCpuFrameTime;
    QBarSet *m_pBarSetCpuFrameTimeFast;
    QBarSet *m_pBarSetCpuFrameTimeMedium;
    QBarSet *m_pBarSetCpuFrameTimeSlow;
    QBarSet *m_pBarSetCpuFrameTimeDropped;
    QStackedBarSeries *m_pSeriesCpuFrameTime;
    QValueAxis *m_pAxisYCpuFrameTime;
    QBarCategoryAxis *m_pAxisXCpuFrameTime;
    QStringList m_categoriesCpuFrameTime;
    QChartView *m_pChartViewCpuFrameTime;

    QChart *m_pChartGpuFrameTime;
    QBarSet *m_pBarSetGpuFrameTimeFast;
    QBarSet *m_pBarSetGpuFrameTimeMedium;
    QBarSet *m_pBarSetGpuFrameTimeSlow;
    QStackedBarSeries *m_pSeriesGpuFrameTime;
    QValueAxis *m_pAxisYGpuFrameTime;
    QBarCategoryAxis *m_pAxisXGpuFrameTime;
    QStringList m_categoriesGpuFrameTime;
    QChartView *m_pChartViewGpuFrameTime;

    QChart *m_pChartFrameRate;
    QBarSet *m_pBarSetFrameRateFast;
    QBarSet *m_pBarSetFrameRateMedium;
    QBarSet *m_pBarSetFrameRateSlow;
    QStackedBarSeries *m_pSeriesFrameRate;
    QValueAxis *m_pAxisYFrameRate;
    QBarCategoryAxis *m_pAxisXFrameRate;
    QStringList m_categoriesFrameRate;
    QChartView *m_pChartViewFrameRate;

signals:
    void requestControllerSwitch();
    void requestMoveBegin();
    void requestScaleUp();
    void requestScaleDown();

    void opacityChanged(float newOpacity);
};


#endif //PERFORMANCEVR_DASHBOARDUI_H