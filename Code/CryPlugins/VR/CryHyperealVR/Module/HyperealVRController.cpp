#include "StdAfx.h"

#include "HyperealVRController.h"

#include <CryRenderer/IRenderer.h>
#include <CryRenderer/IRenderAuxGeom.h>

namespace CryVR
{
namespace HyperealVR
{
static const float MaxPulseDuration = 5000.0f; // 5s max

static const uint32 HyLRPressMask[eHmdController_MaxNumHyperealVRControllers] = {
	HY_BUTTON_TOUCHPAD_LEFT | HY_BUTTON_LMENU, HY_BUTTON_TOUCHPAD_RIGHT | HY_BUTTON_RMENU 
};
static const uint32 HyLRTouchMask[eHmdController_MaxNumHyperealVRControllers] = {
	HY_TOUCH_TOUCHPAD_LEFT | HY_TOUCH_INDEX_TRIGGER_LEFT | HY_TOUCH_SIDE_TRIGGER_LEFT,
	HY_TOUCH_TOUCHPAD_RIGHT | HY_TOUCH_INDEX_TRIGGER_RIGHT | HY_TOUCH_SIDE_TRIGGER_RIGHT,
};

bool HyperealVRController::IsConnected(EHmdController id) const
{
	return controllerMapping[id] != HySubDevice::HY_SUBDEV_UNKNOWN;
}

void HyperealVRController::ApplyForceFeedback(EHmdController id, float freq, float amplitude)
{
	if (device != nullptr)
	{
		float duration = clamp_tpl(freq, 0.0f, MaxPulseDuration);
		device->SetControllerVibration(controllerMapping[id], duration, amplitude);
	}
}

HyperealVRController::HyperealVRController(HyDevice * device)
	: device(device)
{
	memset(&currState, 0, sizeof(currState));
	memset(&prevState, 0, sizeof(prevState));

	for (auto & mapping : controllerMapping)
		mapping = HY_SUBDEV_UNKNOWN;

	for (auto & symbol : symbols)
		symbol = nullptr;
}

#define SafeAssign(dest, src) \
do { if ((dest)) delete (dest); (dest) = (src);} while(0)

#define RegisterSymbol(suffix, type, deviceSpec) \
SafeAssign(symbols[HYKEY(suffix) - HYKEY(Base)], MapSymbol(deviceSpec, HYKEY(suffix), TKeyName(#suffix), type))

bool HyperealVRController::Init()
{
	RegisterSymbol(Menu,			SInputSymbol::EType::Button, 
		HyButton::HY_BUTTON_LMENU		| HyButton::HY_BUTTON_RMENU);
	RegisterSymbol(TouchpadBtn,		SInputSymbol::EType::Button,
		HyButton::HY_BUTTON_LTOUCHPAD	| HyButton::HY_BUTTON_RTOUCHPAD);

	RegisterSymbol(IndexTrigger,	SInputSymbol::EType::Trigger, 0);
	RegisterSymbol(SideTrigger,		SInputSymbol::EType::Trigger, 0);

	RegisterSymbol(IndexTriggerBtn, SInputSymbol::EType::Button,
		HyTouch::HY_TOUCH_INDEX_TRIGGER_LEFT	| HyTouch::HY_TOUCH_INDEX_TRIGGER_RIGHT);
	RegisterSymbol(SideTriggerBtn,	SInputSymbol::EType::Button,
		HyTouch::HY_TOUCH_SIDE_TRIGGER_LEFT		| HyTouch::HY_TOUCH_SIDE_TRIGGER_RIGHT);

	RegisterSymbol(Touchpad_X,		SInputSymbol::EType::Axis,
		HyTouch::HY_TOUCH_TOUCHPAD_LEFT | HyTouch::HY_TOUCH_TOUCHPAD_RIGHT);
	RegisterSymbol(Touchpad_Y,		SInputSymbol::EType::Axis,
		HyTouch::HY_TOUCH_TOUCHPAD_LEFT | HyTouch::HY_TOUCH_TOUCHPAD_RIGHT);

	return false;
}

#undef RegisterSymbol
#undef SafeAssign

#define SendEvent(suffix, eventType, comparator) \
if (prevState[index]. ## comparator != currState[index]. ##comparator) \
{ \
	SInputEvent event; \
	SInputSymbol * pSymbol = symbols[HYKEY(suffix) - HYKEY(Base)]; \
	pSymbol-> ## eventType(currState[index]. ## comparator); \
	pSymbol->AssignTo(event); \
	event.deviceIndex = index; \
	event.deviceType = eIDT_MotionController; \
	gEnv->pInput->PostInputEvent(event); \
} 
	
void HyperealVRController::Update(HySubDevice deviceID, HmdTrackingState nativePose, HmdTrackingState localPose, const HyInputState & inputState)
{
	if (deviceID != HY_SUBDEV_CONTROLLER_LEFT && deviceID != HY_SUBDEV_CONTROLLER_RIGHT)
		return; //not a controller

	uint32 index = deviceID & 0x01;

	//EHmdController index = eHmdController_MaxNumHyperealVRControllers;
	//
	//for (uint32 i = 0; i < eHmdController_MaxNumHyperealVRControllers; i++)
	//{
	//	if (controllerMapping[i] == deviceID)
	//	{
	//		index = (EHmdController) i;
	//		break;
	//	}
	//}

	if (controllerMapping[index] != HY_SUBDEV_UNKNOWN)
	{
		prevState[index] = currState[index];

		auto & state = currState[index];
		state.buttonsPressed = inputState.m_buttons & HyLRPressMask[index];
		state.buttonsTouched = inputState.m_touches & HyLRTouchMask[index];
		state.indexTriggerValue = inputState.m_indexTrigger;
		state.sideTriggerValue = inputState.m_sideTrigger;
		state.indexTriggerProximity = inputState.m_indexTriggerProximity;
		state.touchpadProximity = inputState.m_touchpadProximity;
		state.touchpadValue = Vec2(inputState.m_touchpad.x, inputState.m_touchpad.y);
		state.nativePose = nativePose;
		state.localPose = localPose;

		SendEvent(Menu,				PressEvent,		IsButtonPressed(HYKEY(Menu),			symbols));
		SendEvent(TouchpadBtn,		PressEvent,		IsButtonPressed(HYKEY(TouchpadBtn),		symbols));
		SendEvent(IndexTrigger,		ChangeEvent,	indexTriggerValue);
		SendEvent(SideTrigger,		ChangeEvent,	sideTriggerValue);
		SendEvent(IndexTriggerBtn,	PressEvent,		IsButtonTouched(HYKEY(IndexTriggerBtn),	symbols));
		SendEvent(SideTriggerBtn,	PressEvent,		IsButtonTouched(HYKEY(SideTriggerBtn),	symbols));
		SendEvent(Touchpad_X,		ChangeEvent,	touchpadValue.x);
		SendEvent(Touchpad_Y,		ChangeEvent,	touchpadValue.y);
	}
}

#undef SendEvent

SInputSymbol * HyperealVRController::MapSymbol(uint32 deviceSpecID, EKeyId keyID, const TKeyName & name, SInputSymbol::EType type, uint32 user)
{
	auto pSymbol = new SInputSymbol(deviceSpecID, keyID, name, type, user);
	pSymbol->state = (type == SInputSymbol::EType::Axis) ? eIS_Unknown : eIS_Pressed;
	pSymbol->deviceType = eIDT_MotionController;
	return pSymbol;
}

void HyperealVRController::OnControllerConnect(HySubDevice deviceID)
{
	if (deviceID != HY_SUBDEV_CONTROLLER_LEFT && deviceID != HY_SUBDEV_CONTROLLER_RIGHT)
		return; //not a controller

	uint32 index = deviceID & 0x01;

	if (controllerMapping[index] == HY_SUBDEV_UNKNOWN)
	{
		controllerMapping[index] = deviceID;
		currState[index] = ControllerState();
		prevState[index] = ControllerState();
		return;
	}

	//for (uint32 i = 0; i < eHmdController_MaxNumHyperealVRControllers; i++)
	//{
	//	if (controllerMapping[i] == HY_SUBDEV_UNKNOWN)
	//	{
	//		controllerMapping[i] = deviceID;
	//		currState[i] = ControllerState();
	//		prevState[i] = ControllerState();
	//		return;
	//	}
	//}

	return;
}

void HyperealVRController::OnControllerDisconnect(HySubDevice deviceID)
{
	if (deviceID != HY_SUBDEV_CONTROLLER_LEFT && deviceID != HY_SUBDEV_CONTROLLER_RIGHT)
		return; //not a controller

	uint32 index = deviceID & 0x01;
	controllerMapping[index] = HY_SUBDEV_UNKNOWN;

	//for (auto & mapping : controllerMapping)
	////for (uint32 i = 0; i < eHmdController_MaxNumHyperealVRControllers; i++)
	//{
	//	if (mapping == deviceID)
	//	//if (controllerMapping[i] == deviceID)
	//	{
	//		mapping = HY_SUBDEV_UNKNOWN;
	//		//controllerMapping[i] = HY_SUBDEV_UNKNOWN;
	//		return;
	//	}
	//}
	return;
}

void HyperealVRController::ClearState()
{
	for (auto & state : currState)
		state = ControllerState();
}

}
}