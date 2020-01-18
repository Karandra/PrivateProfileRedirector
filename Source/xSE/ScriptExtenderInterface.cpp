#include "stdafx.h"
#include "ScriptExtenderInterface.h"
#include "PrivateProfileRedirector.h"

#pragma warning(disable: 4005) // macro redefinition
#pragma warning(disable: 4244) // conversion from 'x' to 'y', possible loss of data
#pragma warning(disable: 4267) // conversion from 'size_t' to 'y', possible loss of data
#include "ScriptExtenderInterfaceIncludes.h"

#if xSE_PLATFORM_SKSE64
#include "RefreshINIOverriders/SKSE64.h"
#elif xSE_PLATFORM_F4SE
#include "RefreshINIOverriders/F4SE.h"
#elif xSE_PLATFORM_SKSEVR
#include "RefreshINIOverriders/SKSEVR.h"
#endif

//////////////////////////////////////////////////////////////////////////
bool xSE_QUERYFUNCTION(const xSE_Interface* xSE, PluginInfo* info)
{
	using namespace PPR;
	xSE_LOG("[" _CRT_STRINGIZE(xSE_QUERYFUNCTION) "] On query plugin info");

	// Set info
	info->infoVersion = PluginInfo::kInfoVersion;
	info->name = Redirector::GetLibraryNameA();
	info->version = Redirector::GetLibraryVersionInt();

	// Save handle
	PluginHandle pluginHandle = xSE->GetPluginHandle();

	// Check SE version
	const auto interfaceVersion = xSE_INTERFACE_VERSION(xSE);
	constexpr auto compiledVersion = xSE_PACKED_VERSION;
	if (!SEInterface::GetInstance().OnCheckVersion(interfaceVersion, compiledVersion))
	{
		xSE_LOG("This plugin might be incompatible with this version of " xSE_NAME_A);
		xSE_LOG("Script extender interface version '%u', expected '%u'", (uint32_t)interfaceVersion, (uint32_t)compiledVersion);
		xSE_LOG("Script extender functions will be disabled");

		pluginHandle = kPluginHandle_Invalid;
	}

	// Get the scale form interface and query its version.
	xSE_ScaleformInterface* scaleform = nullptr;
	#if xSE_HAS_SCALEFORM_INTERFACE
	if (pluginHandle != kPluginHandle_Invalid)
	{
		// Don't return, I don't use scaleform anyway.
		scaleform = static_cast<xSE_ScaleformInterface*>(xSE->QueryInterface(kInterface_Scaleform));
		if (scaleform == nullptr)
		{
			xSE_LOG("Couldn't get scaleform interface");
			//return false;
		}
		if (scaleform && scaleform->interfaceVersion < xSE_ScaleformInterface::kInterfaceVersion)
		{
			xSE_LOG("Scaleform interface too old (%d, expected %d)", (int)scaleform->interfaceVersion, (int)xSE_ScaleformInterface::kInterfaceVersion);
			//return false;
		}
	}
	#endif
	return SEInterface::GetInstance().OnQuery(pluginHandle, pluginHandle != kPluginHandle_Invalid ? xSE : nullptr, scaleform);
}
bool xSE_LOADFUNCTION(const xSE_Interface* xSE)
{
	using namespace PPR;
	xSE_LOG("[" _CRT_STRINGIZE(xSE_LOADFUNCTION) "] On load plugin");

	SEInterface& instance = SEInterface::GetInstance();
	if (instance.OnLoad())
	{
		Redirector::GetInstance().Log(L"[" xSE_NAME_A L"] %s %s loaded", Redirector::GetLibraryNameW(), Redirector::GetLibraryVersionW());
		#if xSE_HAS_SE_LOG
		if (SEInterface::GetInstance().CanUseSEFunctions())
		{
			_MESSAGE("[" xSE_NAME_A "] %s %s loaded", Redirector::GetLibraryNameA(), Redirector::GetLibraryVersionA());
		}
		#endif
		return true;
	}
	return false;
}

namespace PPR
{
	SEInterface& SEInterface::GetInstance()
	{
		static SEInterface ms_Instance;
		return ms_Instance;
	}

	bool SEInterface::RegisterScaleform(GFxMovieView* view, GFxValue* root)
	{
		return true;
	}

	bool SEInterface::OnQuery(PluginHandle pluginHandle, const xSE_Interface* xSE, xSE_ScaleformInterface* scaleform)
	{
		m_PluginHandle = pluginHandle;
		m_XSE = xSE;
		m_Scaleform = scaleform;

		return true;
	}
	bool SEInterface::OnCheckVersion(uint32_t interfaceVersion, uint32_t compiledVersion)
	{
		m_CanUseSEFunctions = interfaceVersion == compiledVersion || Redirector::GetInstance().IsOptionEnabled(RedirectorOption::AllowSEVersionMismatch);
		return m_CanUseSEFunctions;
	}
	bool SEInterface::OnLoad()
	{
		if (CanUseSEFunctions())
		{
			#if xSE_PLATFORM_SKSE64
			m_RefreshINIOverrider = std::make_unique<RefreshINIOverrider_SKSE64>();
			#elif xSE_PLATFORM_F4SE
			m_RefreshINIOverrider = std::make_unique<RefreshINIOverrider_F4SE>();
			#elif xSE_PLATFORM_SKSEVR
			m_RefreshINIOverrider = std::make_unique<RefreshINIOverrider_SKSEVR>();
			#endif

			if (m_RefreshINIOverrider)
			{
				m_RefreshINIOverrider->Execute();
			}
		}
		return true;
	}

	SEInterface::SEInterface()
		:m_PluginHandle(kPluginHandle_Invalid)
	{
	}
	SEInterface::~SEInterface() = default;
}
