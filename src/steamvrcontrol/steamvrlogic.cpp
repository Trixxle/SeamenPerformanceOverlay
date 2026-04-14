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
	if (!vr::VR_IsRuntimeInstalled() || !vr::VR_IsHmdPresent()) {
		std::cerr << "SteamVR is not running or no HMD is present." << std::endl;
		return false;
	}

    bool bSuccess = true;

	m_strOverlayName = "seamen_performance_overlay";

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

	if (bSuccess && vr::VRApplications()) {
		std::string sKey = "com.seamen.overlay";
		// Not yet installed
		if (!vr::VRApplications()->IsApplicationInstalled(sKey.c_str())) {

			QString manifestPath = QApplication::applicationDirPath() + "\\manifest.vrmanifest";
			std::cout << manifestPath.toStdString() << std::endl;

			if (vr::VRApplications()->AddApplicationManifest(manifestPath.toStdString().c_str()) == vr::VRApplicationError_None) {
				vr::VRApplications()->SetApplicationAutoLaunch(sKey.c_str(), true);
			}
		}
	}


    bSuccess = bSuccess && vr::VRCompositor() != NULL;

    if( vr::VROverlay() )
    {
        std::string sKey = std::string( "com.seamen.overlay." ) + m_strOverlayName.toStdString();
		//vr::VROverlayError overlayError = vr::VROverlay()->CreateDashboardOverlay( sKey.c_str(),
		//	"Seamen Performance Overlay", &m_ulOverlayHandle, &m_ulOverlayThumbnailHandle );

    	vr::VROverlayError overlayError = vr::VROverlay()->CreateOverlay( sKey.c_str(),
					"Seamen Performance Overlay", &m_ulOverlayHandle );

    	bSuccess = bSuccess && overlayError == vr::VROverlayError_None;
    	if (overlayError != vr::VROverlayError_None) {
    		// Overlay failed to create
    		std::cerr << "Overlay Error: " << vr::VROverlay()->GetOverlayErrorNameFromEnum(overlayError);
    		return false;
    	}
		QString iconPath = QApplication::applicationDirPath() + "/icon.png";
    	vr::VROverlayError textureError = vr::VROverlay()->SetOverlayFromFile( m_ulOverlayThumbnailHandle, iconPath.toStdString().c_str() );
    	if (textureError != vr::VROverlayError_None) {
    		std::cerr << "Failed to load thumbnail icon from: " << iconPath.toStdString() << std::endl;
    	}
    }

	if( bSuccess )
	{
		vr::VROverlay()->SetOverlayWidthInMeters( m_ulOverlayHandle, 0.2f ); // Scaled down to 15cm for a watch size
		vr::VROverlay()->SetOverlayInputMethod( m_ulOverlayHandle, vr::VROverlayInputMethod_Mouse );

		vr::TrackedDeviceIndex_t leftController = m_pVRSystem->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_LeftHand);

		if (leftController != vr::k_unTrackedDeviceIndexInvalid) {
			// Attach to left hand if it's currently turned on
			AttachToDevice(leftController);
		}

		vr::VROverlay()->ShowOverlay(m_ulOverlayHandle);

		m_pPumpEventsTimer = new QTimer( this );
		connect(m_pPumpEventsTimer, SIGNAL( timeout() ), this, SLOT( OnTimeoutPumpEvents() ) );
		m_pPumpEventsTimer->setInterval( 20 );
		m_pPumpEventsTimer->start();
	}

    std::cout << "bSucces: " << bSuccess << std::endl;
    return bSuccess;
}

void SteamVRLogic::Shutdown() {
	//SaveOverlayPosition();
	DisconnectFromVRRuntime();

	delete m_pScene;
	delete m_pFbo;
	delete m_pOffscreenSurface;

	if( m_pOpenGLContext )
	{
		//		m_pOpenGLContext->destroy();
		delete m_pOpenGLContext;
		m_pOpenGLContext = NULL;
	}
}


