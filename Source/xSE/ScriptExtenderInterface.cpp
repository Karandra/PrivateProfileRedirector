#include "stdafx.h"
#include "ScriptExtenderInterface.h"
#include "ScriptExtenderInterfaceIncludes.h"
#include "DLLApplication.h"
#include "Redirector/RedirectorInterface.h"
#include <kxf/Core/IEncodingConverter.h>

#if xSE_PLATFORM_SKSE
#include "ConsoleCommandOverrider/SKSE.h"
#elif xSE_PLATFORM_SKSE64 || xSE_PLATFORM_SKSE64AE || xSE_PLATFORM_SKSEVR
#include "ConsoleCommandOverrider/SKSE64.h"
#elif xSE_PLATFORM_F4SE || xSE_PLATFORM_F4SEVR
#include "ConsoleCommandOverrider/F4SE.h"
#endif

//////////////////////////////////////////////////////////////////////////
bool xSE_PRELOADFUNCTION(const xSE_Interface* xSE)
{
	xSE_LOG("[{}] On preload plugin", _CRT_STRINGIZE(xSE_PRELOADFUNCTION));

	if (xSE)
	{
		xSE_LOG("Preloaded by Script Extender");
	}
	else
	{
		xSE_LOG("Preloaded by xSE PluginPreloader");
	}
	PPR::DLLApplication::InvokeOnProcessAttach();

	return true;
}
bool xSE_QUERYFUNCTION(const xSE_Interface* xSE, PluginInfo* pluginInfo)
{
	using namespace PPR;
	xSE_LOG("[{}] On query plugin info", _CRT_STRINGIZE(xSE_QUERYFUNCTION));

	// Set info
	if (pluginInfo)
	{
		struct PluginInfoData final
		{
			std::string Name;
			uint32_t Version = 0;

			PluginInfoData()
			{
				Name = PPR::DLLApplication::GetInstance().GetName().utf8_str().str();
				Version = PPR::VersionFull;
			}
		};
		static const PluginInfoData data;

		pluginInfo->infoVersion = PluginInfo::kInfoVersion;
		pluginInfo->name = data.Name.c_str();
		pluginInfo->version = data.Version;
	}

	// Save handle
	PluginHandle pluginHandle = xSE->GetPluginHandle();

	// Check SE version
	const auto interfaceVersion = xSE_INTERFACE_VERSION(xSE);
	constexpr auto compiledVersion = xSE_PACKED_VERSION;
	if (!XSEInterface::GetInstance().OnCheckVersion(interfaceVersion, compiledVersion))
	{
		xSE_LOG_WARNING("This plugin might be incompatible with this version of {}", xSE_NAME_A);
		xSE_LOG_WARNING("Script Extender interface version '{}', expected '{}', Script Extender functions will be disabled", interfaceVersion, compiledVersion);

		pluginHandle = kPluginHandle_Invalid;
	}

	// Get the scaleform interface and query its version.
	xSE_ScaleformInterface* scaleform = nullptr;
	#if xSE_HAS_SCALEFORM_INTERFACE
	if (pluginHandle != kPluginHandle_Invalid)
	{
		scaleform = static_cast<xSE_ScaleformInterface*>(xSE->QueryInterface(kInterface_Scaleform));
		if (!scaleform)
		{
			xSE_LOG_WARNING("Couldn't get scaleform interface");
		}
		if (scaleform && scaleform->interfaceVersion < xSE_ScaleformInterface::kInterfaceVersion)
		{
			xSE_LOG_WARNING("Scaleform interface too old ({}, expected {})", scaleform->interfaceVersion, xSE_ScaleformInterface::kInterfaceVersion);
			scaleform = nullptr;
		}
	}
	#endif

	// Get the messaging interface and query its version.
	xSE_MessagingInterface* messaging = nullptr;
	#if xSE_HAS_MESSAGING_INTERFACE
	if (pluginHandle != kPluginHandle_Invalid)
	{
		messaging = static_cast<xSE_MessagingInterface*>(xSE->QueryInterface(kInterface_Messaging));
		if (!messaging)
		{
			xSE_LOG_WARNING("Couldn't get messaging interface");
		}
		if (messaging && messaging->interfaceVersion < xSE_MessagingInterface::kInterfaceVersion)
		{
			xSE_LOG_WARNING("Messaging interface too old ({}, expected {})", messaging->interfaceVersion, xSE_MessagingInterface::kInterfaceVersion);
			messaging = nullptr;
		}
	}
	#endif

	return XSEInterface::GetInstance().OnQuery(pluginHandle, pluginHandle != kPluginHandle_Invalid ? xSE : nullptr, scaleform, messaging);
}
bool xSE_LOADFUNCTION(const xSE_Interface* xSE)
{
	using namespace PPR;
	xSE_LOG("[{}] On load plugin", _CRT_STRINGIZE(xSE_LOADFUNCTION));

	#if xSE_PLATFORM_SKSE64AE || xSE_PLATFORM_F4SE
	if (PluginInfo pluginInfo; !xSE_QUERYFUNCTION(xSE, &pluginInfo))
	{
		return false;
	}
	#endif

	XSEInterface& instance = XSEInterface::GetInstance();
	if (instance.OnLoad())
	{
		auto& app = PPR::DLLApplication::GetInstance();
		xSE_LOG(L"[{}] {} v{} by {} loaded", xSE_NAME_A, app.GetDisplayName(), app.GetVersion().ToString(), app.GetVendorDisplayName());
		#if xSE_PLATFORM_GOG
		xSE_LOG(L"GOG build loaded");
		#endif
		return true;
	}
	return false;
}

