/*
Original work Copyright (c) 2015, Valve Corporation. All rights reserved.
Modified work Copyright (C) 2026 Jorn ten Kate, The Seamen.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

-------------------------------------------------------------------------------

This program is also licensed under the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version.

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
m_eLastHmdError(vr::VRInitError_None),
m_eCompositorError( vr::VRInitError_None ),
m_eOverlayError( vr::VRInitError_None ),
m_ePanicOverlayError( vr::VRInitError_None ),
m_ulOverlayHandle( vr::k_ulOverlayHandleInvalid ),
m_ulPanicOverlayHandle( vr::k_ulOverlayHandleInvalid ),
m_overlayPositionMatrix({
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.866f, 0.5f, 0.1f,
	0.0f, -0.5f, 0.866f, -0.08f
	}),
m_pWidget(NULL),
m_strVRDriver("No Driver"),
m_strVRDisplay("No Display"),
m_strOverlayName("Seamen Performance Overlay"),
m_pPumpEventsTimer(NULL),
m_pRenderTimer(NULL),
m_pOpenGLContext(NULL),
m_pOffscreenSurface(NULL),
m_pScene(NULL),
m_pPanicScene(NULL),
m_pFbo(NULL),
m_pPanicFbo(NULL),
m_lastMouseButtons( 0 ),
m_settings("Seamen", "PerformanceOverlay")
{}

SteamVRLogic::~SteamVRLogic() {
    DisconnectFromVRRuntime();
}

bool SteamVRLogic::Init() {
	if (!vr::VR_IsRuntimeInstalled()) {
		std::cerr << "SteamVR is not running." << std::endl;
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
    if( !bSuccess ) {
		std::cout << "Failed to initialize OpenGL context." << std::endl;
    	return false;
    }

    // create an offscreen surface to attach the context and FBO to
    m_pOffscreenSurface = new QOffscreenSurface();
    m_pOffscreenSurface->create();
    m_pOpenGLContext->makeCurrent( m_pOffscreenSurface );

    m_pScene = new QGraphicsScene();
    connect( m_pScene, SIGNAL(changed(const QList<QRectF>&)), this, SLOT( OnSceneChanged(const QList<QRectF>&)) );

	m_pPanicScene = new QGraphicsScene();
	connect( m_pPanicScene, SIGNAL(changed(const QList<QRectF>&)), this, SLOT( OnPanicSceneChanged(const QList<QRectF>&)) );

    bSuccess = ConnectToVRRuntime();

	if (!bSuccess) {
		int attempt = 0;

		while (attempt < MAX_VRRUNTIME_CONNECTION_ATTEMPTS && !bSuccess) {
			bSuccess = ConnectToVRRuntime();
			++attempt;
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}

		if (!bSuccess) {
			std::cerr << "Failed to connect to VR runtime." << std::endl;
			return false;
		}
	}

	// The following only really applies if ths app is sideloaded. When downloading from steam the app will
	// automatically be added to SteamVR.
	// Check if the app is installed to SteamVR already. If not, add it and turn on autostart
	if (vr::VRApplications()) {
		std::string sKey = "steam.overlay.4666560";

		if (!vr::VRApplications()->IsApplicationInstalled(sKey.c_str())) {

			QString manifestPath = QApplication::applicationDirPath() + "/manifest.vrmanifest";

			manifestPath = QDir::toNativeSeparators(manifestPath);

			if (!QFile::exists(manifestPath)) {
				std::cerr << "Manifest file not found: " << manifestPath.toStdString() << std::endl;
			} else {
				std::cout << manifestPath.toStdString() << std::endl;

				std::string utf8Path = manifestPath.toUtf8().constData();

				vr::EVRApplicationError err = vr::VRApplications()->AddApplicationManifest(utf8Path.c_str());

				if (err == vr::VRApplicationError_None) {
					vr::VRApplications()->SetApplicationAutoLaunch(sKey.c_str(), true);
					std::cout << "Manifest successfully registered and set to auto-launch." << std::endl;
				} else {
					std::cerr << "Failed to add manifest. Error: "
							  << vr::VRApplications()->GetApplicationsErrorNameFromEnum(err) << std::endl;
				}
			}
		}
	}

    bSuccess = vr::VRCompositor() != NULL;

	if (!bSuccess) std::cerr << "Failed to initialize Compositor." << std::endl;

    if( vr::VROverlay() )
    {
        std::string sPanicKey = std::string( "steam.overlay.4666560" ); // + m_strOverlayName.toStdString();
    	std::string sKey = sPanicKey + ".nonPanic";

		vr::VROverlayError overlayErrorPanic = vr::VROverlay()->CreateDashboardOverlay( sPanicKey.c_str(),
			"Seamen Performance Overlay", &m_ulPanicOverlayHandle, &m_ulOverlayThumbnailHandle );

    	vr::VROverlayError overlayError = vr::VROverlay()->CreateOverlay( sKey.c_str(),
					"Seamen Performance Overlay", &m_ulOverlayHandle );

    	bSuccess = bSuccess && overlayError == vr::VROverlayError_None && overlayErrorPanic == vr::VROverlayError_None;
    	if (overlayError != vr::VROverlayError_None || overlayErrorPanic != vr::VROverlayError_None) {
    		// Overlays failed to create
    		std::cerr << "Overlay Error: " << vr::VROverlay()->GetOverlayErrorNameFromEnum(overlayError);
    		std::cerr << "Panic Overlay Error: " << vr::VROverlay()->GetOverlayErrorNameFromEnum(overlayErrorPanic);
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
		m_overlayWidthInMeters = 0.2f;
		vr::VROverlay()->SetOverlayWidthInMeters( m_ulOverlayHandle, m_overlayWidthInMeters );
		vr::VROverlay()->SetOverlayWidthInMeters( m_ulPanicOverlayHandle, 2.0 );

		vr::VROverlay()->SetOverlayInputMethod( m_ulOverlayHandle, vr::VROverlayInputMethod_Mouse );
		vr::VROverlay()->SetOverlayInputMethod( m_ulPanicOverlayHandle, vr::VROverlayInputMethod_Mouse );

		m_leftController = m_pVRSystem->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_LeftHand);
		m_rightController = m_pVRSystem->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_RightHand);

		restoreSession();

		// Some IDEs will say the following statement will always be false, this is incorrect
		if (m_deviceOverlayIsAttachedTo == vr::k_unTrackedDeviceIndexInvalid &&
			m_leftController != vr::k_unTrackedDeviceIndexInvalid) {
			AttachToDevice(m_leftController);
		}

		vr::VROverlay()->ShowOverlay(m_ulOverlayHandle);

		m_pPumpEventsTimer = new QTimer( this );
		m_pRenderTimer = new QTimer( this );
		m_pRenderTimer->setTimerType(Qt::CoarseTimer);
		connect(m_pPumpEventsTimer, SIGNAL( timeout() ), this, SLOT( OnTimeoutPumpEvents() ) );
		connect(m_pRenderTimer, &QTimer::timeout, this, &SteamVRLogic::RenderDirtyOverlayScenes);
		m_pPumpEventsTimer->setInterval( 20 );
		m_pPumpEventsTimer->start();

		// The quickest updating UI element is the text, which happens at 100ms intervals.
		// Rendering above that frequency would be wasted performance
		// Yes, this makes the buttons slightly less responsive but at 100ms interval they still feel fine, it is worth the
		// Trade-off
		m_pRenderTimer->setInterval(100);
		m_pRenderTimer->start();
	}
	else {
		std::cerr << "Failed to initialize VR overlay." << std::endl;
	}
    std::cout << "bSucces: " << bSuccess << std::endl;
    return bSuccess;
}

void SteamVRLogic::setBaseAlpha(float alpha)
{
	m_baseAlpha = std::max(0.0f, std::min(1.0f, alpha));
}

// Why limit this? Allow the user to increase it as much as they want, let them be free. They will play themselves
void SteamVRLogic::increaseOverlayScale() {
	m_overlayWidthInMeters += 0.01;
	updateOverlayWidthInMeters();
	saveSize();
}

// Unfortuantely, we do have to limit this to stop users from being too stupid
void SteamVRLogic::decreaseOverlayScale() {
	// Size can't be 0, then it breaks
	if (m_overlayWidthInMeters - 0.01 < 0.01) {
		m_overlayWidthInMeters = 0.01;
	}
	else m_overlayWidthInMeters -= 0.01;

	updateOverlayWidthInMeters();
	saveSize();
}

void SteamVRLogic::updateOverlayWidthInMeters() {
	vr::VROverlay()->SetOverlayWidthInMeters( m_ulOverlayHandle, m_overlayWidthInMeters );
}

void SteamVRLogic::resetOverlayToDefault() {
	m_overlayPositionMatrix = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.866f, 0.5f, 0.1f,
		0.0f, -0.5f, 0.866f, -0.08f
	};
	m_overlayWidthInMeters = 0.2f;
	if (m_leftController != vr::k_unTrackedDeviceIndexInvalid) AttachToDevice(m_leftController);
	else if (m_rightController != vr::k_unTrackedDeviceIndexInvalid) AttachToDevice(m_rightController);
	else AttachToDevice(vr::k_unTrackedDeviceIndexInvalid);
	updateOverlayWidthInMeters();
}

void SteamVRLogic::Shutdown() {
	saveSession();
	DisconnectFromVRRuntime();

	delete m_pScene;
	delete m_pPanicScene;
	delete m_pFbo;
	delete m_pPanicFbo;
	delete m_pOffscreenSurface;

	if( m_pOpenGLContext )
	{
		delete m_pOpenGLContext;
		m_pOpenGLContext = NULL;
	}
}

// Important to note, the opacity is saved in dashboard.cpp as it is handled by Qt
void SteamVRLogic::saveSession() {
	if (!vr::VROverlay() || !m_pVRSystem) return;
	saveSize();
	saveController();
	savePosition();
	emit saveOpacity();
}

void SteamVRLogic::savePosition() {
	// Attaching to HMD is just a fallback if no controller can be found. We don't want to save that as the last
	// known position
	if (m_deviceOverlayIsAttachedTo == vr::k_unTrackedDeviceIndex_Hmd) return;
	vr::VROverlayTransformType transformType;
	vr::VROverlayError typeError = vr::VROverlay()->GetOverlayTransformType(m_ulOverlayHandle, &transformType);
	if (typeError) return;

	// Flatten the 3x4 matrix into a list for easy saving
	QList<QVariant> matrixValues;
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 4; ++j) {
			matrixValues.append(m_overlayPositionMatrix.m[i][j]);
		}
	}
	m_settings.setValue("TransformMatrix", matrixValues);
}

void SteamVRLogic::saveSize() {
	m_settings.setValue("Size", m_overlayWidthInMeters);
}

void SteamVRLogic::saveController() {
	if (m_deviceOverlayIsAttachedTo == m_leftController) {
		m_settings.setValue("AttachedRole", vr::TrackedControllerRole_LeftHand);
	} else if (m_deviceOverlayIsAttachedTo == m_rightController) {
		m_settings.setValue("AttachedRole", vr::TrackedControllerRole_RightHand);
	}
}

// IMPORTANT NOTE: Opacity is restored directly in dashboard.cpp as this code is ran before the widget exists
void SteamVRLogic::restoreSession() {
	if (!vr::VROverlay() || !m_pVRSystem) return;

	// Restore position
	if (!m_settings.value("TransformMatrix").isNull()) {
		QList<QVariant> matrixValues = m_settings.value("TransformMatrix").toList();
		if (matrixValues.size() == 12) {
			int k = 0;
			for (int i = 0; i < 3; ++i) {
				for (int j = 0; j < 4; ++j) {
					m_overlayPositionMatrix.m[i][j] = matrixValues[k++].toFloat();
				}
			}
		}
	}

	// Restore controller attached to
	if (!m_settings.value("AttachedRole").isNull()) {
		int savedRole = m_settings.value("AttachedRole").toInt();
		vr::TrackedDeviceIndex_t device = m_pVRSystem->GetTrackedDeviceIndexForControllerRole(
			static_cast<vr::ETrackedControllerRole>(savedRole));
		if (device != vr::k_unTrackedDeviceIndexInvalid) {
			AttachToDevice(device);
		}
	}

	// Restore size
	if (!m_settings.value("Size").isNull()) {
		m_overlayWidthInMeters = m_settings.value("Size").toFloat();
		updateOverlayWidthInMeters();
	}
}

void SteamVRLogic::OnSceneChanged(const QList<QRectF>&) {
	// Just mark dirty. The actual GPU render is batched in OnTimeoutPumpEvents
	// to prevent multiple FBO renders per timer tick when the UI updates several
	// items at once, like labels, charts, etc.
	m_mainSceneDirty = true;
}

void SteamVRLogic::OnPanicSceneChanged(const QList<QRectF>&) {
	// Just mark dirty. Rendered in OnTimeoutPumpEvents alongside the main scene.
	m_panicSceneDirty = true;
}

// Performs the OpenGL FBO render and texture upload to OpenVR for any
// scene that has been marked dirty since the last render update tick.
// Called once per timer tick so that multiple scene-changed signals within a
// single tick collapse into one GPU render instead of two, as there are two overlays
void SteamVRLogic::RenderDirtyOverlayScenes() {
	if (!vr::VROverlay()) return;

	bool mainVisible = m_mainSceneDirty
		&& m_ulOverlayHandle != vr::k_ulOverlayHandleInvalid
		&& vr::VROverlay()->IsOverlayVisible(m_ulOverlayHandle)
		&& m_lastAlpha > 0.01f;

	bool panicVisible = m_panicSceneDirty
		&& m_ulPanicOverlayHandle != vr::k_ulOverlayHandleInvalid
		&& vr::VROverlay()->IsOverlayVisible(m_ulPanicOverlayHandle);

	if (!mainVisible && !panicVisible) return;

	// Make context current once for both scenes
	m_pOpenGLContext->makeCurrent(m_pOffscreenSurface);
	QOpenGLFunctions *f = m_pOpenGLContext->functions();

	if (mainVisible && m_pFbo) {
		m_pFbo->bind();
		f->glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		f->glClear(GL_COLOR_BUFFER_BIT);

		QOpenGLPaintDevice device(m_pFbo->size());
		QPainter painter(&device);
		m_pScene->render(&painter);
		painter.end();
		m_pFbo->release();

		GLuint unTexture = m_pFbo->texture();
		if (unTexture != 0) {
			vr::Texture_t texture = {(void*)(uintptr_t)unTexture, vr::TextureType_OpenGL, vr::ColorSpace_Auto};
			vr::VROverlay()->SetOverlayTexture(m_ulOverlayHandle, &texture);
		}
		m_mainSceneDirty = false;
	}

	if (panicVisible && m_pPanicFbo) {
		m_pPanicFbo->bind();
		f->glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		f->glClear(GL_COLOR_BUFFER_BIT);

		QOpenGLPaintDevice panicDevice(m_pPanicFbo->size());
		QPainter panicPainter(&panicDevice);
		m_pPanicScene->render(&panicPainter);
		panicPainter.end();
		m_pPanicFbo->release();

		GLuint unPanicTexture = m_pPanicFbo->texture();
		if (unPanicTexture != 0) {
			vr::Texture_t texture = {(void*)(uintptr_t)unPanicTexture, vr::TextureType_OpenGL, vr::ColorSpace_Auto};
			vr::VROverlay()->SetOverlayTexture(m_ulPanicOverlayHandle, &texture);
		}
		m_panicSceneDirty = false;
	}
}

void SteamVRLogic::OnTimeoutPumpEvents()
{
    if( !vr::VRSystem() )
		return;

	// During autostart, controllers may not be enumerated when Init() runs.
	// VREvent_TrackedDeviceActivated is missed because the overlay doesn't exist yet when
	// controllers first connect. Poll here until we get a valid attachment.
	if (m_deviceOverlayIsAttachedTo == vr::k_unTrackedDeviceIndexInvalid && m_bindToControllerAttempts <= 10) {
		if (m_leftController == vr::k_unTrackedDeviceIndexInvalid)
			m_leftController = m_pVRSystem->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_LeftHand);
		if (m_rightController == vr::k_unTrackedDeviceIndexInvalid)
			m_rightController = m_pVRSystem->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_RightHand);

		if (m_leftController != vr::k_unTrackedDeviceIndexInvalid || m_rightController != vr::k_unTrackedDeviceIndexInvalid) {
			restoreSession();
			if (m_deviceOverlayIsAttachedTo == vr::k_unTrackedDeviceIndexInvalid) {
				vr::TrackedDeviceIndex_t fallback = (m_leftController != vr::k_unTrackedDeviceIndexInvalid)
					? m_leftController : m_rightController;
				AttachToDevice(fallback);
			}
		}
		// Wait 250ms and try to find a controller again
		std::this_thread::sleep_for(std::chrono::milliseconds(250));
		// If no controller has been found after 10 tries, just attach to the HMD
		if (m_bindToControllerAttempts >= 10) {
			AttachToDevice(vr::k_unTrackedDeviceIndex_Hmd);
		}
		++m_bindToControllerAttempts;
	}

	vr::VREvent_t vrEvent;

	// Process events for one overlay, dispatching mouse events to the given scene and widget
	auto processOverlayEvents = [&](vr::VROverlayHandle_t handle, QGraphicsScene* scene, QWidget* widget) {
		while( vr::VROverlay()->PollNextOverlayEvent( handle, &vrEvent, sizeof( vrEvent ) ) )
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
					mouseEvent.setLastScenePos( widget->mapToGlobal( m_tLastMouse.toPoint() ) );
					mouseEvent.setLastScreenPos( widget->mapToGlobal( m_tLastMouse.toPoint() ) );
					mouseEvent.setButtons( m_lastMouseButtons );
					mouseEvent.setButton( Qt::NoButton );
					mouseEvent.setModifiers( (Qt::KeyboardModifiers)0 );
					mouseEvent.setAccepted( false );

					m_tLastMouse = ptNewMouse;
					QApplication::sendEvent( scene, &mouseEvent );
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
					mouseEvent.setButtonDownScenePos( button, ptGlobal );
					mouseEvent.setButtonDownScreenPos( button, ptGlobal );
					mouseEvent.setScenePos( ptGlobal );
					mouseEvent.setScreenPos( ptGlobal );
					mouseEvent.setLastPos( m_tLastMouse );
					mouseEvent.setLastScenePos( ptGlobal );
					mouseEvent.setLastScreenPos( ptGlobal );
					mouseEvent.setButtons( m_lastMouseButtons );
					mouseEvent.setButton( button );
					mouseEvent.setModifiers( (Qt::KeyboardModifiers)0 );
					mouseEvent.setAccepted( false );

					QApplication::sendEvent( scene, &mouseEvent );
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
					mouseEvent.setModifiers( (Qt::KeyboardModifiers)0 );
					mouseEvent.setAccepted( false );

					QApplication::sendEvent( scene, &mouseEvent );

					if (m_isMoving) {
						stopMove();
						m_isMoving = false;
					}
				}
				break;

			case vr::VREvent_OverlayShown:
				widget->repaint();
				break;

			case vr::VREvent_TrackedDeviceActivated:
				{
					vr::TrackedDeviceIndex_t newDeviceIndex = vrEvent.trackedDeviceIndex;
					vr::ETrackedControllerRole role = m_pVRSystem->GetControllerRoleForTrackedDeviceIndex(newDeviceIndex);

					if (role == vr::TrackedControllerRole_LeftHand) {
						m_leftController = newDeviceIndex;
					}
					if (role == vr::TrackedControllerRole_RightHand) {
						m_rightController = newDeviceIndex;
					}
					// Check if the overlay is not attached to anything, or if it is attached to the HMD (fallback) and
					// the left controller is now not invalid, or if it is attached to HMD (fallback) and the right
					// controller is now not invalid. If any of these is true, restore the session and attach to the
					// controller saved from previous session
					if (m_deviceOverlayIsAttachedTo == vr::k_unTrackedDeviceIndexInvalid ||
						m_deviceOverlayIsAttachedTo == vr::k_unTrackedDeviceIndex_Hmd && m_leftController != vr::k_unTrackedDeviceIndexInvalid ||
						m_deviceOverlayIsAttachedTo == vr::k_unTrackedDeviceIndex_Hmd && m_rightController != vr::k_unTrackedDeviceIndexInvalid) {
						restoreSession();
					}
				}
				break;

			case vr::VREvent_Quit:
				QApplication::exit();
				break;
			}
		}
	};

	processOverlayEvents( m_ulOverlayHandle, m_pScene, m_pWidget );
	processOverlayEvents( m_ulPanicOverlayHandle, m_pPanicScene, m_pPanicWidget );

	// Update overlay alpha based on the viewing angle so the overlay fades when seen edge-on
	if (vr::VROverlay()
		&& m_ulOverlayHandle != vr::k_ulOverlayHandleInvalid
		&& m_deviceOverlayIsAttachedTo != vr::k_unTrackedDeviceIndexInvalid
		&& vr::VROverlay()->IsOverlayVisible(m_ulOverlayHandle))
	{
		vr::TrackedDevicePose_t poses[vr::k_unMaxTrackedDeviceCount];
		// Query only up to the highest index we need (HMD=0, controller=small index).
		// This avoids filling all 64 pose slots when only 2 are read.
		uint32_t poseCount = m_deviceOverlayIsAttachedTo + 1;
		m_pVRSystem->GetDeviceToAbsoluteTrackingPose(vr::TrackingUniverseStanding, 0.0f, poses, poseCount);

		const vr::TrackedDevicePose_t& hmdPose    = poses[vr::k_unTrackedDeviceIndex_Hmd];
		const vr::TrackedDevicePose_t& devicePose = poses[m_deviceOverlayIsAttachedTo];

		if (hmdPose.bPoseIsValid && devicePose.bPoseIsValid)
		{
			// When attached to the HMD the overlay is always facing the user; angle-based
			// fading does not apply. devicePose == hmdPose (both index 0), so the
			// overlay-to-HMD vector is near zero and would yield a spurious alpha of 0.
			// Apply base alpha directly and skip the angle computation.
			if (m_deviceOverlayIsAttachedTo == vr::k_unTrackedDeviceIndex_Hmd)
			{
				if (std::abs(m_baseAlpha - m_lastAlpha) > 0.005f)
				{
					vr::VROverlay()->SetOverlayAlpha(m_ulOverlayHandle, m_baseAlpha);
					m_lastAlpha = m_baseAlpha;
				}
			}
			else {
				// Build the overlay's world transform: deviceWorld * overlayRelative
				const vr::HmdMatrix34_t& dw = devicePose.mDeviceToAbsoluteTracking;
				const vr::HmdMatrix34_t& rel = m_overlayPositionMatrix;

				// World position of overlay (translation column of dw * rel)
				float ox = dw.m[0][0]*rel.m[0][3] + dw.m[0][1]*rel.m[1][3] + dw.m[0][2]*rel.m[2][3] + dw.m[0][3];
				float oy = dw.m[1][0]*rel.m[0][3] + dw.m[1][1]*rel.m[1][3] + dw.m[1][2]*rel.m[2][3] + dw.m[1][3];
				float oz = dw.m[2][0]*rel.m[0][3] + dw.m[2][1]*rel.m[1][3] + dw.m[2][2]*rel.m[2][3] + dw.m[2][3];

				// Z-axis of overlay in world space (third column of the 3x3 rotation block of dw * rel)
				float wzx = dw.m[0][0]*rel.m[0][2] + dw.m[0][1]*rel.m[1][2] + dw.m[0][2]*rel.m[2][2];
				float wzy = dw.m[1][0]*rel.m[0][2] + dw.m[1][1]*rel.m[1][2] + dw.m[1][2]*rel.m[2][2];
				float wzz = dw.m[2][0]*rel.m[0][2] + dw.m[2][1]*rel.m[1][2] + dw.m[2][2]*rel.m[2][2];

				// The visible (front) face of an OpenVR overlay faces in the +Z direction of its local frame
				float nx = wzx, ny = wzy, nz = wzz;
				float nLen = std::sqrt(nx*nx + ny*ny + nz*nz);
				if (nLen > 0.0001f) { nx /= nLen; ny /= nLen; nz /= nLen; }

				// Vector from overlay centre to HMD
				float dx = hmdPose.mDeviceToAbsoluteTracking.m[0][3] - ox;
				float dy = hmdPose.mDeviceToAbsoluteTracking.m[1][3] - oy;
				float dz = hmdPose.mDeviceToAbsoluteTracking.m[2][3] - oz;
				float dLen = std::sqrt(dx*dx + dy*dy + dz*dz);
				if (dLen > 0.0001f) { dx /= dLen; dy /= dLen; dz /= dLen; }

				// cosAngle = 1 when facing head-on, 0 at 90 degrees (edge-on)
				float cosAngle = nx*dx + ny*dy + nz*dz;

				// Full opacity within 30 degrees, fades to zero at 50 degrees
				constexpr float cosStart = 0.866025f; // cos(30°)
				constexpr float cosEnd   = 0.642787f; // cos(50°)
				float angleFactor = (cosAngle - cosEnd) / (cosStart - cosEnd);
				angleFactor = std::max(0.0f, std::min(1.0f, angleFactor));

				// Only call the API if the alpha actually changed, to avoid redundant
				// OpenVR calls every 20ms when the viewing angle is stable.
				float newAlpha = m_baseAlpha * angleFactor;
				if (std::abs(newAlpha - m_lastAlpha) > 0.005f) {
					vr::VROverlay()->SetOverlayAlpha(m_ulOverlayHandle, newAlpha);
					m_lastAlpha = newAlpha;
				}
			}
		}
	}

    if( m_ulOverlayThumbnailHandle != vr::k_ulOverlayHandleInvalid )
    {
        while( vr::VROverlay()->PollNextOverlayEvent( m_ulOverlayThumbnailHandle, &vrEvent, sizeof( vrEvent)  ) )
        {
            switch( vrEvent.eventType )
            {
            case vr::VREvent_OverlayShown:
                m_pPanicWidget->repaint();
                break;
            }
        }
    }
}

void SteamVRLogic::AttachToDevice(const vr::TrackedDeviceIndex_t& device) {
	// For reference, matrix format:
	/*
	AXx AYx AZx Tx
	AXy AYy AZy Ty
	AXz AYz AZz Tz
	*/

	// Overlay will default to attaching to HMD if no controllers are found, but the restored matrix in m_overlayPositionMatrix
	// is almost certainly from a controller and we don't want to overwrite the restored value in case a controller
	// connects. This if statement is therefore needed to use a temporary fallback matrix transform to attach with.
	if (device == vr::k_unTrackedDeviceIndex_Hmd) {
		vr::HmdMatrix34_t hmdMatrix = {
			1.0f, 0.0f, 0.0f,  0.0f,  /*Left and right*/
			0.0f, 1.0f, 0.0f, -0.2f,  /*Up and down*/
			0.0f, 0.0f, 1.0f, -0.5f   /*Closer and farther (-Z is forward in HMD space)*/
		};
		vr::VROverlay()->SetOverlayTransformTrackedDeviceRelative(m_ulOverlayHandle, device, &hmdMatrix);
	}
	else vr::VROverlay()->SetOverlayTransformTrackedDeviceRelative(m_ulOverlayHandle, device, &m_overlayPositionMatrix);
	m_deviceOverlayIsAttachedTo = device;
}

