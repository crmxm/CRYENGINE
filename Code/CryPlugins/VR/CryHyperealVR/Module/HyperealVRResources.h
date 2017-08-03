// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace CryVR
{
namespace HyperealVR
{
class HyperealVRDevice;

class HyperealVRResources
{
public:
	static HyperealVRDevice * GetAssociatedDevice() { return resources ? resources->device : nullptr; }

	static void Init();
	static void PostInit() {};
	static void Shutdown();

private:
	HyperealVRResources();
	~HyperealVRResources();

private:
	//
	static HyperealVRResources * resources;
	static bool isLibInitialized;

	HyperealVRDevice * device;
};
}
}