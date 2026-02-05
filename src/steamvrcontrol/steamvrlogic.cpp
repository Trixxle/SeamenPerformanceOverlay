//
// Created by jornt on 05/02/2026.
//

#include "steamvrlogic.h"

SteamVRLogic::SteamVRLogic():
m_eLastHmdError(vr::VRInitError_None),
m_Widget(NULL)
{}

SteamVRLogic::~SteamVRLogic() {

}


bool SteamVRLogic::Init() {
    ConnectToVRRuntime();
    return true;
}

void SteamVRLogic::Shutdown() {
    vr::VR_Shutdown();
}


bool SteamVRLogic::ConnectToVRRuntime() {
    vr::VR_Init(&m_eLastHmdError, vr::VRApplication_Overlay);
    return false;
}


