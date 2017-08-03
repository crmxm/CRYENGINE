#include "StdAfx.h"

#include "HyperealVRDevice.h"
#include "HyperealVRController.h"
#include "HyperealVRResources.h"

#include "PluginDll.h"

#include <CrySystem/VR/IHMDManager.h>

#include <CryGame/IGameFramework.h>
#include <CryRenderer/IStereoRenderer.h>
#include <Cry3DEngine/IIndexedMesh.h>
#include <CryRenderer/IRenderAuxGeom.h>
#include <CryMath/Cry_Camera.h>

#include "../CryAction/IViewSystem.h"
#include <CryCore/Platform/CryWindows.h>

#define DEFAULT_IPD 0.064f;

namespace CryVR
{
namespace HyperealVR
{
inline Quat HmdQuatToWorldQuat(const Quat & quat)
{
	return Quat(quat.w, quat.v.x, -quat.v.z, quat.v.y);
	//return Quat::CreateRotationX(gf_PI * 0.5f) * quat * Quat::CreateRotationX(-gf_PI * 0.5f);
}

inline Vec3 HmdVec3ToWorldVec3(const Vec3 & vec)
{
	return Vec3(vec.x, -vec.z, vec.y);
}

inline Quat WorldQuatToHmdQuat (const Quat & quat)
{
	return Quat(quat.w, -quat.v.x, quat.v.z, -quat.v.y);
}

inline Vec3 WorldVec3ToHmdVec3(const Vec3 & vec)
{
	return { vec.x, vec.z, -vec.y };
}

inline Quat HyQuatToQuat(const HyQuat & quat)
{
	return HmdQuatToWorldQuat(Quat(quat.w, quat.x, quat.y, quat.z));
}

inline Vec3 HyVec3ToVec3(const HyVec3 & vec)
{
	return Vec3(vec.x, vec.y, vec.z);
}

inline HyQuat QuatToHyQuat(const Quat & quat)
{
	return { quat.w, quat.v.x, quat.v.y, quat.v.z };
}

inline HyVec3 Vec3ToHyVec3(const Vec3 & vec)
{
	return { vec.x, vec.y, vec.z };
}

void HyperealVRDevice::AddRef()
{
	CryInterlockedIncrement(&refCount);
}

void HyperealVRDevice::Release()
{
	long count = CryInterlockedDecrement(&refCount);

	if (count == 0)
		delete this;
}

void HyperealVRDevice::GetCameraSetupInfo(float & fov, float & aspectRatioFactor) const
{
	if (graphicsContext == nullptr)
		return;

	float fNear = gEnv->pRenderer->GetCamera().GetNearPlane();
	float fFar = gEnv->pRenderer->GetCamera().GetFarPlane();

	HyFov eyeFov;
	device->GetFloatArray( HY_PROPERTY_HMD_LEFT_EYE_FOV_FLOAT4_ARRAY, eyeFov.val, 4);
	
	HyMat4 proj;
	graphicsContext->GetProjectionMatrix(eyeFov, fNear, fFar, true, proj);

	fov = atanf(eyeFov.m_downTan) + atanf(eyeFov.m_upTan);

	aspectRatioFactor = proj.m[1][1] / proj.m[0][0];
}

void HyperealVRDevice::GetAsymmetricCameraSetupInfo(int nEye, float & fov, float & aspectRatio, 
	float & asymH, float & asymV, float & eyeDist) const
{
	if (graphicsContext == nullptr)
		return;

	float fNear = gEnv->pRenderer->GetCamera().GetNearPlane();
	float fFar = gEnv->pRenderer->GetCamera().GetFarPlane();

	HyFov eyeFov;
	device->GetFloatArray(
		nEye == 0 ? HY_PROPERTY_HMD_LEFT_EYE_FOV_FLOAT4_ARRAY : 
		HY_PROPERTY_HMD_RIGHT_EYE_FOV_FLOAT4_ARRAY, eyeFov.val, 4);
	
	HyMat4 proj;
	graphicsContext->GetProjectionMatrix(eyeFov, fNear, fFar, true, proj);

	fov = atanf(eyeFov.m_downTan) + atanf(eyeFov.m_upTan);

	aspectRatio = proj.m[1][1] / proj.m[0][0];

	const float denom = 1.0f / proj.m[1][1];
	asymH = proj.m[0][2] * denom * aspectRatio;
	asymV = proj.m[1][2] * denom;
	
	eyeDist = GetInterpupillaryDistance();
}

void HyperealVRDevice::UpdateInternal(EInternalUpdate type)
{
	if (!device || !graphicsContext)
		return;

	const HyMsgHeader * msg;
	while (true)
	{
		device->RetrieveMsg(&msg);
		if (msg->m_type == HyMsgType::HY_MSG_NONE)
			break;

		switch (msg->m_type)
		{
		case HyMsgType::HY_MSG_PENDING_QUIT:
			;//
			break;
		case HyMsgType::HY_MSG_INPUT_FOCUS_CHANGED:
		case HyMsgType::HY_MSG_VIEW_FOCUS_CHANGED:
			break;
		case HyMsgType::HY_MSG_SUBDEVICE_STATUS_CHANGED:
		{
			HyMsgSubdeviceChange * data = (HyMsgSubdeviceChange *)msg;
			HySubDevice subDevice = (HySubDevice)(data->m_subdevice);

			if (data->m_value != 0)
				controller.OnControllerConnect(subDevice);
			else
				controller.OnControllerDisconnect(subDevice);
			break;
		}
		default:
			break;
		}
	}
	//quittting;
}

void HyperealVRDevice::UpdateTrackingState(EVRComponent type)
{
	IRenderer * pRenderer = gEnv->pRenderer;

	const int frameID = pRenderer->GetFrameID(false);
	//TODO: editor

	if (device)
	{
		static const HySubDevice devices[EDevice::Totoal_Count] = {
			HY_SUBDEV_CONTROLLER,
			HY_SUBDEV_CONTROLLER_LEFT,
			HY_SUBDEV_CONTROLLER_RIGHT,
		};

		HyTrackingState trackingState;
		HyPose eyePose[HY_EYE_MAX];

		for (uint32 i = 0; i < EDevice::Totoal_Count; i++)
		{
			HyResult r = device->GetTrackingState(devices[i], frameID, trackingState);

			if (hySucceeded(r))
			{
				if (i == EDevice::Hmd)
				{
					float * pIPD = (interpupillaryDistance > 0.01f ? &interpupillaryDistance : nullptr);
					graphicsContext->GetEyePoses(trackingState.m_pose, pIPD, eyePose);
				}

				trackingFlags[i] = trackingState.m_flags;

				if (i == EDevice::Hmd)
				{
					uint32 statusFlag =
						((trackingFlags[i] & HY_TRACKING_POSITION_TRACKED) != 0 ? eHmdStatus_PositionConnected : 0) |
						((trackingFlags[i] & HY_TRACKING_ROTATION_TRACKED) != 0 ? eHmdStatus_OrientationTracked : 0) |
						eHmdStatus_CameraPoseTracked |
						(trackingFlags[i] ? (eHmdStatus_HmdConnected | eHmdStatus_PositionConnected) : 0);

					localStates[i].statusFlags = nativeStates[i].statusFlags = statusFlag;
				}
				
				if (trackingFlags[i])
					CopyPoseState(localStates[i].pose, nativeStates[i].pose, trackingState);
			}

			if (i == EDevice::Hmd)
			{
				auto eyeLocalRot = HmdQuatToWorldQuat(HyQuatToQuat(eyePose[HY_EYE_LEFT].m_rotation));
				auto eyeLocalPos = HmdVec3ToWorldVec3(HyVec3ToVec3(eyePose[HY_EYE_LEFT].m_position));

				localStates[i].pose.orientation = baseLocalRot.GetInverted().GetNormalized() * eyeLocalRot;
				localStates[i].pose.position = (eyeLocalPos - baseLocalPos) * meterToWorld;

				if (isHmdTrackingDisabled)
					localStates[i].pose.position.x = localStates[i].pose.position.y = 0.0f;

				nativeStates[i].pose.orientation = WorldQuatToHmdQuat(localStates[i].pose.orientation);
				nativeStates[i].pose.position = WorldVec3ToHmdVec3(localStates[i].pose.position);
			}
			else if (devices[i] == HY_SUBDEV_CONTROLLER_LEFT || devices[i] == HY_SUBDEV_CONTROLLER_RIGHT)
			{
				HyResult res;
				HyInputState inputState;

				res = device->GetControllerInputState(devices[i], inputState);

				if (hySucceeded(res))
				{
					localStates[i].statusFlags = nativeStates[i].statusFlags = localStates[EDevice::Hmd].statusFlags;

					controller.Update(devices[i], nativeStates[i], localStates[i], inputState);
				}
			}
		}
	}
}

const HmdTrackingState & HyperealVRDevice::GetNativeTrackingState() const
{
	return nativeStates[EDevice::Hmd];
}

const HmdTrackingState & HyperealVRDevice::GetLocalTrackingState() const
{
	return nativeStates[EDevice::Hmd];
}

Quad HyperealVRDevice::GetPlayArea() const
{
	return Quad();
}

Vec2 HyperealVRDevice::GetPlayAreaSize() const
{
	return Vec2();
}

const EHmdSocialScreen HyperealVRDevice::GetSocialScreenType(bool * pKeepAspect) const
{
	if (pKeepAspect)
		*pKeepAspect = (socialScreenType != EHmdSocialScreen::DistortedDualImage);

	return socialScreenType;
}

void HyperealVRDevice::GetPreferredRenderResolution(unsigned int & width, unsigned int & height)
{
	int64 temp;
	device->GetIntValue(HY_PROPERTY_HMD_RESOLUTION_X_INT, temp);
	width = (unsigned int) temp;
	device->GetIntValue(HY_PROPERTY_HMD_RESOLUTION_Y_INT, temp);
	height = (unsigned int)temp;
}

void HyperealVRDevice::DisableHMDTracking(bool disable)
{
	isHmdTrackingDisabled = disable;
}

void HyperealVRDevice::SubmitOverlay(int id)
{
}

void HyperealVRDevice::SubmitFrame()
{
	auto frameID = GetFrameID();
	for (int i = 0; i < HY_EYE_MAX; i++)
		textureDesc[i].m_texture = texture[frameID & 0x1];

	if (graphicsContext)
	{
		HyResult res = HyResult::hySuccess;

		res = graphicsContext->Submit(frameID, textureDesc, HY_EYE_MAX);
	}
}

void HyperealVRDevice::OnSetupEyeTargets(void * leftEyeHandle, void * rightEyeHandle)
{
	for (int i = 0; i < HY_EYE_MAX; i++)
	{
		textureDesc[i].m_uvSize = { 1.0f, 1.0f };
		textureDesc[i].m_uvOffset = { 0.0f, 0.0f };
	}

	textureDesc[HY_EYE_LEFT].m_texture = leftEyeHandle;
	textureDesc[HY_EYE_RIGHT].m_texture = rightEyeHandle;
}

void HyperealVRDevice::OnSetupOverlay(int id, ERenderAPI api, ERenderColorSpace colorSpace, void * overlayTextureHandle)
{
}

void HyperealVRDevice::OnDeleteOverlay(int id)
{
}

void HyperealVRDevice::GetRenderTargetSize(uint & w, uint & h)
{
	GetPreferredRenderResolution(w, h);
}

void HyperealVRDevice::GetMirrorImageView(EEyeType eye, void * resource, void ** mirrorTextureView)
{
	if (graphicsContext)
		graphicsContext->CopyMirrorTexture(resource, graphicsContextDesc.m_mirrorWidth, 
			graphicsContextDesc.m_mirrorHeight);
}

void HyperealVRDevice::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
	switch (event)
	{
	case ESYSTEM_EVENT_LEVEL_LOAD_START_LOADINGSCREEN:
		;// TODO: loading screen;
		break;

	case ESYSTEM_EVENT_LEVEL_LOAD_END:
	case ESYSTEM_EVENT_LEVEL_LOAD_ERROR:
		;
	case ESYSTEM_EVENT_LEVEL_GAMEPLAY_START:
		controller.ClearState();
		break;

	default:
		break;
	}
}

void HyperealVRDevice::OnRecentered()
{
	RecenterPose();
}

HyperealVRDevice::HyperealVRDevice(HyDevice * device)
	: refCount(1), device(device), controller(device), graphicsContext(nullptr),
	interpupillaryDistance(0.0f), meterToWorld(1.0f), isHmdTrackingDisabled(false),
	isQuitting(false), isResetRotKeepPitchAndRoll(false)
{
	memset(textureDesc, 0, sizeof(textureDesc));
	memset(texture, 0, sizeof(texture));
	memset(nativeStates, 0, sizeof(nativeStates));
	memset(localStates, 0, sizeof(localStates));
	memset(trackingFlags, 0, sizeof(trackingFlags));

	CreateDevice();

	gEnv->pSystem->GetHmdManager()->AddEventListener(this);

	if (auto p = GetISystem()->GetISystemEventDispatcher())
		p->RegisterListener(this);

	controller.Init();

	bool b;

	device->GetBoolValue(HY_PROPERTY_DEVICE_CONNECTED_BOOL, b, HY_SUBDEV_CONTROLLER_LEFT);
	if (b)
		controller.OnControllerConnect(HY_SUBDEV_CONTROLLER_LEFT);

	device->GetBoolValue(HY_PROPERTY_DEVICE_CONNECTED_BOOL, b, HY_SUBDEV_CONTROLLER_RIGHT);
	if (b)
		controller.OnControllerConnect(HY_SUBDEV_CONTROLLER_RIGHT);

}

HyperealVRDevice::~HyperealVRDevice()
{
	Shutdown();
	gEnv->pSystem->GetHmdManager()->RemoveEventListener(this);

	if (auto p = GetISystem()->GetISystemEventDispatcher())
		p->RemoveListener(this);
}

void HyperealVRDevice::CreateDevice()
{
	float fNear = gEnv->pRenderer->GetCamera().GetNearPlane();
	float fFar = gEnv->pRenderer->GetCamera().GetFarPlane();

	HyFov eyeFov;
	device->GetFloatArray( HY_PROPERTY_HMD_LEFT_EYE_FOV_FLOAT4_ARRAY, eyeFov.val, 4);
	
	float fovV = atanf(eyeFov.m_downTan) + atanf(eyeFov.m_upTan);
	float fovH = atanf(eyeFov.m_leftTan) + atanf(eyeFov.m_rightTan);

	memset(&deviceInfo, 0, sizeof(deviceInfo));

	GetPreferredRenderResolution(deviceInfo.screenWidth, deviceInfo.screenHeight);
	deviceInfo.manufacturer = GetTrackedDeviceCharPointer(HY_PROPERTY_MANUFACTURER_STRING);
	deviceInfo.productName = GetTrackedDeviceCharPointer(HY_PROPERTY_PRODUCT_NAME_STRING);

	deviceInfo.fovH = fovH;
	deviceInfo.fovV = fovV;
}

void HyperealVRDevice::Shutdown()
{
	SAFE_DELETE(device);

	//if initialized
	HyShutdown();
}

// TODO: don't know why always get value from device. the param ipd seems to be meaningless under this situation.
float HyperealVRDevice::GetInterpupillaryDistance() const
{
	if (interpupillaryDistance > 0.01f)
		return interpupillaryDistance;

	if (false) // a flag that denote whether system is valid is used for lumberyard, but seems to be always false.
	{
		bool isConnected = false;
		HyResult res = device->GetBoolValue(HY_PROPERTY_DEVICE_CONNECTED_BOOL, isConnected, HY_SUBDEV_HMD);
		{
			float ipd = DEFAULT_IPD;
			res = device->GetFloatValue(HY_PROPERTY_HMD_IPD_FLOAT, ipd);
			if (hySucceeded(res))
				return ipd;
		}
	}
	return DEFAULT_IPD;
}

void HyperealVRDevice::CopyPoseState(HmdPoseState & local, HmdPoseState & nativePose, const HyTrackingState & src)
{
	nativePose.position = HyVec3ToVec3(src.m_pose.m_position);
	nativePose.orientation = HyQuatToQuat(src.m_pose.m_rotation);
	nativePose.linearVelocity = HyVec3ToVec3(src.m_linearVelocity);
	nativePose.angularVelocity = HyVec3ToVec3(src.m_angularVelocity);
	nativePose.linearAcceleration = HyVec3ToVec3(src.m_linearAcceleration);
	nativePose.angularAcceleration = HyVec3ToVec3(src.m_angularAcceleration);

	local.position = HmdVec3ToWorldVec3(nativePose.position);
	local.orientation = HmdQuatToWorldQuat(nativePose.orientation);
	local.linearVelocity = HmdVec3ToWorldVec3(nativePose.linearVelocity);
	local.angularVelocity = HmdVec3ToWorldVec3(nativePose.angularVelocity);
	local.linearAcceleration = HmdVec3ToWorldVec3(nativePose.linearAcceleration);
	local.angularAcceleration = HmdVec3ToWorldVec3(nativePose.angularAcceleration);
}

void HyperealVRDevice::ResetOrientation(float yaw)
{
	Ang3 ang = Ang3(localStates[EDevice::Hmd].pose.orientation);

	if (isResetRotKeepPitchAndRoll)
		ang.y = ang.z = 0.0f;

	if (fabs(yaw) > FLT_EPSILON)
		ang.z -= yaw;

	baseLocalRot = Quat(ang);
}

const char* HyperealVRDevice::GetTrackedDeviceCharPointer(int nProperty)
{
	int realStrLen = 512;

	char* pBuffer = new char[realStrLen];
	device->GetStringValue(HY_PROPERTY_MANUFACTURER_STRING, pBuffer, realStrLen, &realStrLen);
	return const_cast<char*>(pBuffer);
}

int HyperealVRDevice::GetFrameID() { return gEnv->pRenderer->GetFrameID(false); }

}
}
