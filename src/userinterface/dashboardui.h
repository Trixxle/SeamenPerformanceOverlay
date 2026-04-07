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
#include <QtCharts/QBarSet>
#include <QtCharts/QValueAxis>
#include <QtCharts/QBarCategoryAxis>

QT_BEGIN_NAMESPACE

namespace Ui {
    class DashboardUI;
}

QT_END_NAMESPACE

class DashboardUI : public QWidget {
    Q_OBJECT

public:
    explicit DashboardUI(QWidget *parent = nullptr);

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


    Ui::DashboardUI *getUi() const;

private:
    Ui::DashboardUI *ui;
    const int MAX_GRAPH_POINTS = 60;

    QChart *m_chartTotalFrameTime;
    QBarSet *m_barSetTotalFrameTime;
    QBarSeries *m_seriesTotalFrameTime;
    QValueAxis *m_axisYTotalFrameTime;
    QBarCategoryAxis *m_axisXTotalFrameTime;
    QStringList m_categoriesTotalFrameTime;
    QChartView *m_chartViewTotalFrameTime;

    QChart *m_chartCpuFrameTime;
    QBarSet *m_barSetCpuFrameTime;
    QBarSeries *m_seriesCpuFrameTime;
    QValueAxis *m_axisYCpuFrameTime;
    QBarCategoryAxis *m_axisXCpuFrameTime;
    QStringList m_categoriesCpuFrameTime;
    QChartView *m_chartViewCpuFrameTime;

    QChart *m_chartGpuFrameTime;
    QBarSet *m_barSetGpuFrameTime;
    QBarSeries *m_seriesGpuFrameTime;
    QValueAxis *m_axisYGpuFrameTime;
    QBarCategoryAxis *m_axisXGpuFrameTime;
    QStringList m_categoriesGpuFrameTime;
    QChartView *m_chartViewGpuFrameTime;
};


#endif //PERFORMANCEVR_DASHBOARDUI_H