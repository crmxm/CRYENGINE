#include "StdAfx.h"

#include "HyperealVRResources.h"
#include "HyperealVRDevice.h"

namespace CryVR
{
namespace HyperealVR
{

HyperealVRResources * HyperealVRResources::resources = nullptr;
bool HyperealVRResources::isLibInitialized = false;


void HyperealVRResources::Init()
{
	if (!resources)
		resources = new HyperealVRResources();
}

void HyperealVRResources::Shutdown()
{
	SAFE_DELETE(resources);
}

HyperealVRResources::HyperealVRResources()
	: device(nullptr)
{
	if (strcmp(gEnv->pConsole->GetCVar("r_driver")->GetString(), "DX11") != 0)
		return;

	HyDevice * hyDevice;
	HyResult res = HyCreateInterface(HyDevice_InterfaceName, 0, (void**)&hyDevice);

	if (!hySucceeded(res))
	{
		device = nullptr;
		return;
	}

	isLibInitialized = true;

	bool b;
	hyDevice->GetBoolValue(HY_PROPERTY_DEVICE_CONNECTED_BOOL, b, HY_SUBDEV_HMD);

	if (!b)
		return;
	else
		device = HyperealVRDevice::CreateInstance(hyDevice);
}

HyperealVRResources::~HyperealVRResources()
{
	SAFE_DELETE(device);

	if (isLibInitialized)
		HyShutdown();

}

}
}