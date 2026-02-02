//
// Created by jornt on 02/02/2026.
//

#ifndef PERFORMANCEVR_DASHBOARDUI_H
#define PERFORMANCEVR_DASHBOARDUI_H

#include <QWidget>


QT_BEGIN_NAMESPACE

namespace Ui {
    class dashboardui;
}

QT_END_NAMESPACE

class dashboardui : public QWidget {
    Q_OBJECT

public:
    explicit dashboardui(QWidget *parent = nullptr);

    ~dashboardui() override;

private:
    Ui::dashboardui *ui;
};


#endif //PERFORMANCEVR_DASHBOARDUI_H