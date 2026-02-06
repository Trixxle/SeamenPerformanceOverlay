//
// Created by jornt on 05/02/2026.
//

#include "steamvrlogic.h"

SteamVRLogic *s_pSharedSteamVRLogic= NULL;

SteamVRLogic *SteamVRLogic::SharedInstance()
{
    if ( !s_pSharedSteamVRLogic )
    {
        s_pSharedSteamVRLogic = new SteamVRLogic();
    }
    return s_pSharedSteamVRLogic;
}

SteamVRLogic::SteamVRLogic():
BaseClass(),
m_eLastHmdError(vr::VRInitError_None),
m_Widget(NULL),
m_strVRDriver("No Driver"),
m_strVRDisplay("No Display"),
m_strOverlayName("Seamen Performance Overlay"),
m_pPumpEventsTimer(NULL),
m_pOpenGLContext(NULL),
m_pOffscreenSurface(NULL),
m_pScene(NULL),
m_pFbo(NULL)
{}

SteamVRLogic::~SteamVRLogic() {
    DisconnectFromVRRuntime();
}

bool SteamVRLogic::Init() {
    bool bSuccess = true;

    QStringList arguments = qApp->arguments();

    int nNameArg = arguments.indexOf( "-name" );
    if( nNameArg != -1 && nNameArg + 2 <= arguments.size() )
    {
        m_strOverlayName = arguments.at( nNameArg + 1 );
    }

    QSurfaceFormat format;
    format.setMajorVersion( 4 );
    format.setMinorVersion( 1 );
    format.setProfile( QSurfaceFormat::CompatibilityProfile );

    m_pOpenGLContext = new QOpenGLContext();
    m_pOpenGLContext->setFormat( format );
    bSuccess = m_pOpenGLContext->create();
    if( !bSuccess )return false;

    // create an offscreen surface to attach the context and FBO to
    m_pOffscreenSurface = new QOffscreenSurface();
    m_pOffscreenSurface->create();
    m_pOpenGLContext->makeCurrent( m_pOffscreenSurface );

    m_pScene = new QGraphicsScene();
    connect( m_pScene, SIGNAL(changed(const QList<QRectF>&)), this, SLOT( OnSceneChanged(const QList<QRectF>&)) );

    bSuccess = ConnectToVRRuntime();

    bSuccess = bSuccess && vr::VRCompositor() != NULL;

    if( vr::VROverlay() )
    {
        std::string sKey = std::string( "sample." ) + m_strOverlayName.toStdString();
        vr::VROverlayError overlayError = vr::VROverlay()->CreateDashboardOverlay( sKey.c_str(), m_strOverlayName.toStdString().c_str(), &m_ulOverlayHandle, &m_ulOverlayThumbnailHandle );
        bSuccess = bSuccess && overlayError == vr::VROverlayError_None;
    }

    if( bSuccess )
    {
        vr::VROverlay()->SetOverlayWidthInMeters( m_ulOverlayHandle, 1.5f );
        vr::VROverlay()->SetOverlayInputMethod( m_ulOverlayHandle, vr::VROverlayInputMethod_Mouse );

        m_pPumpEventsTimer = new QTimer( this );
        connect(m_pPumpEventsTimer, SIGNAL( timeout() ), this, SLOT( OnTimeoutPumpEvents() ) );
        m_pPumpEventsTimer->setInterval( 20 );
        m_pPumpEventsTimer->start();

    }

    std::cout << bSuccess << std::endl;
    return bSuccess;
}

void SteamVRLogic::Shutdown() {
    vr::VR_Shutdown();
}

bool SteamVRLogic::ConnectToVRRuntime() {
    vr::IVRSystem *pVRSystem = vr::VR_Init(&m_eLastHmdError, vr::VRApplication_Overlay);

    // Underneath this line is testing code. This should print the Display and Driver name to the console... But it does not.
    char buf[128]; // Should be big enough (famous last words)
    vr::ETrackedPropertyError err;
    std::cout << pVRSystem->GetStringTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String, buf,sizeof(buf), &err) << std::endl;
    std::cout << pVRSystem->GetStringTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String, buf,sizeof(buf), &err) << std::endl;
    // End of testing code

    if (m_eLastHmdError != vr::VRInitError_None) {
        m_strVRDisplay = "No Display";
        m_strVRDriver = "No Driver";
        return false;
    }

    m_strVRDisplay = GetTrackedDeviceString(pVRSystem, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String);
    m_strVRDriver = GetTrackedDeviceString(pVRSystem, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String);

    return true;
}

void SteamVRLogic::DisconnectFromVRRuntime() {
    vr::VR_Shutdown();
}

// Qt doesn't support std::Strings so they must be converted to QStrings
QString SteamVRLogic::GetTrackedDeviceString(vr::IVRSystem *pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop) {
    char buf[128]; // Should be big enough (famous last words)
    vr::ETrackedPropertyError err;
    pHmd->GetStringTrackedDeviceProperty(unDevice, prop, buf, sizeof(buf), &err);
    if( err != vr::TrackedProp_Success )
    {
        return QString( "Error Getting String: " ) + pHmd->GetPropErrorNameFromEnum( err );
    }
    else
    {
        return buf;
    }
}

void SteamVRLogic::SetWidget( QWidget *pWidget) {
    if( m_pScene )
    {
        // all of the mouse handling stuff requires that the widget be at 0,0
        pWidget->move( 0, 0 );
        m_pScene->addWidget( pWidget );
    }
    m_Widget = pWidget;

    m_pFbo = new QOpenGLFramebufferObject(pWidget->width(), pWidget->height(), GL_TEXTURE_2D);

    if( vr::VROverlay() )
    {
        vr::HmdVector2_t vecWindowSize =
        {
            (float)pWidget->width(),
            (float)pWidget->height()
        };
        vr::VROverlay()->SetOverlayMouseScale( m_ulOverlayHandle, &vecWindowSize );
    }
}