void SteamVRLogic::switchController() {
	// Ensure we actually have a valid device that clicked the button if by some miracle the user happens to somehow
	// click the button with an invalid device
	if (m_unLastInteractingDevice != vr::k_unTrackedDeviceIndexInvalid) {
		// Attach the overlay to the controller that triggered the click
		mirrorMatrix();
		AttachToDevice(m_unLastInteractingDevice);

		m_isMoving = false;
	} else {
		std::cerr << "Warning: You have somehow done something that should be impossible. You clicked the switch controller"
		" button with an invalid device." << std::endl;
	}
}

void SteamVRLogic::startMove() {
	// If widget is already moving, cannot start moving it again
	if (m_isMoving) return;

	// Re-query controller indices — they may have been invalid at init time during SteamVR autostart
	if (m_leftController == vr::k_unTrackedDeviceIndexInvalid) {
		m_leftController = m_pVRSystem->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_LeftHand);
	}
	if (m_rightController == vr::k_unTrackedDeviceIndexInvalid) {
		m_rightController = m_pVRSystem->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_RightHand);
	}

	if (m_deviceOverlayIsAttachedTo == m_leftController) {
		if (m_rightController == vr::k_unTrackedDeviceIndexInvalid) {
			std::cerr << "startMove: right controller not yet tracked, cannot start move." << std::endl;
			return;
		}
		m_overlayPositionMatrix = calculateRelativeTransform(m_rightController);
		AttachToDevice(m_rightController);
		m_isMoving = true;
	}
	else {
		if (m_leftController == vr::k_unTrackedDeviceIndexInvalid) {
			std::cerr << "startMove: left controller not yet tracked, cannot start move." << std::endl;
			return;
		}
		m_overlayPositionMatrix = calculateRelativeTransform(m_leftController);
		AttachToDevice(m_leftController);
		m_isMoving = true;
	}
}