#if xSE_PLATFORM_SKSE64AE
extern "C" __declspec(dllexport) constinit auto SKSEPlugin_Version = []() constexpr
{
	SKSEPluginVersionData versionData = {};
	versionData.dataVersion = SKSEPluginVersionData::kVersion;
	versionData.compatibleVersions[0] = xSE_RUNTIME_VERSION;
	versionData.versionIndependence = SKSEPluginVersionData::kVersionIndependent_Signatures;
	versionData.versionIndependenceEx = SKSEPluginVersionData::kVersionIndependentEx_NoStructUse;
	versionData.pluginVersion = PPR::VersionFull;

	std::ranges::copy(PPR::ProjectName, versionData.name);
	std::ranges::copy(PPR::ProjectAuthor, versionData.author);

	return versionData;
}();
#elif xSE_PLATFORM_F4SE
extern "C" __declspec(dllexport) constinit auto F4SEPlugin_Version = []() constexpr
{
	F4SEPluginVersionData versionData = {};
	versionData.dataVersion = F4SEPluginVersionData::kVersion;
	versionData.compatibleVersions[0] = xSE_RUNTIME_VERSION;
	versionData.addressIndependence = F4SEPluginVersionData::kAddressIndependence_Signatures;
	versionData.structureIndependence = F4SEPluginVersionData::kStructureIndependence_NoStructs|F4SEPluginVersionData::kStructureIndependence_1_10_980Layout;
	versionData.pluginVersion = PPR::VersionFull;

	std::ranges::copy(PPR::ProjectName, versionData.name);
	std::ranges::copy(PPR::ProjectAuthor, versionData.author);

	return versionData;
}();
#endif

