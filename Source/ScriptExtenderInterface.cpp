#include "stdafx.h"
#include "ScriptExtenderInterface.h"
#include "PrivateProfileRedirector.h"

#pragma warning(disable: 4005) // macro redefinition
#pragma warning(disable: 4244) // conversion from 'x' to 'y', possible loss of data
#pragma warning(disable: 4267) // conversion from 'size_t' to 'y', possible loss of data
#include "ScriptExtenderInterfaceIncludes.h"

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
			OverrideRefreshINI();
		}
		return true;
	}

	xSE_ConsoleCommandInfo* SEInterface::FindConsoleCommand(KxDynamicStringRefA fullName) const
	{
		#if xSE_HAS_CONSOLE_COMMAND_INFO
		for (xSE_ConsoleCommandInfo* command = g_firstConsoleCommand; command->opcode < kObScript_NumConsoleCommands + kObScript_ConsoleOpBase; ++command)
		{
			if (command->longName && KxDynamicStringRefA(command->longName) == fullName)
			{
				return command;
			}
		}
		#endif
		return nullptr;
	}
	void SEInterface::OverrideRefreshINI()
	{
		#if xSE_HAS_CONSOLE_COMMAND_INFO
		xSE_LOG("Overriding 'RefreshINI' console command to refresh INIs from disk");

		m_RefreshINICommand = FindConsoleCommand("RefreshINI");
		if (m_RefreshINICommand)
		{
			m_OriginalRefreshINIHandler = m_RefreshINICommand->execute;
			auto OnCall = [](void* paramInfo, void* scriptData, TESObjectREFR* thisObj, void* containingObj, void* scriptObj, void* locals, double* result, void* opcodeOffsetPtr) -> bool
			{
				SEInterface& instance = SEInterface::GetInstance();
				Console_Print("Executing 'RefreshINI'");

				size_t reloadedCount = Redirector::GetInstance().RefreshINI();
				bool ret = instance.m_OriginalRefreshINIHandler(paramInfo, scriptData, thisObj, containingObj, scriptObj, locals, result, opcodeOffsetPtr);

				// Apparently Fallout 4 doesn't support '%zu' format specifier.
				Console_Print("Executing 'RefreshINI' done with result '%d', %u files reloaded.", (int)ret, (unsigned int)reloadedCount);
				return ret;
			};

			xSE_ConsoleCommandInfo newCommand = *m_RefreshINICommand;
			newCommand.helpText = "[Redirector] Reloads INIs content from disk and calls original 'RefreshINI' after it";
			newCommand.execute = OnCall;

			SafeWriteBuf(reinterpret_cast<uintptr_t>(m_RefreshINICommand), &newCommand, sizeof(newCommand));
			xSE_LOG("Command 'RefreshINI' is overridden");
		}
		else
		{
			xSE_LOG("Can't find 'RefreshINI' command to override");
		}
		#endif
	}

	SEInterface::SEInterface()
		:m_PluginHandle(kPluginHandle_Invalid)
	{
	}
	SEInterface::~SEInterface()
	{
	}

	bool SEInterface::CanUseSEFunctions() const
	{
		return m_CanUseSEFunctions;
	}
}