void SteamVRLogic::stopMove() {
	if (m_deviceOverlayIsAttachedTo == m_rightController) {
		m_overlayPositionMatrix = calculateRelativeTransform(m_leftController);
		AttachToDevice(m_leftController);
	}
	else {
		m_overlayPositionMatrix = calculateRelativeTransform(m_rightController);
		AttachToDevice(m_rightController);
	}
	savePosition();
	saveController();
}

void SteamVRLogic::mirrorMatrix() {
	m_overlayPositionMatrix.m[0][3] = -m_overlayPositionMatrix.m[0][3];
	m_overlayPositionMatrix.m[0][1] = -m_overlayPositionMatrix.m[0][1];
	m_overlayPositionMatrix.m[0][2] = -m_overlayPositionMatrix.m[0][2];
	m_overlayPositionMatrix.m[1][0] = -m_overlayPositionMatrix.m[1][0];
	m_overlayPositionMatrix.m[2][0] = -m_overlayPositionMatrix.m[2][0];
}


// -- NOTE: The below code is made almost entirely made by Claude. I was too lazy to refresh my math on matrices -- //

// Calculates the relative transform between the overlay and the passed device
vr::HmdMatrix34_t SteamVRLogic::calculateRelativeTransform(vr::TrackedDeviceIndex_t device) {
    // Fallback to the current matrix if anything goes wrong
    vr::HmdMatrix34_t fallbackMatrix = m_overlayPositionMatrix;

    if (!vr::VROverlay() || !m_pVRSystem || device == vr::k_unTrackedDeviceIndexInvalid) {
        return fallbackMatrix;
    }

    // Helper lambdas to cleanly convert between OpenVR's 3x4 and Qt's 4x4 matrices
    auto toQMatrix = [](const vr::HmdMatrix34_t& mat) {
        return QMatrix4x4(
            mat.m[0][0], mat.m[0][1], mat.m[0][2], mat.m[0][3],
            mat.m[1][0], mat.m[1][1], mat.m[1][2], mat.m[1][3],
            mat.m[2][0], mat.m[2][1], mat.m[2][2], mat.m[2][3],
            0.0f,        0.0f,        0.0f,        1.0f
        );
    };

    auto toVrMatrix = [](const QMatrix4x4& mat) {
        vr::HmdMatrix34_t res;
        for(int i = 0; i < 3; ++i) {
            for(int j = 0; j < 4; ++j) {
                res.m[i][j] = mat(i, j);
            }
        }
        return res;
    };

    vr::TrackingUniverseOrigin trackingOrigin = vr::TrackingUniverseStanding;
    QMatrix4x4 overlayAbsoluteTransform;

    // Determine the current absolute transform of the overlay in the tracking space
    vr::VROverlayTransformType transformType;
    if (vr::VROverlay()->GetOverlayTransformType(m_ulOverlayHandle, &transformType) != vr::VROverlayError_None) {
        return fallbackMatrix;
    }

    if (transformType == vr::VROverlayTransform_Absolute) {
        // Overlay is currently pinned to the world
        vr::HmdMatrix34_t absMat;
        vr::VROverlay()->GetOverlayTransformAbsolute(m_ulOverlayHandle, &trackingOrigin, &absMat);
        overlayAbsoluteTransform = toQMatrix(absMat);
    }
    else if (transformType == vr::VROverlayTransform_TrackedDeviceRelative) {
        // Overlay is currently pinned to another device
        vr::TrackedDeviceIndex_t currentParentDevice;
        vr::HmdMatrix34_t currentRelativeMat;
        vr::VROverlay()->GetOverlayTransformTrackedDeviceRelative(m_ulOverlayHandle, &currentParentDevice, &currentRelativeMat);

        vr::TrackedDevicePose_t poses[vr::k_unMaxTrackedDeviceCount];
        m_pVRSystem->GetDeviceToAbsoluteTrackingPose(trackingOrigin, 0.0f, poses, vr::k_unMaxTrackedDeviceCount);

        if (!poses[currentParentDevice].bPoseIsValid) {
            return fallbackMatrix; // Cannot calculate if the current parent isn't tracking
        }

        QMatrix4x4 parentAbsoluteTransform = toQMatrix(poses[currentParentDevice].mDeviceToAbsoluteTracking);
        QMatrix4x4 relativeTransform = toQMatrix(currentRelativeMat);

        // Absolute = Parent_Absolute * Relative
        overlayAbsoluteTransform = parentAbsoluteTransform * relativeTransform;
    }
    else {
        // Unsupported transform type (e.g., Dashboard)
        return fallbackMatrix;
    }

    // Get the absolute transform of the target device we want to attach the overlay to
    vr::TrackedDevicePose_t poses[vr::k_unMaxTrackedDeviceCount];
    m_pVRSystem->GetDeviceToAbsoluteTrackingPose(trackingOrigin, 0.0f, poses, vr::k_unMaxTrackedDeviceCount);

    if (!poses[device].bPoseIsValid) {
        return fallbackMatrix; // Target device isn't tracking right now
    }

    QMatrix4x4 targetDeviceAbsoluteTransform = toQMatrix(poses[device].mDeviceToAbsoluteTracking);

    // Calculate the new relative transform
    // Target_Absolute * New_Relative = Overlay_Absolute
    // New_Relative = Inverse(Target_Absolute) * Overlay_Absolute
    bool invertible;
    QMatrix4x4 targetDeviceInverse = targetDeviceAbsoluteTransform.inverted(&invertible);

    if (!invertible) {
        return fallbackMatrix; // Prevent division by zero / math errors if tracking glitches heavily
    }

    QMatrix4x4 newRelativeTransform = targetDeviceInverse * overlayAbsoluteTransform;

    // Convert back to OpenVR format and return
    return toVrMatrix(newRelativeTransform);
}
// -- END NOTE BOUNDARY -- //

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
	return buf;
}

