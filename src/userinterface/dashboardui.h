//
// Created by jornt on 03/02/2026.
//

#ifndef PERFORMANCEVR_DASHBOARDUI_H
#define PERFORMANCEVR_DASHBOARDUI_H

#include <QWidget>

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

    void setGpuFrameRate(float frametime);

    Ui::DashboardUI *getUi() const;

private:
    Ui::DashboardUI *ui;
};


#endif //PERFORMANCEVR_DASHBOARDUI_H