#include "stdafx.h"
#include "ScriptExtenderInterface.h"
#include "PrivateProfileRedirector.h"

#pragma warning(disable: 4005) // macro redefinition
#pragma warning(disable: 4244) // conversion from 'x' to 'y', possible loss of data
#pragma warning(disable: 4267) // conversion from 'size_t' to 'y', possible loss of data
#include "ScriptExtenderInterfaceIncludes.h"

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
	PPR::Redirector::GetInstance().DllMain(nullptr, DLL_PROCESS_ATTACH);

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
				Name = Redirector::GetLibraryName().utf8_view();
				Version = static_cast<uint32_t>(Redirector::GetLibraryVersion().ToInteger());
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
	if (!SEInterface::GetInstance().OnCheckVersion(interfaceVersion, compiledVersion))
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

	return SEInterface::GetInstance().OnQuery(pluginHandle, pluginHandle != kPluginHandle_Invalid ? xSE : nullptr, scaleform, messaging);
}
bool xSE_LOADFUNCTION(const xSE_Interface* xSE)
{
	using namespace PPR;
	xSE_LOG("[{}] On load plugin", _CRT_STRINGIZE(xSE_LOADFUNCTION));

	#if xSE_PLATFORM_SKSE64AE
	if (PluginInfo pluginInfo; !xSE_QUERYFUNCTION(xSE, &pluginInfo))
	{
		return false;
	}
	#endif

	SEInterface& instance = SEInterface::GetInstance();
	if (instance.OnLoad())
	{
		xSE_LOG(L"[{}] {} v{} loaded", xSE_NAME_A, Redirector::GetLibraryName(), Redirector::GetLibraryVersion().ToString());
		return true;
	}
	return false;
}

#if xSE_PLATFORM_SKSE64AE
extern "C" __declspec(dllexport) constinit auto SKSEPlugin_Version = []() constexpr
{
	SKSEPluginVersionData versionData = {};
	versionData.dataVersion = SKSEPluginVersionData::kVersion;
	versionData.compatibleVersions[0] = CURRENT_RELEASE_RUNTIME;
	versionData.versionIndependence = SKSEPluginVersionData::kVersionIndependent_Signatures;
	versionData.versionIndependenceEx = SKSEPluginVersionData::kVersionIndependentEx_NoStructUse;
	versionData.pluginVersion = PPR::MakeFullVersion(0, 6, 0);

	std::ranges::copy("PrivateProfileRedirector", versionData.name);
	std::ranges::copy("Karandra", versionData.author);

	return versionData;
}();
#endif

bool xSE_CAN_USE_SCRIPTEXTENDER() noexcept
{
	return PPR::SEInterface::GetInstance().CanUseSEFunctions();
}

namespace PPR
{
	SEInterface& SEInterface::GetInstance() noexcept
	{
		static SEInterface ms_Instance;
		return ms_Instance;
	}

	bool SEInterface::OnCheckVersion(uint32_t interfaceVersion, uint32_t compiledVersion)
	{
		m_CanUseSEFunctions = interfaceVersion == compiledVersion || GetRedirector().IsOptionEnabled(RedirectorOption::AllowSEVersionMismatch);
		return m_CanUseSEFunctions;
	}
	bool SEInterface::OnQuery(PluginHandle pluginHandle, const xSE_Interface* xSE, xSE_ScaleformInterface* scaleform, xSE_MessagingInterface* messaging)
	{
		m_PluginHandle = pluginHandle;
		m_XSE = xSE;
		m_Scaleform = scaleform;
		m_Messaging = messaging;

		return true;
	}
	bool SEInterface::OnLoad()
	{
		if (CanUseSEFunctions())
		{
			// Events
			Bind(ConsoleEvent::EvtCommand, &SEInterface::OnConsoleCommand, this);
			Bind(GameEvent::EvtGameSave, &SEInterface::OnGameSave, this);
		}
		return true;
	}

	bool SEInterface::DoPrintConsole(const char* string) const
	{
		xSE_LOG("Printed to console: '{}'", string);

		#if !xSE_PLATFORM_NVSE
		Console_Print("%s", string);
		return true;
		#else
		return false;
		#endif
	}
	void SEInterface::InitConsoleCommandOverrider()
	{
		if (!m_ConsoleCommandOverrider)
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
				m_ConsoleCommandOverrider->OverrideCommand("RefreshINI", "[Redirector] Reloads INI files content from disk and calls the original 'RefreshINI' afterwards");
			}
		}
	}
	void SEInterface::InitGameMessageDispatcher()
	{
		#if xSE_HAS_MESSAGING_INTERFACE
		if (m_Messaging && !m_GameEventListenerRegistered && GetRedirector().IsOptionEnabled(RedirectorOption::SaveOnGameSave))
		{
			m_GameEventListenerRegistered = m_Messaging->RegisterListener(m_PluginHandle, "SKSE", [](xSE_MessagingInterface::Message* msg)
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
						SEInterface::GetInstance().ProcessEvent(event, eventID);
					}
				}
			});
		}
		#endif
	}

	void SEInterface::OnConsoleCommand(ConsoleEvent& event)
	{
		auto commandName = event.GetCommandName();
		if (commandName == "RefreshINI")
		{
			PrintConsole("Executing '{}'", commandName);

			const size_t reloadedCount = Redirector::GetInstance().RefreshINI();
			PrintConsole("Executing '{}' done, {} files reloaded.", commandName, reloadedCount);
		}
		else
		{
			PrintConsole("Unknown command '{}'", commandName);
		}

		// Allow original console command to be called after
		event.Skip();
	}
	void SEInterface::OnGameSave(GameEvent& event)
	{
		auto saveFile = event.GetSaveFile();

		xSE_LOG("Saving game: {}", saveFile);
		GetRedirector().SaveChangedFiles(L"On game save");
	}

	// IEvtHandler
	bool SEInterface::OnDynamicBind(EventItem& eventItem)
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
	
	SEInterface::SEInterface() noexcept
		:m_PluginHandle(kPluginHandle_Invalid)
	{
	}

	Redirector& SEInterface::GetRedirector() const
	{
		return Redirector::GetInstance();
	}
}
