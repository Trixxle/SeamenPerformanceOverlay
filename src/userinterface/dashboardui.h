#ifndef DASHBOARDUI_H
#define DASHBOARDUI_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class dashboardui;
}
QT_END_NAMESPACE

class dashboardui : public QWidget
{
    Q_OBJECT

public:
    dashboardui(QWidget *parent = nullptr);
    ~dashboardui();

private:
    Ui::dashboardui *ui;
};
#endif // DASHBOARDUI_H