// TODO: IMPLEMENT
void SteamVRLogic::SaveOverlayPosition() {
	if (!vr::VROverlay() || !m_pVRSystem) return;

	vr::VROverlayTransformType transformType;
	vr::VROverlayError typeError = vr::VROverlay()->GetOverlayTransformType(m_ulOverlayHandle, &transformType);

	QSettings settings("Seamen", "PerformanceOverlay");

	// Check if the user tore it off and attached it to a controller
	if (typeError == vr::VROverlayError_None && transformType == vr::VROverlayTransform_TrackedDeviceRelative) {

		std::cout << "Overlay detached" << std::endl;

		vr::TrackedDeviceIndex_t attachedDevice;
		vr::HmdMatrix34_t transform;

		if (vr::VROverlay()->GetOverlayTransformTrackedDeviceRelative(m_ulOverlayHandle, &attachedDevice, &transform) == vr::VROverlayError_None) {

			// Convert index to Role (Left Hand vs Right Hand)
			vr::ETrackedControllerRole role = m_pVRSystem->GetControllerRoleForTrackedDeviceIndex(attachedDevice);

			if (role != vr::TrackedControllerRole_Invalid) {
				settings.setValue("IsTornOff", true);
				settings.setValue("AttachedRole", (int)role);

				// Flatten the 3x4 matrix into a list for easy saving
				QList<QVariant> matrixValues;
				for (int i = 0; i < 3; ++i) {
					for (int j = 0; j < 4; ++j) {
						matrixValues.append(transform.m[i][j]);
					}
				}
				settings.setValue("TransformMatrix", matrixValues);
				return;
			}
		}
	}
	std::cout << "Overlay not detached" << std::endl;

	// If it's still in the dashboard or an error occurred, mark it as not torn off
	settings.setValue("IsTornOff", false);
}

// TODO: IMPLEMENT
void SteamVRLogic::RestoreOverlayPosition() {
	if (!vr::VROverlay() || !m_pVRSystem) return;

	QSettings settings("Seamen", "PerformanceOverlay");

	if (settings.value("IsTornOff", false).toBool()) {
		// Read the target role and convert it back to a current session device index
		vr::ETrackedControllerRole role = (vr::ETrackedControllerRole)settings.value("AttachedRole").toInt();
		vr::TrackedDeviceIndex_t deviceIndex = m_pVRSystem->GetTrackedDeviceIndexForControllerRole(role);

		if (deviceIndex != vr::k_unTrackedDeviceIndexInvalid) {
			QList<QVariant> matrixValues = settings.value("TransformMatrix").toList();

			if (matrixValues.size() == 12) {
				vr::HmdMatrix34_t transform;
				int k = 0;
				for (int i = 0; i < 3; ++i) {
					for (int j = 0; j < 4; ++j) {
						transform.m[i][j] = matrixValues[k++].toFloat();
					}
				}

				// This call effectively "tears it out" of the dashboard programmatically
				vr::VROverlay()->SetOverlayTransformTrackedDeviceRelative(m_ulOverlayHandle, deviceIndex, &transform);

				// Dashboard overlays are hidden by default when torn out programmatically
				vr::VROverlay()->ShowOverlay(m_ulOverlayHandle);
			}
		}
	}
}

