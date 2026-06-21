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

SteamVRLogic::SteamVRLogic() :
m_eLastHmdError(vr::VRInitError_None),
m_eCompositorError(vr::VRInitError_None),
m_eOverlayError(vr::VRInitError_None),
m_ePanicOverlayError(vr::VRInitError_None),
m_ulOverlayHandle(vr::k_ulOverlayHandleInvalid),
m_ulPanicOverlayHandle(vr::k_ulOverlayHandleInvalid), m_ulOverlayThumbnailHandle(0),
m_pVRSystem(nullptr),
/*
m_overlayWidthInMeters(0),
m_overlayPositionMatrix({
   1.0f, 0.0f, 0.0f, 0.0f,
   0.0f, 0.866f, 0.5f, 0.1f,
   0.0f, -0.5f, 0.866f, -0.08f
	}),
	*/
m_rTrackedDevicePose{},
m_pWidget(NULL),
m_pPanicWidget(nullptr),
m_strVRDriver("No Driver"),
m_strVRDisplay("No Display"),
m_strOverlayName("Seamen Performance Overlay"),
m_pPumpEventsTimer(NULL),
m_pRenderTimer(NULL),
m_lastMouseButtons(0)
//m_settings("Seamen", "PerformanceOverlay")
{}

SteamVRLogic::~SteamVRLogic() {
}

void SteamVRLogic::DestroyInstance()
{
	if (s_pSharedSteamVRLogic)
	{
		delete s_pSharedSteamVRLogic;
		s_pSharedSteamVRLogic = nullptr; // Prevents the dangling pointer
	}
}

SteamVRLogic::initializationError SteamVRLogic::Init() {

	if (!vr::VR_IsRuntimeInstalled()) {
		std::cerr << "SteamVR is not installed." << std::endl;
		return eSteamVrNotInstalled;
	}

    bool bSuccess = true;

	m_strOverlayName = "seamen_performance_overlay";

    QStringList arguments = qApp->arguments();

    int nNameArg = arguments.indexOf( "-name" );
    if( nNameArg != -1 && nNameArg + 2 <= arguments.size() )
    {
        m_strOverlayName = arguments.at( nNameArg + 1 );
    }

	m_pOpenGLContext = std::make_unique<QOpenGLContext>();
	bSuccess = m_pOpenGLContext->create();

	if( !bSuccess ) {
		std::cout << "Failed to initialize OpenGL context." << std::endl;
		return eOpenGLFailedToInitialize;
	}

    // create an offscreen surface to attach the context and FBO to
	m_pOffscreenSurface = std::make_unique<QOffscreenSurface>();
	m_pOffscreenSurface->create();
    m_pOpenGLContext->makeCurrent( m_pOffscreenSurface.get() );

	m_pScene = std::make_unique<QGraphicsScene>();
	connect( m_pScene.get(), SIGNAL(changed(const QList<QRectF>&)), this, SLOT( OnSceneChanged(const QList<QRectF>&)) );

	m_pPanicScene = std::make_unique<QGraphicsScene>();
	connect( m_pPanicScene.get(), SIGNAL(changed(const QList<QRectF>&)), this, SLOT( OnPanicSceneChanged(const QList<QRectF>&)) );

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

			return eFailedToConnectToSteamVr;
		}
	}

	if (vr::VRInput()) {
		vr::VRInput()->GetInputSourceHandle("/user/hand/left", &m_leftHandHandle);
		vr::VRInput()->GetInputSourceHandle("/user/hand/right", &m_rightHandHandle);
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
        std::string sPanicKey = std::string( "steam.overlay.4666560" );
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
    		return eFailedToCreateOverlays;
    	}
		QString iconPath = QApplication::applicationDirPath() + "/icon.png";
    	vr::VROverlayError textureError = vr::VROverlay()->SetOverlayFromFile( m_ulOverlayThumbnailHandle, iconPath.toStdString().c_str() );
    	if (textureError != vr::VROverlayError_None) {
    		std::cerr << "Failed to load thumbnail icon from: " << iconPath.toStdString() << std::endl;
    	}
    }

	if( bSuccess )
	{
		vr::VROverlay()->SetOverlayWidthInMeters( m_ulOverlayHandle, userSettings::instance().getSize());
		vr::VROverlay()->SetOverlayWidthInMeters( m_ulPanicOverlayHandle, 2.0 );

		vr::VROverlay()->SetOverlayInputMethod( m_ulOverlayHandle, vr::VROverlayInputMethod_Mouse );
		vr::VROverlay()->SetOverlayInputMethod( m_ulPanicOverlayHandle, vr::VROverlayInputMethod_Mouse );

		m_leftController = getControllerForRole(vr::TrackedControllerRole_LeftHand);
		m_rightController = getControllerForRole(vr::TrackedControllerRole_RightHand);

		vr::VROverlay()->ShowOverlay(m_ulOverlayHandle);

		m_pPumpEventsTimer = new QTimer( this );
		m_pRenderTimer = new QTimer( this );
		m_pRenderTimer->setTimerType(Qt::CoarseTimer);
		connect(m_pPumpEventsTimer, SIGNAL( timeout() ), this, SLOT( OnTimeoutPumpEvents() ) );
		connect(m_pRenderTimer, &QTimer::timeout, this, &SteamVRLogic::RenderDirtyOverlayScenes);
		m_pPumpEventsTimer->setInterval( 40 );
		m_pPumpEventsTimer->start();

		// The quickest updating UI element is the text, which happens at 250ms intervals.
		// Rendering above that frequency would be wasted performance
		m_pRenderTimer->setInterval(250);
		m_pRenderTimer->start();
	}
	else {
		std::cerr << "Failed to initialize VR overlay." << std::endl;
		return eFailedToInitialize;
	}

	restoreSession();

	return eNone;
}

void SteamVRLogic::handlePendingTrackers() {
	std::erase_if(m_pendingTrackers, [this](auto tracker) {
		if (!m_pVRSystem->IsTrackedDeviceConnected(tracker)) {
			qDebug() << "Tracker " << tracker << " not connected. Removed from pending buffer.";
			return true;
		}

		vr::ETrackedPropertyError error = vr::TrackedProp_Success;
		bool providesBattery = m_pVRSystem->GetBoolTrackedDeviceProperty(tracker, vr::Prop_DeviceProvidesBatteryStatus_Bool, &error);
		qDebug() << "Returned error from tracker " << tracker << " in pending buffer: " << error;

		if (providesBattery && error == vr::TrackedProp_Success) {
			qDebug() << "Added tracker " << tracker << " to the UI. Removed from pending buffer and added to main vector.";
			m_trackers.push_back(tracker);
			emit addTrackerToUi(tracker);
			return true;
		}

		qDebug() << "Tracker " << tracker << " remains in pending buffer.";
		return false;
	});
}

void SteamVRLogic::addTracker(vr::TrackedDeviceIndex_t trackerToAdd) {
	// OpenVR mirrors VREvent_TrackedDeviceActivated into every overlay's queue, so this can be invoked
	// more than once for the same tracker per pump cycle. Skip if already known.
	if (std::find(m_trackers.begin(), m_trackers.end(), trackerToAdd) != m_trackers.end()) return;
	if (std::find(m_pendingTrackers.begin(), m_pendingTrackers.end(), trackerToAdd) != m_pendingTrackers.end()) return;

	vr::ETrackedPropertyError error = vr::TrackedProp_Success;

	bool providesBattery = m_pVRSystem->GetBoolTrackedDeviceProperty(trackerToAdd, vr::Prop_DeviceProvidesBatteryStatus_Bool, &error);

	qDebug() << "Returned error from tracker " << trackerToAdd << ": " << error;
	if (error == vr::TrackedProp_NotYetAvailable || (!providesBattery && error != vr::TrackedProp_Success)) {
		qDebug() << "Tracker " << trackerToAdd << " is not yet available or unknown. Added to pending buffer.";
		m_pendingTrackers.push_back(trackerToAdd);
		return;
	}

	if (!providesBattery && error == vr::TrackedProp_Success) {
		qDebug() << "Tracker " << trackerToAdd << " does not support battery data.";
		return;
	}

	qDebug() << "Added tracker " << trackerToAdd << " to the UI.";
	m_trackers.push_back(trackerToAdd);
	emit addTrackerToUi(trackerToAdd);
}

