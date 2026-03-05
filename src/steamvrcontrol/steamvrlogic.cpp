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
m_eCompositorError( vr::VRInitError_None ),
m_eOverlayError( vr::VRInitError_None ),
m_ulOverlayHandle( vr::k_ulOverlayHandleInvalid ),
m_Widget(NULL),
m_strVRDriver("No Driver"),
m_strVRDisplay("No Display"),
m_strOverlayName("Seamen Performance Overlay"),
m_pPumpEventsTimer(NULL),
m_pOpenGLContext(NULL),
m_pOffscreenSurface(NULL),
m_pScene(NULL),
m_pFbo(NULL),
m_lastMouseButtons( 0 )
{}

SteamVRLogic::~SteamVRLogic() {
    DisconnectFromVRRuntime();
}

bool SteamVRLogic::Init() {
    bool bSuccess = true;

	m_strOverlayName = "Seamen Performance Overlay";

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
	DisconnectFromVRRuntime();

	delete m_pScene;
	delete m_pFbo;
	delete m_pOffscreenSurface;

	if( m_pOpenGLContext )
	{
		//		m_pOpenGLContext->destroy();
		delete m_pOpenGLContext;
		m_pOpenGLContext = NULL;
	}}

void SteamVRLogic::OnSceneChanged(const QList<QRectF>&) {
    // Don't render if the overlay isn't visible
    if ((m_ulOverlayHandle == vr::k_ulOverlayHandleInvalid ) || !vr::VROverlay() ||
        (!vr::VROverlay()->IsOverlayVisible(m_ulOverlayHandle) && !vr::VROverlay()->IsOverlayVisible(m_ulOverlayThumbnailHandle)))
        return;

    m_pOpenGLContext->makeCurrent( m_pOffscreenSurface );
    m_pFbo->bind();

    QOpenGLPaintDevice device(m_pFbo->size());
    QPainter painter (&device);

    m_pScene ->render(&painter);

    m_pFbo->release();

    GLuint unTexture = m_pFbo->texture();
    if (unTexture != 0)
    {
        vr::Texture_t texure = {(void*)(uintptr_t)unTexture, vr::TextureType_OpenGL, vr::ColorSpace_Auto};
        vr::VROverlay()->SetOverlayTexture(m_ulOverlayHandle, &texure);
    }
}

void SteamVRLogic::OnTimeoutPumpEvents()
{
    if( !vr::VRSystem() )
		return;

	vr::VREvent_t vrEvent;
    while( vr::VROverlay()->PollNextOverlayEvent( m_ulOverlayHandle, &vrEvent, sizeof( vrEvent )  ) )
	{
		switch( vrEvent.eventType )
		{
		case vr::VREvent_MouseMove:
			{
				QPointF ptNewMouse( vrEvent.data.mouse.x, vrEvent.data.mouse.y );
				QPoint ptGlobal = ptNewMouse.toPoint();
				QGraphicsSceneMouseEvent mouseEvent( QEvent::GraphicsSceneMouseMove );
				mouseEvent.setWidget( NULL );
				mouseEvent.setPos( ptNewMouse );
				mouseEvent.setScenePos( ptGlobal );
				mouseEvent.setScreenPos( ptGlobal );
				mouseEvent.setLastPos( m_tLastMouse );
				mouseEvent.setLastScenePos( m_Widget->mapToGlobal( m_tLastMouse.toPoint() ) );
				mouseEvent.setLastScreenPos( m_Widget->mapToGlobal( m_tLastMouse.toPoint() ) );
				mouseEvent.setButtons( m_lastMouseButtons );
				mouseEvent.setButton( Qt::NoButton );
				mouseEvent.setModifiers( ( Qt::KeyboardModifiers)0 );
				mouseEvent.setAccepted( false );

				m_tLastMouse = ptNewMouse;
				QApplication::sendEvent( m_pScene, &mouseEvent );

				OnSceneChanged( QList<QRectF>() );
			}
			break;

		case vr::VREvent_MouseButtonDown:
			{
				Qt::MouseButton button = vrEvent.data.mouse.button == vr::VRMouseButton_Right ? Qt::RightButton : Qt::LeftButton;

				m_lastMouseButtons |= button;

				QPoint ptGlobal = m_tLastMouse.toPoint();
				QGraphicsSceneMouseEvent mouseEvent( QEvent::GraphicsSceneMousePress );
				mouseEvent.setWidget( NULL );
				mouseEvent.setPos( m_tLastMouse );
				mouseEvent.setButtonDownPos( button, m_tLastMouse );
				mouseEvent.setButtonDownScenePos( button, ptGlobal);
				mouseEvent.setButtonDownScreenPos( button, ptGlobal );
				mouseEvent.setScenePos( ptGlobal );
				mouseEvent.setScreenPos( ptGlobal );
				mouseEvent.setLastPos( m_tLastMouse );
				mouseEvent.setLastScenePos( ptGlobal );
				mouseEvent.setLastScreenPos( ptGlobal );
				mouseEvent.setButtons( m_lastMouseButtons );
				mouseEvent.setButton( button );
				mouseEvent.setModifiers( ( Qt::KeyboardModifiers ) 0 );
				mouseEvent.setAccepted( false );

				QApplication::sendEvent( m_pScene, &mouseEvent );
			}
			break;

		case vr::VREvent_MouseButtonUp:
			{
				Qt::MouseButton button = vrEvent.data.mouse.button == vr::VRMouseButton_Right ? Qt::RightButton : Qt::LeftButton;
				m_lastMouseButtons &= ~button;

				QPoint ptGlobal = m_tLastMouse.toPoint();
				QGraphicsSceneMouseEvent mouseEvent( QEvent::GraphicsSceneMouseRelease );
				mouseEvent.setWidget( NULL );
				mouseEvent.setPos( m_tLastMouse );
				mouseEvent.setScenePos( ptGlobal );
				mouseEvent.setScreenPos( ptGlobal );
				mouseEvent.setLastPos( m_tLastMouse );
				mouseEvent.setLastScenePos( ptGlobal );
				mouseEvent.setLastScreenPos( ptGlobal );
				mouseEvent.setButtons( m_lastMouseButtons );
				mouseEvent.setButton( button );
				mouseEvent.setModifiers( ( Qt::KeyboardModifiers )0 );
				mouseEvent.setAccepted( false );

				QApplication::sendEvent(  m_pScene, &mouseEvent );
			}
			break;

		case vr::VREvent_OverlayShown:
			{
				m_Widget->repaint();
			}
			break;

        case vr::VREvent_Quit:
            QApplication::exit();
            break;
		}
	}

    if( m_ulOverlayThumbnailHandle != vr::k_ulOverlayHandleInvalid )
    {
        while( vr::VROverlay()->PollNextOverlayEvent( m_ulOverlayThumbnailHandle, &vrEvent, sizeof( vrEvent)  ) )
        {
            switch( vrEvent.eventType )
            {
            case vr::VREvent_OverlayShown:
                {
                    m_Widget->repaint();
                }
                break;
            }
        }
    }

}

bool SteamVRLogic::ConnectToVRRuntime() {
	m_eLastHmdError = vr::VRInitError_None;
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
        pWidget->move(0,0);
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

vr::HmdError SteamVRLogic::GetLastHmdError()
{
	return m_eLastHmdError;
}

bool SteamVRLogic::BHMDAvailable()
{
	return vr::VRSystem() != NULL;
}

QString SteamVRLogic::GetVRDriverString()
{
	return m_strVRDriver;
}

QString SteamVRLogic::GetVRDisplayString()
{
	return m_strVRDisplay;
}