void SteamVRLogic::OnSceneChanged(const QList<QRectF>&) {
	// Only render if the overlay is visible
	if ((m_ulOverlayHandle == vr::k_ulOverlayHandleInvalid ) || !vr::VROverlay() ||
		(!vr::VROverlay()->IsOverlayVisible(m_ulOverlayHandle) && !vr::VROverlay()->IsOverlayVisible(m_ulOverlayThumbnailHandle))) {
		return;
		}

	m_pOpenGLContext->makeCurrent( m_pOffscreenSurface );
	m_pFbo->bind();

	// We must clear the tender buffer when rendering transparency as otherwise you'll see ghost images from previous frames
	QOpenGLFunctions *f = m_pOpenGLContext->functions();
	f->glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Clear to fully transparent
	f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	QOpenGLPaintDevice device(m_pFbo->size());
	QPainter painter(&device);

	m_pScene->render(&painter);

	m_pFbo->release();

	GLuint unTexture = m_pFbo->texture();
	if (unTexture != 0) {
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
    	if (vrEvent.trackedDeviceIndex != vr::k_unTrackedDeviceIndexInvalid) {
    		m_unLastInteractingDevice = vrEvent.trackedDeviceIndex;
    	}

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

		case vr::VREvent_TrackedDeviceActivated:
			{
			vr::TrackedDeviceIndex_t newDeviceIndex = vrEvent.trackedDeviceIndex;
			vr::ETrackedControllerRole role = m_pVRSystem->GetControllerRoleForTrackedDeviceIndex(newDeviceIndex);

			if (role == vr::TrackedControllerRole_LeftHand) AttachToDevice(newDeviceIndex);
			//RestoreOverlayPosition();
			}
			break;

        case vr::VREvent_Quit:
            QApplication::exit();
            break;
		}
    	if (m_switchController) switchController();
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

// False for left, true for right
void SteamVRLogic::AttachToDevice(vr::TrackedDeviceIndex_t device) {
	/*
	AXx AYx AZx Tx
	AXy AYy AZy Ty
	AXz AYz AZz Tz
	*/
		vr::HmdMatrix34_t transform = {
			1.0f, /*Stretches horizontally*/ 0.0f, /*Horizontal Shear*/			0.0f, /*Unknown*/			0.0f, /*Left and right*/
			0.0f, /*Vertical Shear*/			1.0f, /*Stretches vertically*/		0.0f, /*Unknown*/			0.1f, /*Up and down*/
			0.0f, /*Rotation vertical axis*/	0.0f, /*Rotation horizontal axis*/	1.0f, /*Stretches depth*/	0.08f /*Closer and farther*/
		};
		vr::VROverlay()->SetOverlayTransformTrackedDeviceRelative(m_ulOverlayHandle, device, &transform);
}

void SteamVRLogic::switchController() {
    m_switchController = true;

    // Ensure we actually have a valid device that clicked the button
    if (m_unLastInteractingDevice != vr::k_unTrackedDeviceIndexInvalid) {

        // Attach the overlay to the controller that triggered the click
        AttachToDevice(m_unLastInteractingDevice);

        // Because AttachToDevice sets a persistent relative transform in SteamVR,
        // you don't actually need to keep calling moveOverlay() every tick.
        // We can immediately toggle the flag off to save API calls.
        m_switchController = false;

    } else {
        std::cerr << "Warning: Attempted to move overlay, but no interacting device was found." << std::endl;
        m_switchController = false;
    }
}

bool SteamVRLogic::ConnectToVRRuntime() {
	m_eLastHmdError = vr::VRInitError_None;
    m_pVRSystem = vr::VR_Init(&m_eLastHmdError, vr::VRApplication_Overlay);

    char buf[128]; // Should be big enough (famous last words)
    vr::ETrackedPropertyError err;
    std::cout << "SerialNumber: " << m_pVRSystem->GetStringTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String, buf,sizeof(buf), &err) << std::endl;
    std::cout << "TrackingSystemName: " << m_pVRSystem->GetStringTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String, buf,sizeof(buf), &err) << std::endl;
    // End of testing code

    if (m_eLastHmdError != vr::VRInitError_None) {
        m_strVRDisplay = "No Display";
        m_strVRDriver = "No Driver";
        return false;
    }

    m_strVRDisplay = GetTrackedDeviceString(m_pVRSystem, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String);
    m_strVRDriver = GetTrackedDeviceString(m_pVRSystem, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String);

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

    	QGraphicsProxyWidget *proxy = m_pScene->addWidget( pWidget );

    	// This forces Qt to render the whole overlay as a single texture, meaning one draw call. This massively improves GPU usage.
    	// Going from 20% to 1.5% GPU usage on my system
    	proxy->setCacheMode(QGraphicsItem::DeviceCoordinateCache);

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

// Given in ms
float SteamVRLogic::GetHeadsetRefreshRate()
{
	vr::ETrackedPropertyError err;
	return 1000 / m_pVRSystem->GetFloatTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_DisplayFrequency_Float, &err);
}

// Given in fps
float SteamVRLogic::GetHeadsetMaxFrameRate()
{
	vr::ETrackedPropertyError err;
	return m_pVRSystem->GetFloatTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_DisplayFrequency_Float, &err);
}
