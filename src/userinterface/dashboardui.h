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
#include <QtCharts/QtCharts>
#include <QtCharts/QChartView>
#include <QtCharts/QBarSeries>
#include <QtCharts/QStackedBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QValueAxis>
#include <QtCharts/QBarCategoryAxis>
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
    void setDashboardFrameTime(float frametime);
    void setDashboardCpuFrameTime(float cpuFrameTime);
    void setDashboardGpuFrameTime(float gpuFrameTime);
    void setDashboardHeadsetRefreshRate(float headsetRefreshRate);
    void setDashboardTargetFrameRate(float targetFrameRate);

    void updateTotalFrameTimeGraph(float newFrameTimeMs);
    void updateGpuFrameTimeGraph(float newFrameTimeMs);
    void updateCpuFrameTimeGraph(float newFrameTimeMs);
    void updateFrameRateGraph(float newFrameRate);

    void insertWidgetAtRow(QGridLayout* layout, QWidget* newWidget, int targetRow, int targetColumn, bool toShiftDown);

    [[nodiscard]] Ui::DashboardUI *getUi() const;

private:
    void setUpCharts();
    void paintEvent(QPaintEvent *event) override;

    Ui::DashboardUI *ui;
    const int MAX_GRAPH_POINTS = 50;

    float m_headsetRefreshRate;
    float m_targetFrameRate;

    QChart *m_chartTotalFrameTime;
    QBarSet *m_barSetTotalFrameTimeFast;
    QBarSet *m_barSetTotalFrameTimeMedium;
    QBarSet *m_barSetTotalFrameTimeSlow;
    QStackedBarSeries *m_seriesTotalFrameTime;
    QValueAxis *m_axisYTotalFrameTime;
    QBarCategoryAxis *m_axisXTotalFrameTime;
    QStringList m_categoriesTotalFrameTime;
    QChartView *m_chartViewTotalFrameTime;

    QChart *m_chartCpuFrameTime;
    QBarSet *m_barSetCpuFrameTimeFast;
    QBarSet *m_barSetCpuFrameTimeMedium;
    QBarSet *m_barSetCpuFrameTimeSlow;
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
};


#endif //PERFORMANCEVR_DASHBOARDUI_H