void SteamVRLogic::removeTracker(vr::TrackedDeviceIndex_t trackerToRemove) {
	if (!m_pendingTrackers.empty()) {
		if(std::find(m_pendingTrackers.begin(), m_pendingTrackers.end(), trackerToRemove) != m_pendingTrackers.end()) {
			m_pendingTrackers.erase(std::remove(m_pendingTrackers.begin(), m_pendingTrackers.end(), trackerToRemove), m_pendingTrackers.end());
		}
	}
	if (!m_trackers.empty()) {
		if(std::find(m_trackers.begin(), m_trackers.end(), trackerToRemove) != m_trackers.end()) {
			emit removeTrackerFromUi(trackerToRemove);
			m_trackers.erase(std::remove(m_trackers.begin(), m_trackers.end(), trackerToRemove), m_trackers.end());
		}
	}
}

// Blindly add all trackers to the list and then purge the ones that dont support battery percentage, are disconnected,
// or not yet available
void SteamVRLogic::searchForTrackers() {
	if (!vr::VRInput() || !m_pVRSystem) return;

	m_trackers = getDevicesForClass(vr::TrackedDeviceClass_GenericTracker);

	if (m_trackers.empty()) return;

	std::erase_if(m_trackers, [this](auto tracker) {
		if (!m_pVRSystem->IsTrackedDeviceConnected(tracker)){
			qDebug() << "Tracker " << tracker << " not connected.";
			return true;
		}

		vr::ETrackedPropertyError error = vr::TrackedProp_Success;
		bool providesBattery = m_pVRSystem->GetBoolTrackedDeviceProperty(tracker, vr::Prop_DeviceProvidesBatteryStatus_Bool, &error);

		// Add tracker to buffer vector until it's available
		qDebug() << "Returned error from tracker " << tracker << ": " << error;
		if (error == vr::TrackedProp_NotYetAvailable || (!providesBattery && error != vr::TrackedProp_Success)) {
			qDebug() << "Tracker " << tracker << " is not yet available or unkown. Added to pending buffer.";
			m_pendingTrackers.push_back(tracker);
			return true;
		}

		if (!providesBattery && error == vr::TrackedProp_Success) {
			qDebug() << "Tracker " << tracker << " does not support battery data.";
			return true;
		}

		qDebug() << "Added tracker " << tracker << " to the UI.";
		emit addTrackerToUi(tracker);
		return false;
	});
}

void SteamVRLogic::setCurrentGame() {
	// Safety check to ensure the OpenVR interface is initialized
	if (!vr::VRApplications()) {
		return;
	}

	uint32_t processId = vr::VRApplications()->GetCurrentSceneProcessId();

	// A process ID of 0 means the user is in the "Void" or loading compositor
	if (processId != 0) {
		char appKey[vr::k_unMaxApplicationKeyLength];
		vr::EVRApplicationError keyErr = vr::VRApplications()->GetApplicationKeyByProcessId(processId, appKey, sizeof(appKey));

		if (keyErr == vr::VRApplicationError_None) {

			// Filter out SteamVR Home
			if (std::strcmp(appKey, "openvr.tool.steamvr_environments") == 0) {
				return;
			}

			char appName[1024];
			vr::EVRApplicationError nameErr;

			// Retrieve the human-readable name
			vr::VRApplications()->GetApplicationPropertyString(
				appKey,
				vr::VRApplicationProperty_Name_String,
				appName,
				sizeof(appName),
				&nameErr
			);

			if (nameErr == vr::VRApplicationError_None) {
				QString qAppName = QString::fromUtf8(appName);
				QByteArray qAppKey(appKey);

				// Check if it's already tracked just in case
				if (!m_activeProcesses.contains(processId)) {
					// Cache it so your VREvent_ProcessQuit logic can cleanly shut it down later
					m_activeProcesses.insert(processId, {qAppName, qAppKey});
					emit appLaunched(qAppName);
				}
			} else {
				std::cerr << "Could not get application name for current scene key:" << appKey << std::endl;
			}
		}
	}
}
void SteamVRLogic::steamDashboardStateForUi() {
	if (!vr::VROverlay()) return;

	if (vr::VROverlay()->IsDashboardVisible()) {
		emit hideUi(false);
		m_pRenderTimer->setInterval(33);
	}
	else {
		emit hideUi(true);
		m_pRenderTimer->setInterval(250);
	}
}

void SteamVRLogic::updateOverlayWidthInMeters() {
	vr::VROverlay()->SetOverlayWidthInMeters( m_ulOverlayHandle, userSettings::instance().getSize());
}

void SteamVRLogic::resetPosition() {
	userSettings::instance().setMatrix({
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.866f, 0.5f, 0.1f,
		0.0f, -0.5f, 0.866f, -0.08f
	});
	m_matrixForRole = vr::TrackedControllerRole_LeftHand;
	if (m_leftController != vr::k_unTrackedDeviceIndexInvalid) {
		AttachToDevice(m_leftController);
		userSettings::instance().setSavedRole(vr::TrackedControllerRole_LeftHand);
	}
	else if (m_rightController != vr::k_unTrackedDeviceIndexInvalid) {
		AttachToDevice(m_rightController);
		userSettings::instance().setSavedRole(vr::TrackedControllerRole_RightHand);
		mirrorMatrix();
	}
	else {
		AttachToDevice(vr::k_unTrackedDeviceIndexInvalid);
		userSettings::instance().setSavedRole(vr::TrackedControllerRole_Invalid);
	}
}

void SteamVRLogic::Shutdown() {
	saveSession();
	DisconnectFromVRRuntime();

	if (m_pOpenGLContext) {
		m_pOpenGLContext->makeCurrent(m_pOffscreenSurface.get());
		m_pFbo.reset();
		m_pPanicFbo.reset();
		m_pOffscreenSurface.reset();
		m_pOpenGLContext.reset();
	}
}

// Important to note, the opacity is saved in dashboard.cpp as it is handled by Qt
void SteamVRLogic::saveSession() {
	if (!vr::VROverlay() || !m_pVRSystem) return;
	saveSize();
	saveController();
	savePosition();
	saveDistanceFadeStart();
}

void SteamVRLogic::savePosition() {
	// Attaching to HMD is just a fallback if no controller can be found. We don't want to save that as the last
	// known position
	if (m_deviceOverlayIsAttachedTo == vr::k_unTrackedDeviceIndex_Hmd || m_deviceOverlayIsAttachedTo == vr::k_unTrackedDeviceIndexInvalid) return;
	vr::VROverlayTransformType transformType;
	vr::VROverlayError typeError = vr::VROverlay()->GetOverlayTransformType(m_ulOverlayHandle, &transformType);
	if (typeError) return;

	userSettings::instance().setMatrix(calculateRelativeTransform(m_deviceOverlayIsAttachedTo));
}

void SteamVRLogic::saveSize() {
	float currentWidth = 0.0f;
	if (vr::VROverlay()->GetOverlayWidthInMeters(m_ulOverlayHandle, &currentWidth) == vr::VROverlayError_None) {
		userSettings::instance().setSize(currentWidth);
	}
}

void SteamVRLogic::saveController() {
	if (m_deviceOverlayIsAttachedTo == m_leftController) {
		userSettings::instance().setSavedRole(vr::TrackedControllerRole_LeftHand);
	} else if (m_deviceOverlayIsAttachedTo == m_rightController) {
		userSettings::instance().setSavedRole(vr::TrackedControllerRole_RightHand);
	}
}

void SteamVRLogic::saveDistanceFadeStart() {
	// Distance fade value is persisted directly in userSettings via its setters.
}

void SteamVRLogic::setDistanceFade(bool enabled) {
	userSettings::instance().setDistanceFadeState(enabled);
}

