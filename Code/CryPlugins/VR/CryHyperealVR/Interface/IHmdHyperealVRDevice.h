// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/VR/IHMDDevice.h>

namespace CryVR
{
namespace HyperealVR
{
struct TextureDesc
{
	uint32 width;
	uint32 height;
	uint32 format;
};

enum ERenderAPI
{
	eRenderAPI_DirectX = 0,
};

enum ERenderColorSpace
{
	eRenderColorSpace_Auto   = 0,
	eRenderColorSpace_Gamma  = 1,
	eRenderColorSpace_Linear = 2,
};

struct IHyperealVRDevice : public IHmdDevice
{
public:
	virtual void OnSetupEyeTargets(void* leftEyeHandle, void* rightEyeHandle) = 0;
	virtual void OnSetupOverlay(int id, ERenderAPI api, ERenderColorSpace colorSpace, void* overlayTextureHandle) = 0;
	virtual void OnDeleteOverlay(int id) = 0;
	virtual void SubmitOverlay(int id) = 0;
	virtual void SubmitFrame() = 0;
	virtual void GetRenderTargetSize(uint& w, uint& h) = 0;
	virtual void GetMirrorImageView(EEyeType eye, void* resource, void** mirrorTextureView) = 0;
protected:
	virtual ~IHyperealVRDevice() {}
};
}
}