void SteamVRLogic::SetWidget( QWidget *pWidget) {
    if( m_pScene )
    {
        // all of the mouse handling stuff requires that the widget be at 0,0
        pWidget->move(0,0);

    	QGraphicsProxyWidget *proxy = m_pScene->addWidget( pWidget );

    	// This forces Qt to render the whole overlay as a single texture, meaning one draw call. This massively improves GPU usage.
    	// Going from 20% to 1.5% GPU usage on my system (weak laptop)
    	proxy->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    }
    m_pWidget = pWidget;

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

void SteamVRLogic::SetPanicWidget(QWidget *pWidget) {
	if( m_pPanicScene )
	{
		// all of the mouse handling stuff requires that the widget be at 0,0
		pWidget->move(0,0);

		QGraphicsProxyWidget *proxy = m_pPanicScene->addWidget( pWidget );

		// This forces Qt to render the whole overlay as a single texture, meaning one draw call. This massively improves GPU usage.
		// Going from 20% to 1.5% GPU usage on my system
		proxy->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
	}
	m_pPanicWidget = pWidget;

	m_pPanicFbo = new QOpenGLFramebufferObject(pWidget->width(), pWidget->height(), GL_TEXTURE_2D);

	if( vr::VROverlay() )
	{
		vr::HmdVector2_t vecWindowSize =
		{
			(float)pWidget->width(),
			(float)pWidget->height()
		};
		vr::VROverlay()->SetOverlayMouseScale( m_ulPanicOverlayHandle, &vecWindowSize );
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

// Given in ms, 11.(1) ms for 90 hz (yes this is confusing)
// TODO: Refactor this and all of the wrongly named variables that pull from this function
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
