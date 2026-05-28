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

#ifndef PERFORMANCEVR_PANICDASHBOARD_H
#define PERFORMANCEVR_PANICDASHBOARD_H

#include <QSettings>
#include <QWidget>
#include <QApplication>
#include <QPushButton>

QT_BEGIN_NAMESPACE

namespace Ui {
    class panicDashboard;
}

QT_END_NAMESPACE

class panicDashboard : public QWidget {
    Q_OBJECT

public:
    explicit panicDashboard(QWidget *parent = nullptr);

    ~panicDashboard() override;

public slots:
    void setDistanceFadeState(bool checked);
    void restoreDistanceFadeState();
    void saveDistanceFadeState(bool state);

    void setOpacityValue(float newOpacity);
    void setScaleValue(float newScale);
    void setDistanceFadeValue(float newDistanceFadeValue);

    void restoreScale();
    void restoreDistanceFadeValue();
    void restoreOpacity();

private:
    Ui::panicDashboard *ui;
    QSettings m_settings;

signals:
    void panicButtonClicked();
    void distanceCheckboxToggled(bool checked);

    void requestDistanceFadeStartUp();
    void requestDistanceFadeStartDown();
    void requestScaleUp();
    void requestScaleDown();
    void requestOpacityUp();
    void requestOpacityDown();
    void requestLeftControllerAttach();
    void requestRightControllerAttach();
    void requestHmdAttach();
};


#endif //PERFORMANCEVR_PANICDASHBOARD_H