bool xSE_CAN_USE_XSE() noexcept
{
	return PPR::XSEInterface::GetInstance().CanUseSEFunctions();
}
void xSE_FORWARD_LOG_TO_XSE(kxf::LogLevel logLevel, kxf::StringView logString)
{
	#if xSE_HAS_SE_LOG
	if (!xSE_CAN_USE_XSE())
	{
		return;
	}

	auto utf8 = kxf::EncodingConverter_UTF8.ToMultiByte(logString);
	switch (logLevel)
	{
		case kxf::LogLevel::Critical:
		{
			::_FATALERROR("[%s] %s", xSE_NAME_A, utf8.c_str());
			break;
		}
		case kxf::LogLevel::Error:
		{
			::_ERROR("[%s] %s", xSE_NAME_A, utf8.c_str());
			break;
		}
		case kxf::LogLevel::Warning:
		{
			::_WARNING("[%s] %s", xSE_NAME_A, utf8.c_str());
			break;
		}
		case kxf::LogLevel::Debug:
		{
			::_DMESSAGE("[%s] %s", xSE_NAME_A, utf8.c_str());
			break;
		}
		case kxf::LogLevel::Trace:
		{
			::_VMESSAGE("[%s] %s", xSE_NAME_A, utf8.c_str());
			break;
		}
		default:
		{
			::_MESSAGE("[%s] %s", xSE_NAME_A, utf8.c_str());
			break;
		}
	};
	#endif
}

namespace PPR
{
	// XSEInterface
	XSEInterface& XSEInterface::GetInstance() noexcept
	{
		return PPR::DLLApplication::GetInstance().GetXSEInterface();
	}

	void XSEInterface::LoadConfig(DLLApplication& app, const AppConfigLoader& config)
	{
		KXF_SCOPEDLOG_FUNC;

		// Load options
		config.LoadXSEOption(m_Options, XSEOption::AllowVersionMismatch, L"AllowVersionMismatch");

		// Print options
		KXF_SCOPEDLOG.Info().Format("AllowVersionMismatch: {}", m_Options.Contains(XSEOption::AllowVersionMismatch));

		KXF_SCOPEDLOG.SetSuccess();
	}

	bool XSEInterface::OnCheckVersion(uint32_t interfaceVersion, uint32_t compiledVersion)
	{
		m_CanUseSEFunctions = interfaceVersion == compiledVersion || m_Options.Contains(XSEOption::AllowVersionMismatch);
		return m_CanUseSEFunctions;
	}
	bool XSEInterface::OnQuery(PluginHandle pluginHandle, const xSE_Interface* xSE, xSE_ScaleformInterface* scaleform, xSE_MessagingInterface* messaging)
	{
		m_PluginHandle = pluginHandle;
		m_XSE = xSE;
		m_Scaleform = scaleform;
		m_Messaging = messaging;

		return true;
	}
	bool XSEInterface::OnLoad()
	{
		if (CanUseSEFunctions())
		{
			InitGameMessageDispatcher();
			InitConsoleCommandOverrider();
		}
		return true;
	}

