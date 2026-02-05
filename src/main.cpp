#include "userinterface/dashboardui.h"
#include "steamvrcontrol/steamvrlogic.h"
#include "openvr.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    /*QApplication a(argc, argv);

    vr::HmdError m_eLastHmdError = vr::VRInitError_None;
    QString m_strVRDriver;
    QString m_strVRDisplay;
    vr::IVRSystem *pVRSystem = vr::VR_Init( &m_eLastHmdError, vr::VRApplication_Overlay );

        if ( m_eLastHmdError != vr::VRInitError_None )
        {
            m_strVRDriver = "No Driver";
            m_strVRDisplay = "No Display";
            return false;
        }

        m_strVRDriver = GetTrackedDeviceString(pVRSystem, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String);
        m_strVRDisplay = GetTrackedDeviceString(pVRSystem, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String);


    return 0;
*/
    /*
    QApplication a(argc, argv);
    dashboardui w;
    w.show();
    return a.exec();
    */

    QApplication a(argc, argv);

    SteamVRLogic testInit = SteamVRLogic();
    testInit.Init();

    return 0;
}