#include "StdAfx.h"
#include "PluginDll.h"

#include "HyperealVRResources.h"
#include "HyperealVRDevice.h"

// Included only once per DLL module.
#include <CryCore/Platform/platform_impl.inl>

namespace CryVR
{
namespace HyperealVR
{

CPlugin_HyperealVR::~CPlugin_HyperealVR()
{
	CryVR::HyperealVR::HyperealVRResources::Shutdown();

	if (auto p = GetISystem()->GetISystemEventDispatcher())
		p->RemoveListener(this);
}
bool CPlugin_HyperealVR::Initialize(SSystemGlobalEnvironment & env, const SSystemInitParams & initParams)
{
	if (auto p = GetISystem()->GetISystemEventDispatcher())
		p->RegisterListener(this);

	return true;
}

IHyperealVRDevice * CPlugin_HyperealVR::CreateDevice()
{
	return GetDevice();
}

IHyperealVRDevice * CPlugin_HyperealVR::GetDevice() const
{
	return CryVR::HyperealVR::HyperealVRResources::GetAssociatedDevice();
}

void CPlugin_HyperealVR::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
	switch (event)
	{
	case ESYSTEM_EVENT_PRE_RENDERER_INIT:
		CryVR::HyperealVR::HyperealVRResources::Init();

		if (auto p = GetDevice())
			gEnv->pSystem->GetHmdManager()->RegisterDevice(GetName(), *p);

		break;
	default:
		break;
	}
}

CRYREGISTER_SINGLETON_CLASS(CPlugin_HyperealVR)
}
}