	bool XSEInterface::DoPrintConsole(const char* string) const
	{
		xSE_LOG("Printed to console: '{}'", string);

		#if !xSE_PLATFORM_NVSE
		Console_Print("%s", string);
		return true;
		#else
		return false;
		#endif
	}
	void XSEInterface::InitConsoleCommandOverrider()
	{
		KXF_SCOPEDLOG_FUNC;

		if (CanUseSEFunctions() && !m_ConsoleCommandOverrider)
		{
			#if xSE_PLATFORM_SKSE
			m_ConsoleCommandOverrider = std::make_unique<ConsoleCommandOverrider_SKSE>(*this);
			#elif xSE_PLATFORM_SKSE64 || xSE_PLATFORM_SKSE64AE|| xSE_PLATFORM_SKSEVR
			m_ConsoleCommandOverrider = std::make_unique<ConsoleCommandOverrider_SKSE64>(*this);
			#elif xSE_PLATFORM_F4SE || xSE_PLATFORM_F4SEVR
			m_ConsoleCommandOverrider = std::make_unique<ConsoleCommandOverrider_F4SE>(*this);
			#endif

			if (m_ConsoleCommandOverrider)
			{
				m_ConsoleCommandOverrider->OverrideCommand("RefreshINI", kxf::Format("[{}] Reloads INI files content from disk and calls the original 'RefreshINI' afterwards", kxf::StringViewOf(PPR::ProjectName)));
			}
		}

		KXF_SCOPEDLOG.SetSuccess();
	}
	void XSEInterface::InitGameMessageDispatcher()
	{
		KXF_SCOPEDLOG_FUNC;

		#if xSE_HAS_MESSAGING_INTERFACE
		if (CanUseSEFunctions() && m_Messaging && !m_GameEventListenerRegistered)
		{
			auto senderName = xSE_FOLDER_NAME_A;
			m_GameEventListenerRegistered = m_Messaging->RegisterListener(m_PluginHandle, senderName, [](xSE_MessagingInterface::Message* msg)
			{
				if (msg)
				{
					auto MapEventID = [](uint32_t messageID) -> kxf::EventID
					{
						switch (messageID)
						{
							case xSE_MessagingInterface::kMessage_PostPostLoad:
							{
								return GameEvent::EvtPluginsLoaded;
							}
							case xSE_MessagingInterface::kMessage_InputLoaded:
							{
								return GameEvent::EvtInputLoaded;
							}

							#if xSE_PLATFORM_F4SE || xSE_PLATFORM_F4SEVR
							case xSE_MessagingInterface::kMessage_GameDataReady:
							{
								return GameEvent::EvtDataLoaded;
							}
							#else
							case xSE_MessagingInterface::kMessage_DataLoaded:
							{
								return GameEvent::EvtDataLoaded;
							}
							#endif


							case xSE_MessagingInterface::kMessage_NewGame:
							{
								return GameEvent::EvtNewGame;
							}

							#if xSE_PLATFORM_F4SE || xSE_PLATFORM_F4SEVR
							case xSE_MessagingInterface::kMessage_PreSaveGame:
							{
								return GameEvent::EvtGameSave;
							}
							case xSE_MessagingInterface::kMessage_PostSaveGame:
							{
								return GameEvent::EvtGameSaved;
							}
							#else
							case xSE_MessagingInterface::kMessage_SaveGame:
							{
								return GameEvent::EvtGameSave;
							}
							#endif

							case xSE_MessagingInterface::kMessage_PreLoadGame:
							{
								return GameEvent::EvtGameLoad;
							}
							case xSE_MessagingInterface::kMessage_PostLoadGame:
							{
								return GameEvent::EvtGameLoaded;
							}
							case xSE_MessagingInterface::kMessage_DeleteGame:
							{
								return GameEvent::EvtDeleteSavedGame;
							}
						};
						return kxf::IEvent::EvtNull;
					};
					if (auto eventID = MapEventID(msg->type))
					{
						GameEvent event(msg->data, msg->dataLen);
						XSEInterface::GetInstance().ProcessEvent(event, eventID);
					}
				}
			});

			if (m_GameEventListenerRegistered)
			{
				xSE_LOG("Game event listener registered successfully for sender: '{}'", senderName);
			}
			else
			{
				xSE_LOG_ERROR("Failed to register game event listener for sender: '{}'", senderName);
			}
		}
		#endif

		KXF_SCOPEDLOG.SetSuccess();
	}

	// IEvtHandler
	bool XSEInterface::OnDynamicBind(EventItem& eventItem)
	{
		if (CanUseSEFunctions())
		{
			auto id = eventItem.GetEventID();
			if (id.IsOfEventClass<ConsoleEvent>())
			{
				InitConsoleCommandOverrider();
			}
			else if (id.IsOfEventClass<GameEvent>())
			{
				InitGameMessageDispatcher();
			}
		}

		return EvtHandler::OnDynamicBind(eventItem);
	}
	
	// XSEInterface
	XSEInterface::XSEInterface(DLLApplication& app, const AppConfigLoader& config)
		:m_PluginHandle(kPluginHandle_Invalid)
	{
		KXF_SCOPEDLOG_FUNC;

		LoadConfig(app, config);

		KXF_SCOPEDLOG.SetSuccess();
	}
	XSEInterface::~XSEInterface()
	{
		KXF_SCOPEDLOG_FUNC;
		KXF_SCOPEDLOG.SetSuccess();
	}
	
	// AppModule
	void XSEInterface::OnInit(DLLApplication& app)
	{
		KXF_SCOPEDLOG_FUNC;
		KXF_SCOPEDLOG.SetSuccess();
	}
}
