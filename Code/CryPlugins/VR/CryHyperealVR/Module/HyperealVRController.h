// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/VR/IHMDDevice.h>
#include <CryInput/IInput.h>

#include "Hypereal_VR.h"

#define HYKEY(suffix) eKI_Motion_HyperealVR_ ## suffix

namespace CryVR
{
namespace HyperealVR
{
class HyperealVRController : public IHmdController
{
public:
	// IHmdController
	virtual bool                    IsConnected(EHmdController id) const override;
	virtual bool                    IsButtonPressed(EHmdController controllerId, EKeyId id) const override { return (id < HYKEY(Base) || id >= HYKEY(Trigger_Base)) ? false : (currState[controllerId].IsButtonPressed(id, symbols)); };
	virtual bool                    IsButtonTouched(EHmdController controllerId, EKeyId id) const override { return (id < HYKEY(Touch_Base) || id > HYKEY(End)) ? false : (currState[controllerId].IsButtonTouched(id, symbols)); };
	virtual bool                    IsGestureTriggered(EHmdController controllerId, EKeyId id) const override { return false; }                           // HyperealVR does not have gesture support (yet?)
	virtual float                   GetTriggerValue(EHmdController controllerId, EKeyId id) const override { return currState[controllerId].indexTriggerValue; }   // we only have one trigger => ignore trigger id
	virtual Vec2                    GetThumbStickValue(EHmdController controllerId, EKeyId id)const override { return currState[controllerId].touchpadValue; }  // we only have one 'stick' (/the touch pad) => ignore thumb stick id

	virtual const HmdTrackingState& GetNativeTrackingState(EHmdController controller) const override { return currState[controller].nativePose; }
	virtual const HmdTrackingState& GetLocalTrackingState(EHmdController controller) const override { return currState[controller].localPose; }

	virtual void                    ApplyForceFeedback(EHmdController id, float freq, float amplitude) override;
	virtual void                    SetLightColor(EHmdController id, TLightColor color) override {}
	virtual TLightColor             GetControllerColor(EHmdController id) const override { return 0; }
	virtual uint32                  GetCaps(EHmdController id) const override { return (eCaps_Buttons | eCaps_Tracking | eCaps_Sticks | eCaps_Capacitors); }
	// ~IHmdController

	~HyperealVRController()
	{
		for (auto & p : symbols)
			SAFE_DELETE(p);
	}
private:
	friend class HyperealVRDevice;

	HyperealVRController(HyDevice * device);
	HyperealVRController(const HyperealVRController &) = delete;
	HyperealVRController& operator = (const HyperealVRController &) = delete;

	struct ControllerState
	{
		ControllerState() {};
		ControllerState(const ControllerState &) = default;
		ControllerState& operator = (const ControllerState &) = default;

		inline bool IsButtonPressed(const EKeyId keyID, 
			const SInputSymbol * const symbols[eKI_Motion_HyperealVR_NUM_SYMBOLS]) const
		{
			return (buttonsPressed & symbols[keyID - HYKEY(Base)]->devSpecId) != 0;
		}

		inline bool IsButtonTouched(const EKeyId keyID, 
			const SInputSymbol * const symbols[eKI_Motion_HyperealVR_NUM_SYMBOLS]) const
		{
			return (buttonsTouched & symbols[keyID - HYKEY(Base)]->devSpecId) != 0;
		}

		~ControllerState() {};
		
		uint32 buttonsPressed;
		uint32 buttonsTouched;
		float indexTriggerValue;
		float sideTriggerValue;
		float indexTriggerProximity;
		float touchpadProximity;
		Vec2 touchpadValue;

		HmdTrackingState nativePose;
		HmdTrackingState localPose;
	};

	bool Init();
	void Update(HySubDevice deviceID, HmdTrackingState nativePose, HmdTrackingState localPose, const HyInputState & inputState);
	SInputSymbol * MapSymbol(uint32 deviceSpecID, EKeyId keyID, const TKeyName & name, SInputSymbol::EType type, uint32 user = 0);
	void OnControllerConnect(HySubDevice deviceID);
	void OnControllerDisconnect(HySubDevice deviceID);
	void ClearState();

	ControllerState currState[eHmdController_MaxNumHyperealVRControllers];
	ControllerState prevState[eHmdController_MaxNumHyperealVRControllers];

	SInputSymbol * symbols[eKI_Motion_HyperealVR_NUM_SYMBOLS];
	
	HySubDevice controllerMapping[eHmdController_MaxNumHyperealVRControllers];

	HyDevice * device;
};
}
}
