// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/ICryPlugin.h>

#include "IHmdHyperealVRDevice.h"

namespace CryVR
{
namespace HyperealVR
{
struct IHyperealVRPlugin : public ICryPlugin
{
	CRYINTERFACE_DECLARE(IHyperealVRPlugin, 0x0, 0x0);

public:
	virtual IHyperealVRPlugin * CreateDevice() = 0;
	virtual IHyperealVRPlugin * GetDevice() const = 0;
};
}
}