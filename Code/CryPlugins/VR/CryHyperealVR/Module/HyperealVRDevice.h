#pragma once

#include <CrySystem/VR/IHMDDevice.h>
#include <CrySystem/VR/IHMDManager.h>

#include "../Interface/IHmdHyperealVRDevice.h"
#include "HyperealVRController.h"

#include <Cry3DEngine/IIndexedMesh.h>
#include <CryRenderer/IStereoRenderer.h>

struct IConsoleCmdArgs;
struct IRenderer;

namespace CryVR
{
namespace HyperealVR
{
class HyperealVRController;

class HyperealVRDevice : public IHyperealVRDevice, public IHmdEventListener, public ISystemEventListener
{
public:
	// IHmdDevice
	virtual void                    AddRef() override;
	virtual void                    Release() override;

	virtual EHmdClass               GetClass() const override { return eHmdClass_HyperealVR; }
	virtual void                    GetDeviceInfo(HmdDeviceInfo& info) const override { info = deviceInfo; }

	virtual void                    GetCameraSetupInfo(float& fov, float& aspectRatioFactor) const override;
	virtual void                    GetAsymmetricCameraSetupInfo(int nEye, float& fov, float& aspectRatio, float& asymH, float& asymV, float& eyeDist) const;
	virtual void                    UpdateInternal(EInternalUpdate type) override;
	virtual void                    RecenterPose() override { ResetOrientationAndPosition(0.0f); };
	virtual void                    UpdateTrackingState(EVRComponent type) override;
	virtual const HmdTrackingState& GetNativeTrackingState() const override;
	virtual const HmdTrackingState& GetLocalTrackingState() const override;
	virtual Quad                    GetPlayArea() const override;
	virtual Vec2                    GetPlayAreaSize() const override;
	virtual const IHmdController*   GetController() const override { return &controller; }
	virtual const EHmdSocialScreen  GetSocialScreenType(bool* pKeepAspect = nullptr) const override;
	virtual int                     GetControllerCount() const override { __debugbreak(); return 2; /* OPENVR_TODO */ }
	virtual void                    GetPreferredRenderResolution(unsigned int& width, unsigned int& height) override;
	virtual void                    DisableHMDTracking(bool disable) override;
	// ~IHmdDevice

	// IHyperealVRDevice
	virtual void SubmitOverlay(int id);
	virtual void SubmitFrame();
	virtual void OnSetupEyeTargets(ERenderAPI api, ERenderColorSpace colorSpace, void* leftEyeHandle, void* rightEyeHandle);
	virtual void OnSetupOverlay(int id, ERenderAPI api, ERenderColorSpace colorSpace, void* overlayTextureHandle);
	virtual void OnDeleteOverlay(int id);
	virtual void GetRenderTargetSize(uint& w, uint& h);
	virtual void GetMirrorImageView(EEyeType eye, void* resource, void** mirrorTextureView) override;
	// ~IHyperealVRDevice

	// ISystemEventListener
	virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam);
	// ~ISystemEventListener

	// IHmdEventListener
	virtual void OnRecentered() override;
	// ~IHmdEventListener

public:
	//
private:
	enum EDevice
	{
		Hmd,
		Left_Controller,
		Right_Controller,
		Totoal_Count,
	};

	void CopyPoseState(HmdPoseState & local, HmdPoseState & native, const HyTrackingState & src);
	inline void ResetOrientationAndPosition(float yaw)
	{
		ResetOrientation(yaw);
		ResetPosition();
	}
	void ResetOrientation(float yaw);
	inline void ResetPosition()
	{
		baseLocalPos = localStates[EDevice::Hmd].pose.position;
	};
	inline int GetFrameID() { return gEnv->pRenderer->GetFrameID(false); }

	volatile int refCount;
	HmdDeviceInfo deviceInfo;

	HmdTrackingState nativeStates[EDevice::Totoal_Count];
	HmdTrackingState localStates[EDevice::Totoal_Count];
	uint32 trackingFlags[EDevice::Totoal_Count];

	HyperealVRController controller;

	Quat baseLocalRot;
	Vec3 baseLocalPos;
	float interpuillaryDistance;
	float meterToWorld;
	EHmdSocialScreen socialScreenType;
	bool isHmdTrackingDisabled;

	HyDevice * device;
	HyGraphicsContext * graphicsContext;
	HyGraphicsContextDesc graphicsContextDesc;

	HyTextureDesc textureDesc[HY_EYE_MAX];
	void * texture[HY_EYE_MAX][2]; //double buffering.
};
}
}