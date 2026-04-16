//
// Created by jornt on 03/02/2026.
//

#ifndef PERFORMANCEVR_DASHBOARDUI_H
#define PERFORMANCEVR_DASHBOARDUI_H

#include <QWidget>

#include <QChartView>
#include <QBarSeries>
#include <QBarSet>
#include <QValueAxis>
#include <QList>
#include <QtCharts/QtCharts>
#include "frameHandler.h"
#include <QtCharts/QChartView>
#include <QtCharts/QBarSeries>
#include <QtCharts/QStackedBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QValueAxis>
#include <QtCharts/QBarCategoryAxis>
#include <QLineSeries>
#include <QPainter>
#include <QStyleOption>

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

    void updateFrameTimeConsistencyGraph(const QList<float>& newFrameTimes);
    void updateGpuFrameTimeGraph(const QList<float>& newFrameTimes);
    void updateCpuFrameTimeGraph(const QList<float>& newFrameTimes);
    void updateFrameRateGraph(const QList<float>& newFrameRates);

    void insertWidgetAtRow(QGridLayout* layout, QWidget* newWidget, int targetRow, int targetColumn, bool toShiftDown);

    [[nodiscard]] Ui::DashboardUI *getUi() const;

public slots:
    void updateGraphs(const FrameHandler::FrameStatsList& informationList);
    void updateLabels(const FrameHandler::frameStats& information);

private slots:
    void onExitButtonClicked();
    void increaseOpacityButtonClicked();
    void decreaseOpacityButtonClicked();

private:
    void setUpCharts();
    void paintEvent(QPaintEvent *event) override;
    float roundFloat(float number, int decimalCases);

    Ui::DashboardUI *ui;
    const int MAX_GRAPH_POINTS = 250;

    float m_headsetRefreshRate;
    float m_targetFrameRate;

    float m_opacity;

    QChart *m_chartFrameTimeConsistency;
    QBarSet *m_barSetFrameTimeConsistencyFast;
    QBarSet *m_barSetFrameTimeConsistencyMedium;
    QBarSet *m_barSetFrameTimeConsistencySlow;
    QStackedBarSeries *m_seriesFrameTimeConsistency;
    QValueAxis *m_axisYFrameTimeConsistency;
    QBarCategoryAxis *m_axisXFrameTimeConsistency;
    QStringList m_categoriesFrameTimeConsistency;
    QChartView *m_chartViewFrameTimeConsistency;

    QChart *m_chartCpuFrameTime;
    QBarSet *m_barSetCpuFrameTimeFast;
    QBarSet *m_barSetCpuFrameTimeMedium;
    QBarSet *m_barSetCpuFrameTimeSlow;
    QBarSet *m_barSetCpuFrameTimeDropped;
    QStackedBarSeries *m_seriesCpuFrameTime;
    QValueAxis *m_axisYCpuFrameTime;
    QBarCategoryAxis *m_axisXCpuFrameTime;
    QStringList m_categoriesCpuFrameTime;
    QChartView *m_chartViewCpuFrameTime;

    QChart *m_chartGpuFrameTime;
    QBarSet *m_barSetGpuFrameTimeFast;
    QBarSet *m_barSetGpuFrameTimeMedium;
    QBarSet *m_barSetGpuFrameTimeSlow;
    QStackedBarSeries *m_seriesGpuFrameTime;
    QValueAxis *m_axisYGpuFrameTime;
    QBarCategoryAxis *m_axisXGpuFrameTime;
    QStringList m_categoriesGpuFrameTime;
    QChartView *m_chartViewGpuFrameTime;

    QChart *m_chartFrameRate;
    QBarSet *m_barSetFrameRateFast;
    QBarSet *m_barSetFrameRateMedium;
    QBarSet *m_barSetFrameRateSlow;
    QStackedBarSeries *m_seriesFrameRate;
    QValueAxis *m_axisYFrameRate;
    QBarCategoryAxis *m_axisXFrameRate;
    QStringList m_categoriesFrameRate;
    QChartView *m_chartViewFrameRate;

signals:
    void requestControllerSwitch();
    void requestMoveBegin();
    //void requestMoveEnd();
};


#endif //PERFORMANCEVR_DASHBOARDUI_H