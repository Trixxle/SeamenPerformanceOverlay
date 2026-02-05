#include "userinterface/dashboardui.h"
#include "openvr.h"

#include <QApplication>

QString GetTrackedDeviceString( vr::IVRSystem *pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop )
{
    char buf[128];
    vr::TrackedPropertyError err;
    pHmd->GetStringTrackedDeviceProperty( unDevice, prop, buf, sizeof( buf ), &err );
    if( err != vr::TrackedProp_Success )
    {
        return QString( "Error Getting String: " ) + pHmd->GetPropErrorNameFromEnum( err );
    }
    else
    {
        return buf;
    }
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

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
    //QApplication a(argc, argv);
    //dashboardui w;
    //w.show();
    //return a.exec();
}