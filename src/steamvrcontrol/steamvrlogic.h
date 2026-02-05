//
// Created by jornt on 05/02/2026.
//

#ifndef PERFORMANCEVR_STEAMVRLOGIC_H
#define PERFORMANCEVR_STEAMVRLOGIC_H
#include <QObject>
#include "openvr.h"


class SteamVRLogic: public QObject {
    Q_OBJECT
    typedef QObject BaseClass();

public:
    SteamVRLogic();
    ~SteamVRLogic();

    bool Init();
    void Shutdown();
    void SetWidget();

    vr::HmdError m_eLastHmdError;

private:
    bool ConnectToVRRuntime();
    void DisconnectFromVRRuntime();

    // The widget created with Qt
    QWidget *m_Widget;

};


#endif //PERFORMANCEVR_STEAMVRLOGIC_H