// IMPORTANT NOTE: Opacity is restored directly in dashboard.cpp as this code is ran before the widget exists
void SteamVRLogic::restoreSession() {
	if (!vr::VROverlay() || !m_pVRSystem) return;

	m_matrixForRole = userSettings::instance().getSavedRole();
	vr::TrackedDeviceIndex_t device = getControllerForRole(userSettings::instance().getSavedRole());
	if (device != vr::k_unTrackedDeviceIndexInvalid) {
		AttachToDevice(device);
	}

	updateOverlayWidthInMeters();
}

void SteamVRLogic::OnSceneChanged(const QList<QRectF> &region)
{
	for (const QRectF& rect : region) {
		if (!m_mainSceneDirty) {
			m_mainSceneDirtyRect = rect.toAlignedRect();
			m_mainSceneDirty = true;
		} else {
			// .united() grows the rectangle to encompass both
			m_mainSceneDirtyRect = m_mainSceneDirtyRect.united(rect.toAlignedRect());
		}
	}
}

void SteamVRLogic::OnPanicSceneChanged(const QList<QRectF>& region) {
	for (const QRectF& rect : region) {
		if (!m_panicSceneDirty) {
			m_panicSceneDirtyRect = rect.toAlignedRect();
			m_panicSceneDirty = true;
		} else {
			// .united() grows the rectangle to encompass both
			m_panicSceneDirtyRect = m_panicSceneDirtyRect.united(rect.toAlignedRect());
		}
	}
}

void SteamVRLogic::RenderDirtyOverlayScenes() {
    if (!vr::VROverlay()) return;

    bool mainVisible = m_mainSceneDirty && m_ulOverlayHandle != vr::k_ulOverlayHandleInvalid && vr::VROverlay()->IsOverlayVisible(m_ulOverlayHandle) && m_lastAlpha > 0.01f;
    bool panicVisible = m_panicSceneDirty && m_ulPanicOverlayHandle != vr::k_ulOverlayHandleInvalid && vr::VROverlay()->IsOverlayVisible(m_ulPanicOverlayHandle);

    if (!mainVisible && !panicVisible) return;

    m_pOpenGLContext->makeCurrent(m_pOffscreenSurface.get());

    // --- MAIN SCENE ---
    if (mainVisible && m_pFbo && m_pMainPaintDevice) {
        m_pFbo->bind();

        QPainter painter(m_pMainPaintDevice.get());

        // 1. Create a region from the single united bounding rect
        QRegion clipRegion(m_mainSceneDirtyRect);

        // 2. Restrict the painter to only draw inside the dirty region
        painter.setClipRegion(clipRegion);

        // 3. Clear the dirty region (acts as a targeted glClear)
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.fillRect(m_mainSceneDirtyRect, Qt::transparent);

        // 4. Render the scene (Qt will only process items inside the clip region)
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        m_pScene->render(&painter);

        painter.end();
        m_pFbo->release();

        GLuint unTexture = m_pFbo->texture();

        if (unTexture != 0) {
            vr::Texture_t texture = {(void*)(uintptr_t)unTexture, vr::TextureType_OpenGL, vr::ColorSpace_Auto};

        	QOpenGLExtraFunctions *gl = QOpenGLContext::currentContext()->extraFunctions();

        	// Insert a sync fence into the GPU command stream
        	GLsync fence = gl->glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        	gl->glFlush(); // Ensure the fence is pushed to the queue

        	// Wait for the GPU to reach the fence, yielding the CPU while waiting
        	GLenum waitReturn = GL_UNSIGNALED;
        	while (waitReturn != GL_ALREADY_SIGNALED && waitReturn != GL_CONDITION_SATISFIED) {
        		// Wait for up to 1ms
        		waitReturn = gl->glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 1000000);

        		if (waitReturn == GL_TIMEOUT_EXPIRED) {
        			// GPU is busy. Yield the CPU manually instead of spin-waiting
        			QThread::yieldCurrentThread();
        		}
        	}

        	gl->glDeleteSync(fence);

            vr::VROverlay()->SetOverlayTexture(m_ulOverlayHandle, &texture);
        }

        // 5. Reset state
        m_mainSceneDirty = false;
        m_mainSceneDirtyRect = QRect(); // Reset the bounding rect
    }

    // --- PANIC SCENE ---
    if (panicVisible && m_pPanicFbo && m_pPanicPaintDevice) {
        m_pPanicFbo->bind();

        QPainter panicPainter(m_pPanicPaintDevice.get());

        QRegion clipRegion(m_panicSceneDirtyRect);

        panicPainter.setClipRegion(clipRegion);

        panicPainter.setCompositionMode(QPainter::CompositionMode_Source);
        panicPainter.fillRect(m_panicSceneDirtyRect, Qt::transparent);

        panicPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        m_pPanicScene->render(&panicPainter);

        panicPainter.end();
        m_pPanicFbo->release();

        GLuint unPanicTexture = m_pPanicFbo->texture();
        if (unPanicTexture != 0) {
            vr::Texture_t texture = {(void*)(uintptr_t)unPanicTexture, vr::TextureType_OpenGL, vr::ColorSpace_Auto};
            vr::VROverlay()->SetOverlayTexture(m_ulPanicOverlayHandle, &texture);
        }

        m_panicSceneDirty = false;
        m_panicSceneDirtyRect = QRect();
    }
}

