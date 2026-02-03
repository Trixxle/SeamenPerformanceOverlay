/********************************************************************************
** Form generated from reading UI file 'dashboardui.ui'
**
** Created by: Qt User Interface Compiler version 6.10.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DASHBOARDUI_H
#define UI_DASHBOARDUI_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_dashboardui
{
public:

    void setupUi(QWidget *dashboardui)
    {
        if (dashboardui->objectName().isEmpty())
            dashboardui->setObjectName("dashboardui");
        dashboardui->resize(400, 300);

        retranslateUi(dashboardui);

        QMetaObject::connectSlotsByName(dashboardui);
    } // setupUi

    void retranslateUi(QWidget *dashboardui)
    {
        dashboardui->setWindowTitle(QCoreApplication::translate("dashboardui", "dashboardui", nullptr));
    } // retranslateUi

};

namespace Ui {
    class dashboardui: public Ui_dashboardui {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DASHBOARDUI_H