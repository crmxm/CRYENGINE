// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "../Interface/IHyperealVRPlugin.h"

namespace CryVR
{
namespace HyperealVR
{
class CPlugin_HyperealVR : public IHyperealVRPlugin, public ISystemEventListener
{
	CRYINTERFACE_BEGIN()
	CRYINTERFACE_ADD(IHyperealVRPlugin)
	CRYINTERFACE_ADD(ICryPlugin)
	CRYINTERFACE_END()

	CRYGENERATE_SINGLETONCLASS(CPlugin_HyperealVR, "Plugin_HyperealVR", 0x0, 0x0)

	virtual ~CPlugin_HyperealVR();

	virtual const char * GetName() const override { return "CryHyperealVR"; };

	virtual const char * GetCategory() const override { return "Plugin"; };

	virtual bool Initialize(SSystemGlobalEnvironment & env, const SSystemInitParams & initParams) override;

public:
	virtual IHyperealVRDevice * CreateDevice() override;
	virtual IHyperealVRDevice * GetDevice() const override;

	virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;

public:
	//

protected:
	virtual void OnPluginUpdate(EPluginUpdateType updateType) override {};

};
}
}