void SteamVRLogic::checkClosestControllerForRole() {
	if (!m_pVRSystem || !vr::VROverlay()) return;

	// Only relevant when attached to a controller, not the HMD or invalid
	if (m_deviceOverlayIsAttachedTo == vr::k_unTrackedDeviceIndexInvalid
		|| m_deviceOverlayIsAttachedTo == vr::k_unTrackedDeviceIndex_Hmd) return;

	// Don't switch controllers while the user is moving the overlay
	if (m_isMoving) return;

	//vr::ETrackedControllerRole attachedRole = m_pVRSystem->GetControllerRoleForTrackedDeviceIndex(m_deviceOverlayIsAttachedTo);
	vr::ETrackedControllerRole attachedRole = getRoleForController(m_deviceOverlayIsAttachedTo);
	if (attachedRole != vr::TrackedControllerRole_LeftHand && attachedRole != vr::TrackedControllerRole_RightHand) return;

	// Collect all devices with the same role
	std::vector<vr::TrackedDeviceIndex_t> sameRoleControllers;
	for (vr::TrackedDeviceIndex_t i = 0; i < vr::k_unMaxTrackedDeviceCount; ++i) {
		vr::ETrackedDeviceClass devClass = m_pVRSystem->GetTrackedDeviceClass(i);
		if (devClass == vr::TrackedDeviceClass_Invalid) continue;
		if (devClass == vr::TrackedDeviceClass_TrackingReference) continue;
		if (devClass == vr::TrackedDeviceClass_HMD) continue;
		if (getRoleForController(i) == attachedRole) {
			sameRoleControllers.push_back(i);
		}
	}

	// Nothing to do if there's only one (or zero) controllers with this role
	if (sameRoleControllers.size() <= 1) return;

	// Get poses for all devices we care about
	vr::TrackedDevicePose_t poses[vr::k_unMaxTrackedDeviceCount];
	m_pVRSystem->GetDeviceToAbsoluteTrackingPose(vr::TrackingUniverseStanding, 0.0f, poses, vr::k_unMaxTrackedDeviceCount);

	if (!poses[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid) return;

	float hmdX = poses[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking.m[0][3];
	float hmdY = poses[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking.m[1][3];
	float hmdZ = poses[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking.m[2][3];

	vr::TrackedDeviceIndex_t closestDevice = m_deviceOverlayIsAttachedTo;
	float closestDistSq = std::numeric_limits<float>::max();

	for (vr::TrackedDeviceIndex_t dev : sameRoleControllers) {
		if (!poses[dev].bPoseIsValid) continue;
		float dx = poses[dev].mDeviceToAbsoluteTracking.m[0][3] - hmdX;
		float dy = poses[dev].mDeviceToAbsoluteTracking.m[1][3] - hmdY;
		float dz = poses[dev].mDeviceToAbsoluteTracking.m[2][3] - hmdZ;
		float distSq = dx*dx + dy*dy + dz*dz;
		if (distSq < closestDistSq) {
			closestDistSq = distSq;
			closestDevice = dev;
		}
	}

	if (closestDevice != m_deviceOverlayIsAttachedTo) {
		// Recalculate the relative transform so the overlay stays in the same world position
		userSettings::instance().setMatrix(calculateRelativeTransform(closestDevice));
		m_matrixForRole = attachedRole;
		AttachToDevice(closestDevice);

		// Update our cached controller index for the role
		if (attachedRole == vr::TrackedControllerRole_LeftHand) m_leftController = closestDevice;
		else m_rightController = closestDevice;
	}
}

// Unused but could be useful in the future
float SteamVRLogic::calculateOverlayDistance() {
	if (!m_pVRSystem || m_deviceOverlayIsAttachedTo == vr::k_unTrackedDeviceIndexInvalid)
		return -1.0f;

	vr::TrackedDevicePose_t poses[vr::k_unMaxTrackedDeviceCount];
	uint32_t poseCount = std::max<uint32_t>(m_deviceOverlayIsAttachedTo + 1, 1);
	m_pVRSystem->GetDeviceToAbsoluteTrackingPose(vr::TrackingUniverseStanding, 0.0f, poses, poseCount);

	const vr::TrackedDevicePose_t& hmdPose = poses[vr::k_unTrackedDeviceIndex_Hmd];
	const vr::TrackedDevicePose_t& devicePose = poses[m_deviceOverlayIsAttachedTo];

	if (!hmdPose.bPoseIsValid || !devicePose.bPoseIsValid)
		return -1.0f;

	// Compute overlay world position: deviceWorld * overlayRelative
	const vr::HmdMatrix34_t& dw = devicePose.mDeviceToAbsoluteTracking;
	const vr::HmdMatrix34_t rel = userSettings::instance().getMatrix();

	float ox = dw.m[0][0]*rel.m[0][3] + dw.m[0][1]*rel.m[1][3] + dw.m[0][2]*rel.m[2][3] + dw.m[0][3];
	float oy = dw.m[1][0]*rel.m[0][3] + dw.m[1][1]*rel.m[1][3] + dw.m[1][2]*rel.m[2][3] + dw.m[1][3];
	float oz = dw.m[2][0]*rel.m[0][3] + dw.m[2][1]*rel.m[1][3] + dw.m[2][2]*rel.m[2][3] + dw.m[2][3];

	float dx = hmdPose.mDeviceToAbsoluteTracking.m[0][3] - ox;
	float dy = hmdPose.mDeviceToAbsoluteTracking.m[1][3] - oy;
	float dz = hmdPose.mDeviceToAbsoluteTracking.m[2][3] - oz;

	return std::sqrt(dx*dx + dy*dy + dz*dz);
}

void SteamVRLogic::attemptControllerBind() {
	if (m_leftController == vr::k_unTrackedDeviceIndexInvalid)
		//m_leftController = m_pVRSystem->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_LeftHand);
		m_leftController = getControllerForRole(vr::TrackedControllerRole_LeftHand);
	if (m_rightController == vr::k_unTrackedDeviceIndexInvalid)
		//m_rightController = m_pVRSystem->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_RightHand);
		m_rightController = getControllerForRole(vr::TrackedControllerRole_RightHand);


	// Try to bind to left controller if that is the saved role
	if (m_leftController != vr::k_unTrackedDeviceIndexInvalid) {
		if (userSettings::instance().getSavedRole() != vr::TrackedControllerRole_Invalid && userSettings::instance().getSavedRole() == vr::TrackedControllerRole_LeftHand) AttachToDevice(m_leftController);
	}

	// If the overlay is still not attached try to bind to right controller if that is the saved role
	if (m_rightController != vr::k_unTrackedDeviceIndexInvalid && m_deviceOverlayIsAttachedTo == vr::k_unTrackedDeviceIndexInvalid) {
		if (userSettings::instance().getSavedRole() != vr::TrackedControllerRole_Invalid && userSettings::instance().getSavedRole() == vr::TrackedControllerRole_RightHand) AttachToDevice(m_rightController);
	}

	// So far we failed to bind to the saved controller. As a last ditch effort attempt to bind to other controller
	// if we attempted 9 times already
	if (m_deviceOverlayIsAttachedTo == vr::k_unTrackedDeviceIndexInvalid && m_bindToControllerAttempts <= 9) {
		if (m_leftController != vr::k_unTrackedDeviceIndexInvalid) {
			mirrorMatrix();
			AttachToDevice(m_leftController);
		}
		else if (m_rightController != vr::k_unTrackedDeviceIndexInvalid) {
			mirrorMatrix();
			AttachToDevice(m_rightController);
		}
	}
	// Wait 250ms and try to find a controller again
	std::this_thread::sleep_for(std::chrono::milliseconds(250));
	// If no controller has been found after 10 tries, just attach to the HMD
	if (m_bindToControllerAttempts >= 10) {
		AttachToDevice(vr::k_unTrackedDeviceIndex_Hmd);
	}
}

void SteamVRLogic::setControllersBatteryLevel() {
	// Attempt to retrieve a controller if the variable is invalid. No need to check whether it actually got one
	// as the getControllerForRole will just return -1.0 if it fails
	if (m_leftController == vr::k_unTrackedDeviceIndexInvalid) m_leftController = getControllerForRole(vr::TrackedControllerRole_LeftHand);

	emit leftControllerBattery(getDeviceBatteryLevel(m_leftController),
		m_pVRSystem->GetBoolTrackedDeviceProperty(m_leftController, vr::Prop_DeviceIsCharging_Bool, nullptr));

	if (m_rightController == vr::k_unTrackedDeviceIndexInvalid) m_rightController= getControllerForRole(vr::TrackedControllerRole_LeftHand);

	emit rightControllerBattery(getDeviceBatteryLevel(m_rightController),
		m_pVRSystem->GetBoolTrackedDeviceProperty(m_rightController, vr::Prop_DeviceIsCharging_Bool, nullptr));
}

void SteamVRLogic::setHeadsetBatteryLevel() {
	emit headsetBattery(getDeviceBatteryLevel(vr::k_unTrackedDeviceIndex_Hmd),
		m_pVRSystem->GetBoolTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_DeviceIsCharging_Bool, nullptr));
}

void SteamVRLogic::setTrackersBattery() {
	if (m_trackers.empty()) return;
	for (const auto &tracker : m_trackers) {
		emit trackersBattery(getDeviceBatteryLevel(tracker), tracker,
			m_pVRSystem->GetBoolTrackedDeviceProperty(tracker, vr::Prop_DeviceIsCharging_Bool, nullptr));
	}
}

// Returns -1 if the device does not support battery level
// This is a more comprehensive check the querying Prop_DeviceProvidesBatteryStatus_Bool
float SteamVRLogic::getDeviceBatteryLevel(vr::TrackedDeviceIndex_t device) {
	vr::ETrackedPropertyError error = vr::TrackedProp_ValueNotProvidedByDevice;
	float level = m_pVRSystem->GetFloatTrackedDeviceProperty(device, vr::Prop_DeviceBatteryPercentage_Float, &error);
	if (error != vr::TrackedProp_Success || level == 0) {
		return -1.0f;
	}
	return level;
}

void SteamVRLogic::checkForCloserController() {
	if (m_pVRSystem && vr::VROverlay() && !m_isMoving) {
		vr::TrackedDevicePose_t poses[vr::k_unMaxTrackedDeviceCount];
		m_pVRSystem->GetDeviceToAbsoluteTrackingPose(
			vr::TrackingUniverseStanding, 0.0f, poses, vr::k_unMaxTrackedDeviceCount);

		bool onDevice = m_deviceOverlayIsAttachedTo != vr::k_unTrackedDeviceIndexInvalid
					 && m_deviceOverlayIsAttachedTo != vr::k_unTrackedDeviceIndex_Hmd;
		bool attachedPoseValid = onDevice && poses[m_deviceOverlayIsAttachedTo].bPoseIsValid;

		vr::ETrackedControllerRole preferredRole = (userSettings::instance().getSavedRole() != vr::TrackedControllerRole_Invalid)
			? userSettings::instance().getSavedRole() : vr::TrackedControllerRole_LeftHand;
		vr::ETrackedControllerRole otherRole = (preferredRole == vr::TrackedControllerRole_LeftHand)
			? vr::TrackedControllerRole_RightHand : vr::TrackedControllerRole_LeftHand;

		// Find a device for a given role that currently has a valid (tracked) pose
		auto findTrackedDevice = [&](vr::ETrackedControllerRole role) -> vr::TrackedDeviceIndex_t {
			for (vr::TrackedDeviceIndex_t i = 0; i < vr::k_unMaxTrackedDeviceCount; ++i) {
				if (!poses[i].bPoseIsValid) continue;
				vr::ETrackedDeviceClass devClass = m_pVRSystem->GetTrackedDeviceClass(i);
				if (devClass == vr::TrackedDeviceClass_Invalid
					|| devClass == vr::TrackedDeviceClass_TrackingReference
					|| devClass == vr::TrackedDeviceClass_HMD) continue;
				if (getRoleForController(i) == role) return i;
			}
			return vr::k_unTrackedDeviceIndexInvalid;
		};

		vr::TrackedDeviceIndex_t preferredDev = findTrackedDevice(preferredRole);

		// Priority 1: Switch to the preferred-role device if it's tracked and we're not on it
		if (preferredDev != vr::k_unTrackedDeviceIndexInvalid
		 && preferredDev != m_deviceOverlayIsAttachedTo) {

		 vr::ETrackedControllerRole attachedRole = getRoleForController(m_deviceOverlayIsAttachedTo);

		 if (onDevice && attachedPoseValid && attachedRole == preferredRole) {
		    // Both devices tracked AND they share the same role (e.g., controller to hand tracking)
		    // Safely preserve world position
		    userSettings::instance().setMatrix(calculateRelativeTransform(preferredDev));
		    m_matrixForRole = preferredRole;
		 } else if (m_matrixForRole != preferredRole
		          && m_matrixForRole != vr::TrackedControllerRole_Invalid) {
		    // Coming from HMD, untracked device, or the OTHER hand — mirror to snap to saved pos
		    mirrorMatrix();
		 }

		 AttachToDevice(preferredDev);
		 if (preferredRole == vr::TrackedControllerRole_LeftHand) m_leftController = preferredDev;
		 else m_rightController = preferredDev;
		}
		// Priority 2: Current device lost tracking, preferred not available — try other hand
		else if ((!onDevice || !attachedPoseValid)
		     && preferredDev == vr::k_unTrackedDeviceIndexInvalid) {

			vr::TrackedDeviceIndex_t otherDev = findTrackedDevice(otherRole);

			if (otherDev != vr::k_unTrackedDeviceIndexInvalid
				&& otherDev != m_deviceOverlayIsAttachedTo) {

				vr::ETrackedControllerRole attachedRole = getRoleForController(m_deviceOverlayIsAttachedTo);

				if (onDevice && attachedPoseValid && attachedRole == otherRole) {
					userSettings::instance().setMatrix(calculateRelativeTransform(otherDev));
					m_matrixForRole = otherRole;
				} else if (m_matrixForRole != otherRole
					&& m_matrixForRole != vr::TrackedControllerRole_Invalid) {
					mirrorMatrix();
				}

				AttachToDevice(otherDev);
				if (otherRole == vr::TrackedControllerRole_LeftHand) m_leftController = otherDev;
				else m_rightController = otherDev;
			}
			// Priority 3: No tracked hand at all — fall back to HMD
			else if (onDevice && !attachedPoseValid) {
				AttachToDevice(vr::k_unTrackedDeviceIndex_Hmd);
			}
		}
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
		attemptControllerBind();
		++m_bindToControllerAttempts;
	}

	vr::VREvent_t vrEvent;

	// Every ~5 seconds, check if a closer controller with the same role exists
	if (++m_proximityCheckCounter >= 125) {
		m_proximityCheckCounter = 0;
		checkClosestControllerForRole();
	}

	// Every ~30 seconds set battery life of devices
	if (++m_batteryCheckCounter >= 750) {
		m_batteryCheckCounter = 0;
		setControllersBatteryLevel();
		setHeadsetBatteryLevel();
		if (!m_pendingTrackers.empty()) handlePendingTrackers();
		if (userSettings::instance().getShowTrackers()) setTrackersBattery(); // Skip querying tracker batteries if the UI element isnt shown
	}

	// Every ~2 seconds, check if the attached device has a valid pose and if the preferred
	// device is available. Hand tracking devices stay "active" but lose their pose when
	// the hand leaves the camera's view, so VREvent_TrackedDeviceDeactivated never fires.
	if (++m_poseCheckCounter >= 50) {
		m_poseCheckCounter = 0;
		checkForCloserController();
	}

	// Process events for one overlay, dispatching mouse events to the given scene and widget.
	// `isPrimaryOverlay` gates system-wide events so they aren't handled once per overlay.
	auto processOverlayEvents = [&](vr::VROverlayHandle_t handle, QGraphicsScene* scene, QWidget* widget, bool isPrimaryOverlay) {

		float scaleBaseWidth = 0.0f;
		if (m_isScaling && vr::VROverlay()) {
			vr::VROverlay()->GetOverlayWidthInMeters(handle, &scaleBaseWidth);
		}

		while( vr::VROverlay()->PollNextOverlayEvent( handle, &vrEvent, sizeof( vrEvent ) ) )
		{
			if (vrEvent.trackedDeviceIndex != vr::k_unTrackedDeviceIndexInvalid) {
				m_unLastInteractingDevice = vrEvent.trackedDeviceIndex;
			}

			// OpenVR copies global events into every overlay's queue; handle them only once.
			if (!isPrimaryOverlay) {
				switch (vrEvent.eventType) {
					case vr::VREvent_TrackedDeviceActivated:
					case vr::VREvent_TrackedDeviceDeactivated:
					case vr::VREvent_DashboardActivated:
					case vr::VREvent_DashboardDeactivated:
					case vr::VREvent_SceneApplicationChanged:
					case vr::VREvent_ProcessQuit:
					case vr::VREvent_Quit:
						continue;
					default:
						break;
				}
			}

			switch( vrEvent.eventType )
			{
			case vr::VREvent_MouseMove:
				{
					QPointF ptNewMouse( vrEvent.data.mouse.x, vrEvent.data.mouse.y );

					if (m_isScaling && vr::VROverlay()) {
						if (m_scaleButtonPressX < 0.0f) {
							m_scaleButtonPressX = ptNewMouse.x();
							if (scaleBaseWidth <= 0.0f) {
								vr::VROverlay()->GetOverlayWidthInMeters(handle, &scaleBaseWidth);
							}
						} else {
							float centerPx = widget->width() * 0.5f;
							float denominator = m_scaleButtonPressX - centerPx;
							if (std::abs(denominator) > 0.5f) {
								float newWidth = scaleBaseWidth * (ptNewMouse.x() - centerPx) / denominator;
								newWidth = std::max(0.01f, std::min(newWidth, 10.0f));
								userSettings::instance().setSize(newWidth);
							}
						}
					}

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

					if (m_isScaling) {
						stopScale();
					}
				}
				break;

			case vr::VREvent_DashboardActivated:
				{
					emit hideUi(false);
					// Render the UI at a high refresh rate for smooth UI interaction
					m_pRenderTimer->setInterval(33);
				}
				break;

			case vr::VREvent_DashboardDeactivated:
				{
					emit hideUi(true);
					// Stop rendering the UI at a high refresh rate to save GPU performance
					m_pRenderTimer->setInterval(250);
				}
				break;

			case vr::VREvent_FocusLeave:
				{
					// When the laser pointer leaves the overlay, send a mouse move
					// to an off-screen position so Qt generates proper leave/hover-out
					// events for any widget that was under the cursor. Without this,
					// a fast-moving laser can exit the overlay without a final MouseMove
					// inside its bounds, leaving buttons stuck in hover state.
					QPointF ptOffScreen( -1.0f, -1.0f );
					QPoint ptGlobal = ptOffScreen.toPoint();
					QGraphicsSceneMouseEvent mouseEvent( QEvent::GraphicsSceneMouseMove );
					mouseEvent.setWidget( NULL );
					mouseEvent.setPos( ptOffScreen );
					mouseEvent.setScenePos( ptGlobal );
					mouseEvent.setScreenPos( ptGlobal );
					mouseEvent.setLastPos( m_tLastMouse );
					mouseEvent.setLastScenePos( widget->mapToGlobal( m_tLastMouse.toPoint() ) );
					mouseEvent.setLastScreenPos( widget->mapToGlobal( m_tLastMouse.toPoint() ) );
					mouseEvent.setButtons( m_lastMouseButtons );
					mouseEvent.setButton( Qt::NoButton );
					mouseEvent.setModifiers( (Qt::KeyboardModifiers)0 );
					mouseEvent.setAccepted( false );

					m_tLastMouse = ptOffScreen;
					QApplication::sendEvent( scene, &mouseEvent );
				}
				break;

			case vr::VREvent_OverlayShown:
				widget->repaint();
				break;

			case vr::VREvent_SceneApplicationChanged:
				{
				   uint32_t processId = vrEvent.data.process.pid;

				   if (processId != 0 && vr::VRApplications()) {
				      char appKey[vr::k_unMaxApplicationKeyLength];
				      vr::EVRApplicationError keyErr = vr::VRApplications()->GetApplicationKeyByProcessId(processId, appKey, sizeof(appKey));

				      if (keyErr == vr::VRApplicationError_None) {
				         char appName[1024];
				         vr::EVRApplicationError nameErr;

				         vr::VRApplications()->GetApplicationPropertyString(
				            appKey,
				            vr::VRApplicationProperty_Name_String,
				            appName,
				            sizeof(appName),
				            &nameErr
				         );

				         if (nameErr == vr::VRApplicationError_None) {
				            QString qAppName = QString::fromUtf8(appName);
				            QByteArray qAppKey(appKey);

				            // Prevent false-positive re-launches
				            bool isNewLaunch = true;
				            for (auto it = m_activeProcesses.constBegin(); it != m_activeProcesses.constEnd(); ++it) {
				                if (it.value().appKey == qAppKey) {
				                    isNewLaunch = false;
				                    break;
				                }
				            }

				            // Cache the PID mapping to both Name and Key
				            m_activeProcesses.insert(processId, {qAppName, qAppKey});

				            // Only emit if its a genuinely new launch
				            if (isNewLaunch) {
				                emit appLaunched(qAppName);
				            }
				         } else {
				            std::cerr << "Could not get application name for key:" << appKey << std::endl;
				         }
				      }
				   }
				}
				break;

			case vr::VREvent_ProcessQuit:
				{
				   uint32_t processId = vrEvent.data.process.pid;

				   if (m_activeProcesses.contains(processId)) {
				      // Retrieve and remove the quitting PID from the cache
				      AppCacheData data = m_activeProcesses.take(processId);

				      // Ask OpenVR if the main application is actually dead
				      uint32_t mainProcessId = vr::VRApplications()->GetApplicationProcessId(data.appKey.constData());

				      // If mainProcessId is 0, the application is completely shut down
				      if (mainProcessId == 0) {
				          emit appQuit(data.appName);
				      } else {
				          // The main app is still running (this was just a child process or compositor shift).
				          // Ensure the actual main process is tracked in our cache so we don't lose it.
				          if (!m_activeProcesses.contains(mainProcessId)) {
				              m_activeProcesses.insert(mainProcessId, data);
				          }
				      }
				   }
				}
				break;

			case vr::VREvent_KeyboardOpened_Global:
				{
					if (vr::VROverlay())
					{
						vr::VROverlay()->HideOverlay(handle);
					}
				}
				break;

			case vr::VREvent_KeyboardClosed_Global:
				{
					if (vr::VROverlay())
					{
						vr::VROverlay()->ShowOverlay(handle);
					}
				}
				break;

			case vr::VREvent_TrackedDeviceActivated:
				{
					vr::TrackedDeviceIndex_t newDeviceIndex = vrEvent.trackedDeviceIndex;
					vr::ETrackedControllerRole role = getRoleForController(newDeviceIndex);
					vr::ETrackedDeviceClass deviceClass = m_pVRSystem->GetTrackedDeviceClass(newDeviceIndex);
					vr::ETrackedControllerRole currentAttachedRole = (m_deviceOverlayIsAttachedTo != vr::k_unTrackedDeviceIndexInvalid
					   && m_deviceOverlayIsAttachedTo != vr::k_unTrackedDeviceIndex_Hmd)
					   ? getRoleForController(m_deviceOverlayIsAttachedTo) : vr::TrackedControllerRole_Invalid;

					if (deviceClass == vr::TrackedDeviceClass_GenericTracker) {
						addTracker(newDeviceIndex);
					}

					else if (role == vr::TrackedControllerRole_LeftHand) {
					   m_leftController = newDeviceIndex;
					   if (m_deviceOverlayIsAttachedTo == vr::k_unTrackedDeviceIndex_Hmd || m_deviceOverlayIsAttachedTo == vr::k_unTrackedDeviceIndexInvalid) {
					      // Overlay is on HMD/unattached — move to this hand
					      if (vr::TrackedControllerRole_LeftHand != m_matrixForRole) mirrorMatrix();
					      AttachToDevice(m_leftController);
					   }
					   else if (currentAttachedRole == vr::TrackedControllerRole_LeftHand) {
					      // Already on a left-hand device, user probably changed device for same role
					      userSettings::instance().setMatrix(calculateRelativeTransform(m_leftController));
					      m_matrixForRole = vr::TrackedControllerRole_LeftHand;
					      AttachToDevice(m_leftController);
					   }
					   else if (userSettings::instance().getSavedRole() != currentAttachedRole) {
					      // On wrong hand — switch to saved hand and ensure matrix matches this hand
					      if (vr::TrackedControllerRole_LeftHand != m_matrixForRole) mirrorMatrix();
					      AttachToDevice(m_leftController);
					   }
					}

					else if (role == vr::TrackedControllerRole_RightHand) {
					   m_rightController = newDeviceIndex;
					   if (m_deviceOverlayIsAttachedTo == vr::k_unTrackedDeviceIndex_Hmd || m_deviceOverlayIsAttachedTo == vr::k_unTrackedDeviceIndexInvalid) {
					      // Overlay is on HMD/unattached — move to this hand
					      if (vr::TrackedControllerRole_RightHand != m_matrixForRole) mirrorMatrix();
					      AttachToDevice(m_rightController);
					   }
					   else if (currentAttachedRole == vr::TrackedControllerRole_RightHand) {
					      // Already on a right-hand device
					      userSettings::instance().setMatrix(calculateRelativeTransform(m_rightController));
					      m_matrixForRole = vr::TrackedControllerRole_RightHand;
					      AttachToDevice(m_rightController);
					   }
					   else if (userSettings::instance().getSavedRole() != currentAttachedRole) {
					      // On wrong hand — switch to saved hand and ensure matrix matches this hand
					      if (vr::TrackedControllerRole_RightHand != m_matrixForRole) mirrorMatrix();
					      AttachToDevice(m_rightController);
					   }
					}
				}
				break;

			case vr::VREvent_TrackedDeviceDeactivated:
				{
					vr::TrackedDeviceIndex_t deactivatedDevice = vrEvent.trackedDeviceIndex;
					vr::ETrackedDeviceClass deviceClass = m_pVRSystem->GetTrackedDeviceClass(deactivatedDevice);

					if (deviceClass == vr::TrackedDeviceClass_GenericTracker) {
						removeTracker(deactivatedDevice);
						break;
					}

					// Determine the role before clearing the cached index
					vr::ETrackedControllerRole deactivatedRole = vr::TrackedControllerRole_Invalid;
					if (m_leftController == deactivatedDevice) {
						deactivatedRole = vr::TrackedControllerRole_LeftHand;
						m_leftController = vr::k_unTrackedDeviceIndexInvalid;
					}
					if (m_rightController == deactivatedDevice) {
						deactivatedRole = vr::TrackedControllerRole_RightHand;
						m_rightController = vr::k_unTrackedDeviceIndexInvalid;
					}

					// If the overlay was attached to this device, find a replacement
					if (m_deviceOverlayIsAttachedTo == deactivatedDevice) {
						vr::TrackedDeviceIndex_t replacement = vr::k_unTrackedDeviceIndexInvalid;

						// Try same role first
						if (deactivatedRole != vr::TrackedControllerRole_Invalid) {
							replacement = getControllerForRole(deactivatedRole);
							if (replacement != vr::k_unTrackedDeviceIndexInvalid) {
								userSettings::instance().setMatrix(calculateRelativeTransform(replacement));
								m_matrixForRole = deactivatedRole;
								AttachToDevice(replacement);
								if (deactivatedRole == vr::TrackedControllerRole_LeftHand) m_leftController = replacement;
								else m_rightController = replacement;
							}
						}

						// Try the other hand
						if (replacement == vr::k_unTrackedDeviceIndexInvalid) {
							vr::ETrackedControllerRole otherRole = (deactivatedRole == vr::TrackedControllerRole_LeftHand)
								? vr::TrackedControllerRole_RightHand : vr::TrackedControllerRole_LeftHand;
							if (deactivatedRole != vr::TrackedControllerRole_Invalid) {
								replacement = getControllerForRole(otherRole);
								if (replacement != vr::k_unTrackedDeviceIndexInvalid) {
									userSettings::instance().setMatrix(calculateRelativeTransform(replacement));
									m_matrixForRole = otherRole;
									AttachToDevice(replacement);
									if (otherRole == vr::TrackedControllerRole_LeftHand) m_leftController = replacement;
									else m_rightController = replacement;
								}
							}
						}

						// No replacement found, fall back to HMD
						if (replacement == vr::k_unTrackedDeviceIndexInvalid) {
							AttachToDevice(vr::k_unTrackedDeviceIndex_Hmd);
						}
					}
				}
				break;

			case vr::VREvent_Quit:
				QApplication::exit();
				break;
			}
		}
	};

	processOverlayEvents( m_ulOverlayHandle, m_pScene.get(), m_pWidget, /*isPrimaryOverlay=*/true );
	processOverlayEvents( m_ulPanicOverlayHandle, m_pPanicScene.get(), m_pPanicWidget, /*isPrimaryOverlay=*/false );

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
			// When attached to the HMD the overlay is always facing the user. Angle-based
			// fading does not apply. devicePose == hmdPose (both index 0), so the
			// overlay-to-HMD vector is near zero and would yield an alpha of 0.
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
				const vr::HmdMatrix34_t rel = userSettings::instance().getMatrix();

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

				// Distance-based fade: full opacity up to the use defined setting, fades to 0% over the next 10cm
				float distanceFactor = 1.0f;
				if (userSettings::instance().getDistanceFadeState()) {
					if (dLen > userSettings::instance().getDistanceFadeValue()) {
						constexpr float distFadeRange = 0.10f;
						distanceFactor = 1.0f - (dLen - userSettings::instance().getDistanceFadeValue()) / distFadeRange;
						distanceFactor = std::max(0.0f, std::min(1.0f, distanceFactor));
					}
				}

				// Only call the API if the alpha actually changed, to avoid redundant
				// OpenVR calls every 20ms when the viewing angle is stable.
				float newAlpha = m_baseAlpha * angleFactor * distanceFactor;

				// Clamp to avoid overlay being in a barely visible state
				if (newAlpha < 0.005f) newAlpha = 0.0f;

				// When the opacity is 0% we have to disable cursor input its hitbox is still active.
				// Update only if the alpha changed significantly OR if we just hit absolute zero
				if (std::abs(newAlpha - m_lastAlpha) >= 0.005f || (newAlpha == 0.0f && m_lastAlpha != 0.0f)) {
					if (newAlpha == 0.0f) vr::VROverlay()->SetOverlayInputMethod(m_ulOverlayHandle, vr::VROverlayInputMethod_None);
					else if (m_lastAlpha == 0) vr::VROverlay()->SetOverlayInputMethod(m_ulOverlayHandle, vr::VROverlayInputMethod_Mouse);
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

void SteamVRLogic::switchToSpecificController(vr::TrackedDeviceIndex_t targetDevice) {
	if (targetDevice == vr::k_unTrackedDeviceIndexInvalid) return;

	mirrorMatrix();
	AttachToDevice(targetDevice);

	vr::ETrackedControllerRole newRole = getRoleForController(targetDevice);
	if (newRole == vr::TrackedControllerRole_LeftHand || newRole == vr::TrackedControllerRole_RightHand) {
		userSettings::instance().setSavedRole(newRole);
	}

	m_isMoving = false;
}

void SteamVRLogic::attachToLeftController() {
	if (getRoleForController(m_deviceOverlayIsAttachedTo) == vr::TrackedControllerRole_LeftHand) return;

	vr::TrackedDeviceIndex_t leftIndex = getControllerForRole(vr::TrackedControllerRole_LeftHand);
	switchToSpecificController(leftIndex);
}

void SteamVRLogic::attachToRightController() {
	if (getRoleForController(m_deviceOverlayIsAttachedTo) == vr::TrackedControllerRole_RightHand) return;

	vr::TrackedDeviceIndex_t rightIndex = getControllerForRole(vr::TrackedControllerRole_RightHand);
	switchToSpecificController(rightIndex);
}

// Abuse the overlay's logic to default to HMD when no controller can be found
void SteamVRLogic::attachToHmd() {
	m_leftController = vr::k_unTrackedDeviceIndexInvalid;
	m_rightController = vr::k_unTrackedDeviceIndexInvalid;
	AttachToDevice(vr::k_unTrackedDeviceIndex_Hmd);
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
	else {
		vr::HmdMatrix34_t matrix = userSettings::instance().getMatrix();
		vr::VROverlay()->SetOverlayTransformTrackedDeviceRelative(m_ulOverlayHandle, device, &matrix);
	}
	m_deviceOverlayIsAttachedTo = device;
}

void SteamVRLogic::switchController() {
	// Ensure we actually have a valid device that clicked the button if by some miracle the user happens to somehow
	// click the button with an invalid device
	if (m_unLastInteractingDevice != vr::k_unTrackedDeviceIndexInvalid) {
		// Attach the overlay to the controller that triggered the click
		mirrorMatrix();
		AttachToDevice(m_unLastInteractingDevice);

		// Update saved role so device-activation logic respects the user's explicit choice
		vr::ETrackedControllerRole newRole = getRoleForController(m_unLastInteractingDevice);
		if (newRole == vr::TrackedControllerRole_LeftHand || newRole == vr::TrackedControllerRole_RightHand) {
			userSettings::instance().setSavedRole(newRole);
		}

		m_isMoving = false;
	} else {
		std::cerr << "Warning: You have somehow done something that should be impossible. You clicked the switch controller"
		" button with an invalid device." << std::endl;
	}
}

void SteamVRLogic::startScale() {
	if (m_isScaling) return;
	m_isScaling = true;
	m_scaleButtonPressX = -1.0f;  // anchor will be captured on first MouseMove
}

void SteamVRLogic::stopScale() {
	m_isScaling = false;
	m_scaleButtonPressX = -1.0f;
}

void SteamVRLogic::startMove() {
	// If widget is already moving, cannot start moving it again
	if (m_isMoving) return;

	// Re-query controller indices — they may have been invalid at init time during SteamVR autostart
	// Re-query controller/hand indices — they may have been invalid at init time during SteamVR autostart
	if (m_leftController == vr::k_unTrackedDeviceIndexInvalid) {
		//m_leftController = m_pVRSystem->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_LeftHand);
		m_leftController = getControllerForRole(vr::TrackedControllerRole_LeftHand);
	}
	if (m_rightController == vr::k_unTrackedDeviceIndexInvalid) {
		//m_rightController = m_pVRSystem->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_RightHand);
		m_rightController = getControllerForRole(vr::TrackedControllerRole_RightHand);
	}

	if (m_deviceOverlayIsAttachedTo == m_leftController) {
		if (m_rightController == vr::k_unTrackedDeviceIndexInvalid) {
			std::cerr << "startMove: right controller not yet tracked, cannot start move." << std::endl;
			return;
		}
		userSettings::instance().setMatrix(calculateRelativeTransform(m_rightController));
		m_matrixForRole = vr::TrackedControllerRole_RightHand;
		AttachToDevice(m_rightController);
		m_isMoving = true;
	}
	else {
		if (m_leftController == vr::k_unTrackedDeviceIndexInvalid) {
			std::cerr << "startMove: left controller not yet tracked, cannot start move." << std::endl;
			return;
		}
		userSettings::instance().setMatrix(calculateRelativeTransform(m_leftController));
		m_matrixForRole = vr::TrackedControllerRole_LeftHand;
		AttachToDevice(m_leftController);
		m_isMoving = true;
	}
}

void SteamVRLogic::stopMove() {
	if (m_deviceOverlayIsAttachedTo == m_rightController) {
		userSettings::instance().setMatrix(calculateRelativeTransform(m_leftController));
		m_matrixForRole = vr::TrackedControllerRole_LeftHand;
		AttachToDevice(m_leftController);
	}
	else {
		userSettings::instance().setMatrix(calculateRelativeTransform(m_rightController));
		m_matrixForRole = vr::TrackedControllerRole_RightHand;
		AttachToDevice(m_rightController);
	}
	//savePosition();
	//saveController();
}

void SteamVRLogic::mirrorMatrix() {
	vr::HmdMatrix34_t matrix = userSettings::instance().getMatrix();
	matrix.m[0][3] = -matrix.m[0][3];
	matrix.m[0][1] = -matrix.m[0][1];
	matrix.m[0][2] = -matrix.m[0][2];
	matrix.m[1][0] = -matrix.m[1][0];
	matrix.m[2][0] = -matrix.m[2][0];
	userSettings::instance().setMatrix(matrix);

	if (m_matrixForRole == vr::TrackedControllerRole_LeftHand)
		m_matrixForRole = vr::TrackedControllerRole_RightHand;
	else if (m_matrixForRole == vr::TrackedControllerRole_RightHand)
		m_matrixForRole = vr::TrackedControllerRole_LeftHand;
}


// -- NOTE: The below function is made almost entirely made by Claude. I was too lazy to refresh my math on matrices -- //

// Calculates the relative transform between the overlay and the passed device
vr::HmdMatrix34_t SteamVRLogic::calculateRelativeTransform(vr::TrackedDeviceIndex_t device) {
    // Fallback to the current matrix if anything goes wrong
	vr::HmdMatrix34_t fallbackMatrix = userSettings::instance().getMatrix();

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

// Finds a tracked device for the given hand role. First tries the standard controller
// role API, then falls back to scanning all tracked devices by their role hint property.
// This ensures hand tracking devices are found even when they are not classified as controllers.
vr::TrackedDeviceIndex_t SteamVRLogic::getControllerForRole(vr::ETrackedControllerRole role) {
	if (!vr::VRInput()) return vr::k_unTrackedDeviceIndexInvalid;

	// Pick the handle based on the requested role
	vr::VRInputValueHandle_t handle = (role == vr::TrackedControllerRole_LeftHand) ? m_leftHandHandle : m_rightHandHandle;

	if (handle == vr::k_ulInvalidInputValueHandle) return vr::k_unTrackedDeviceIndexInvalid;

	// Query IVRInput for the physical device index backing this handle
	vr::InputOriginInfo_t originInfo;
	vr::EVRInputError err = vr::VRInput()->GetOriginTrackedDeviceInfo(handle, &originInfo, sizeof(originInfo));

	if (err == vr::VRInputError_None && originInfo.trackedDeviceIndex != vr::k_unTrackedDeviceIndexInvalid) {
		return originInfo.trackedDeviceIndex;
	}

	return vr::k_unTrackedDeviceIndexInvalid;
}

std::vector<vr::TrackedDeviceIndex_t>  SteamVRLogic::getDevicesForClass(vr::ETrackedDeviceClass classToLookFor) {
	if (!vr::VRInput() || !m_pVRSystem) return std::vector<vr::TrackedDeviceIndex_t>(); // return empty
	// Passing nullptr and 0 will return the required array size.
	uint32_t deviceCount = m_pVRSystem->GetSortedTrackedDeviceIndicesOfClass(
		classToLookFor,
		nullptr,
		0,
		vr::k_unTrackedDeviceIndex_Hmd // Sort relative to the headset
	);

	if (deviceCount > 0) {
		std::vector<vr::TrackedDeviceIndex_t> foundDevices(deviceCount);
		// Call the function again to actually populate your array
		m_pVRSystem->GetSortedTrackedDeviceIndicesOfClass(
			classToLookFor,
			foundDevices.data(),
			deviceCount,
			vr::k_unTrackedDeviceIndex_Hmd
		);
		return foundDevices;
	}

	return std::vector<vr::TrackedDeviceIndex_t>(); // return empty if nothing found
}

// Returns the role for a device. First tries the standard controller role API,
// then falls back to the role hint property to support hand tracking devices.
vr::ETrackedControllerRole SteamVRLogic::getRoleForController(vr::TrackedDeviceIndex_t device) {
	if (device == vr::k_unTrackedDeviceIndexInvalid) return vr::TrackedControllerRole_Invalid;

	// Simply check if the passed device matches our active left or right device
	if (device == getControllerForRole(vr::TrackedControllerRole_LeftHand)) {
		return vr::TrackedControllerRole_LeftHand;
	}
	if (device == getControllerForRole(vr::TrackedControllerRole_RightHand)) {
		return vr::TrackedControllerRole_RightHand;
	}

	return vr::TrackedControllerRole_Invalid;
}

bool SteamVRLogic::ConnectToVRRuntime() {
	m_eLastHmdError = vr::VRInitError_None;
    m_pVRSystem = vr::VR_Init(&m_eLastHmdError, vr::VRApplication_Overlay);

	if (m_eLastHmdError != vr::VRInitError_None) {
		m_strVRDisplay = "No Display";
		m_strVRDriver = "No Driver";
		return false;
	}

    vr::ETrackedPropertyError err;
	m_strVRDisplay = GetTrackedDeviceString(m_pVRSystem, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String);
    m_strVRDriver = GetTrackedDeviceString(m_pVRSystem, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String);

    return true;
}

void SteamVRLogic::DisconnectFromVRRuntime() {
    vr::VR_Shutdown();
}

// Qt doesn't support std::Strings so they must be converted to QStrings
QString SteamVRLogic::GetTrackedDeviceString(vr::IVRSystem *pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop) {
    char buf[vr::k_unMaxPropertyStringSize]; // Should be big enough (famous last words)
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
    	//proxy->setTransformOriginPoint(0,0);
    	//proxy->setScale(0.5); // Scale the whole widget down by a factor of 2 to save on GPU power.
    	//m_pScene->setSceneRect(0, 0, pWidget->width() * 0.5, pWidget->height() * 0.5); // Match scene to new size

    	proxy->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    }
    m_pWidget = pWidget;

	//int fboWidth = m_pScene->sceneRect().width();
	//int fboHeight = m_pScene->sceneRect().height();
	//m_pFbo = new QOpenGLFramebufferObject(fboWidth, fboHeight, GL_TEXTURE_2D);
	m_pFbo = std::make_unique<QOpenGLFramebufferObject>(pWidget->width(), pWidget->height(), GL_TEXTURE_2D);
	m_pMainPaintDevice = std::make_unique<QOpenGLPaintDevice>(pWidget->width(), pWidget->height());

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

	m_pPanicFbo = std::make_unique<QOpenGLFramebufferObject>(pWidget->width(), pWidget->height(), GL_TEXTURE_2D);
	m_pPanicPaintDevice = std::make_unique<QOpenGLPaintDevice>(pWidget->width(), pWidget->height());

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