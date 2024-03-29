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
bool xSE_QUERYFUNCTION(const xSE_Interface* xSE, PluginInfo* pluginInfo)
{
	using namespace PPR;
	xSE_LOG("[" _CRT_STRINGIZE(xSE_QUERYFUNCTION) "] On query plugin info");

	// Set info
	if (pluginInfo)
	{
		pluginInfo->infoVersion = PluginInfo::kInfoVersion;
		pluginInfo->name = Redirector::GetLibraryNameA();
		pluginInfo->version = Redirector::GetLibraryVersionInt();
	}

	// Save handle
	PluginHandle pluginHandle = xSE->GetPluginHandle();

	// Check SE version
	const auto interfaceVersion = xSE_INTERFACE_VERSION(xSE);
	constexpr auto compiledVersion = xSE_PACKED_VERSION;
	if (!SEInterface::GetInstance().OnCheckVersion(interfaceVersion, compiledVersion))
	{
		xSE_LOG("This plugin might be incompatible with this version of " xSE_NAME_A);
		xSE_LOG("Script Extender interface version '%u', expected '%u'", static_cast<uint32_t>(interfaceVersion), static_cast<uint32_t>(compiledVersion));
		xSE_LOG("Script Extender functions will be disabled");

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
			xSE_LOG("Couldn't get scaleform interface");
		}
		if (scaleform && scaleform->interfaceVersion < xSE_ScaleformInterface::kInterfaceVersion)
		{
			xSE_LOG("Scaleform interface too old (%d, expected %d)", (int)scaleform->interfaceVersion, (int)xSE_ScaleformInterface::kInterfaceVersion);
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
			xSE_LOG("Couldn't get messaging interface");
		}
		if (messaging && messaging->interfaceVersion < xSE_MessagingInterface::kInterfaceVersion)
		{
			xSE_LOG("Messaging interface too old (%d, expected %d)", (int)messaging->interfaceVersion, (int)xSE_MessagingInterface::kInterfaceVersion);
			messaging = nullptr;
		}
	}
	#endif

	return SEInterface::GetInstance().OnQuery(pluginHandle, pluginHandle != kPluginHandle_Invalid ? xSE : nullptr, scaleform, messaging);
}
bool xSE_LOADFUNCTION(const xSE_Interface* xSE)
{
	using namespace PPR;
	xSE_LOG("[" _CRT_STRINGIZE(xSE_LOADFUNCTION) "] On load plugin");

	#if xSE_PLATFORM_SKSE64AE
	if (!xSE_QUERYFUNCTION(xSE, nullptr))
	{
		return false;
	}
	#endif

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

#if xSE_PLATFORM_SKSE64AE
extern "C" __declspec(dllexport) constinit auto SKSEPlugin_Version = []() constexpr
{
	SKSEPluginVersionData versionData = {};
	versionData.dataVersion = SKSEPluginVersionData::kVersion;
	versionData.compatibleVersions[0] = CURRENT_RELEASE_RUNTIME;
	versionData.versionIndependence = SKSEPluginVersionData::kVersionIndependent_Signatures;
	versionData.versionIndependenceEx = SKSEPluginVersionData::kVersionIndependentEx_NoStructUse;
	versionData.pluginVersion = PPR::MakeFullVersion(0, 5, 3);

	std::ranges::copy("PrivateProfileRedirector", versionData.name);
	std::ranges::copy("Karandra", versionData.author);

	return versionData;
}();
#endif

namespace PPR
{
	SEInterface& SEInterface::GetInstance()
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
			InitConsoleCommandOverrider();
			InitGameMessageDispatcher();

			// Events
			Bind(QxConsoleEvent::EvtCommand, &SEInterface::OnConsoleCommand, this);
			Bind(QxGameEvent::EvtGameSave, &SEInterface::OnGameSave, this);
		}
		return true;
	}

	bool SEInterface::DoPrintConsole(const char* string) const
	{
		GetRedirector().Log("Printed to console: %s", string);

		#if !xSE_PLATFORM_NVSE
		Console_Print("%s", string);
		return true;
		#else
		return false;
		#endif
	}
	void SEInterface::InitConsoleCommandOverrider()
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
			m_ConsoleCommandOverrider->OverrideCommand("RefreshINI", "[Redirector] Reloads INIs content from disk and calls original 'RefreshINI' after it");
		}
	}
	void SEInterface::InitGameMessageDispatcher()
	{
		#if xSE_HAS_MESSAGING_INTERFACE
		if (m_Messaging)
		{
			if (GetRedirector().IsOptionEnabled(RedirectorOption::SaveOnGameSave))
			{
				m_Messaging->RegisterListener(m_PluginHandle, "SKSE", [](xSE_MessagingInterface::Message* msg)
				{
					if (msg)
					{
						auto MapEventID = [](uint32_t messageID) -> QxEventID
						{
							switch (messageID)
							{
								case xSE_MessagingInterface::kMessage_PostPostLoad:
								{
									return QxGameEvent::EvtPluginsLoaded;
								}
								case xSE_MessagingInterface::kMessage_InputLoaded:
								{
									return QxGameEvent::EvtInputLoaded;
								}

								#if xSE_PLATFORM_F4SE || xSE_PLATFORM_F4SEVR
								case xSE_MessagingInterface::kMessage_GameDataReady:
								{
									return QxGameEvent::EvtDataLoaded;
								}
								#else
								case xSE_MessagingInterface::kMessage_DataLoaded:
								{
									return QxGameEvent::EvtDataLoaded;
								}
								#endif


								case xSE_MessagingInterface::kMessage_NewGame:
								{
									return QxGameEvent::EvtNewGame;
								}

								#if xSE_PLATFORM_F4SE || xSE_PLATFORM_F4SEVR
								case xSE_MessagingInterface::kMessage_PreSaveGame:
								{
									return QxGameEvent::EvtGameSave;
								}
								case xSE_MessagingInterface::kMessage_PostSaveGame:
								{
									return QxGameEvent::EvtGameSaved;
								}
								#else
								case xSE_MessagingInterface::kMessage_SaveGame:
								{
									return QxGameEvent::EvtGameSave;
								}
								#endif

								case xSE_MessagingInterface::kMessage_PreLoadGame:
								{
									return QxGameEvent::EvtGameLoad;
								}
								case xSE_MessagingInterface::kMessage_PostLoadGame:
								{
									return QxGameEvent::EvtGameLoaded;
								}
								case xSE_MessagingInterface::kMessage_DeleteGame:
								{
									return QxGameEvent::EvtDeleteSavedGame;
								}
							};
							return QxEvent::EvtNull;
						};

						QxEventID eventID = MapEventID(msg->type);
						if (eventID != QxEvent::EvtNull)
						{
							SEInterface::GetInstance().ProcessEventEx<QxGameEvent>(eventID, msg->data, msg->dataLen).Do();
						}
					}
				});
			}
		}
		#endif
	}

	void SEInterface::OnConsoleCommand(QxConsoleEvent& event)
	{
		auto commandName = event.GetCommandName();
		if (commandName == "RefreshINI")
		{
			PrintConsole("Executing '%s'", commandName.c_str());

			const size_t reloadedCount = Redirector::GetInstance().RefreshINI();
			PrintConsole("Executing '%s' done, %u files reloaded.", commandName.c_str(), static_cast<unsigned int>(reloadedCount));
		}
		else
		{
			PrintConsole("Unknown command '%s'", commandName.c_str());
		}

		// Allow original console command to be called after
		event.Skip();
	}
	void SEInterface::OnGameSave(QxGameEvent& event)
	{
		auto saveFile = event.GetSaveFile();

		GetRedirector().Log("Saving game: %s", saveFile.data());
		GetRedirector().SaveChnagedFiles(L"On game save");
	}

	SEInterface::SEInterface()
		:m_PluginHandle(kPluginHandle_Invalid)
	{
	}

	Redirector& SEInterface::GetRedirector() const
	{
		return Redirector::GetInstance();
	}
}
