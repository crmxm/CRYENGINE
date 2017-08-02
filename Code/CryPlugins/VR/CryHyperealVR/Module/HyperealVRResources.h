// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace CryVR
{
namespace HyperealVR
{
class HyperealVRDevice;

class Resources
{
public:
	static HyperealVRDevice * GetAssociatedDevice();

	static void Init();
	static void PostInit();
	static void Shutdown();

private:
	Resources();
	~Resources();

private:
	//
};
}
}