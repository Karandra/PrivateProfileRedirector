#include "stdafx.h"
#include "ScriptExtenderInterface.h"
#include "PrivateProfileRedirector.h"

#pragma warning(disable: 4005) // macro redefinition
#pragma warning(disable: 4244) // conversion from 'x' to 'y', possible loss of data
#pragma warning(disable: 4267) // ~
#include "ScriptExtenderInterfaceIncludes.h"

//////////////////////////////////////////////////////////////////////////
bool xSE_QUERYFUNCTION(const xSE_Interface* xSE, PluginInfo* info)
{
	xSE_LOG("[" _CRT_STRINGIZE(xSE_QUERYFUNCTION) "] On query plugin info");

	// Set info
	info->infoVersion = PluginInfo::kInfoVersion;
	info->name = PrivateProfileRedirector::GetLibraryNameA();
	info->version = PrivateProfileRedirector::GetLibraryVersionInt();

	// Save handle
	PluginHandle pluginHandle = xSE->GetPluginHandle();

	// Check SE version
	if (xSE_INTERFACE_VERSION(xSE) != xSE_PACKED_VERSION)
	{
		xSE_LOG("This plugin is not compatible with this version of " xSE_NAME_A);
		xSE_LOG("Script extender interface version '%d', expected '%d'", (int)xSE_INTERFACE_VERSION(xSE), (int)xSE_PACKED_VERSION);
		xSE_LOG("Script extender functions will be disabled");

		pluginHandle = kPluginHandle_Invalid;
	}

	// Get the scale form interface and query its version.
	xSE_ScaleformInterface* scaleform = NULL;
	#if xSE_HAS_SCALEFORM_INTERFACE
	if (pluginHandle != kPluginHandle_Invalid)
	{
		// Don't return, I don't use scaleform anyway.
		scaleform = static_cast<xSE_ScaleformInterface*>(xSE->QueryInterface(kInterface_Scaleform));
		if (!scaleform)
		{
			xSE_LOG("Couldn't get scaleform interface");
			//return false;
		}
		if (scaleform->interfaceVersion < xSE_ScaleformInterface::kInterfaceVersion)
		{
			xSE_LOG("Scaleform interface too old (%d, expected %d)", (int)scaleform->interfaceVersion, (int)xSE_ScaleformInterface::kInterfaceVersion);
			//return false;
		}
	}
	#endif
	return RedirectorSEInterface::GetInstance().OnQuery(pluginHandle, pluginHandle != kPluginHandle_Invalid ? xSE : NULL, scaleform);
}
bool xSE_LOADFUNCTION(const xSE_Interface* xSE)
{
	xSE_LOG("[" _CRT_STRINGIZE(xSE_LOADFUNCTION) "] On load plugin");

	RedirectorSEInterface& instance = RedirectorSEInterface::GetInstance();
	if (instance.OnLoad())
	{
		PrivateProfileRedirector::GetInstance().Log(L"[" xSE_NAME_A L"] %s %s loaded", PrivateProfileRedirector::GetLibraryNameW(), PrivateProfileRedirector::GetLibraryVersionW());
		#if xSE_HAS_SE_LOG
		if (RedirectorSEInterface::GetInstance().CanUseSEFunctions())
		{
			_MESSAGE("[" xSE_NAME_A "] %s %s loaded", PrivateProfileRedirector::GetLibraryNameA(), PrivateProfileRedirector::GetLibraryVersionA());
		}
		#endif
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
RedirectorSEInterface& RedirectorSEInterface::GetInstance()
{
	static RedirectorSEInterface ms_Instance;
	return ms_Instance;
}

bool RedirectorSEInterface::RegisterScaleform(GFxMovieView* view, GFxValue* root)
{
	return true;
}

bool RedirectorSEInterface::OnQuery(PluginHandle pluginHandle, const xSE_Interface* xSE, xSE_ScaleformInterface* scaleforem)
{
	m_PluginHandle = pluginHandle;
	m_XSE = xSE;
	m_Scaleform = scaleforem;

	return true;
}
bool RedirectorSEInterface::OnLoad()
{
	if (CanUseSEFunctions())
	{
		OverrideRefreshINI();
	}
	return true;
}

xSE_ConsoleCommandInfo* RedirectorSEInterface::FindConsoleCommand(const std::string_view& fullName) const
{
	#if xSE_HAS_CONSOLE_COMMAND_INFO
	for (xSE_ConsoleCommandInfo* command = g_firstConsoleCommand; command->opcode < kObScript_NumConsoleCommands + kObScript_ConsoleOpBase; ++command)
	{
		if (command->longName && std::string_view(command->longName) == fullName)
		{
			return command;
		}
	}
	#endif
	return NULL;
}
void RedirectorSEInterface::OverrideRefreshINI()
{
	#if xSE_HAS_CONSOLE_COMMAND_INFO
	xSE_LOG("Overriding 'RefreshINI' console command to refresh INIs from disk");

	m_RefreshINICommand = FindConsoleCommand("RefreshINI");
	if (m_RefreshINICommand)
	{
		m_OriginalRefreshINIHandler = m_RefreshINICommand->execute;
		auto OnCall = [](void* paramInfo, void* scriptData, TESObjectREFR* thisObj, void* containingObj, void* scriptObj, void* locals, double* result, void* opcodeOffsetPtr) -> bool
		{
			RedirectorSEInterface& instance = RedirectorSEInterface::GetInstance();
			Console_Print("Executing 'RefreshINI'");

			size_t reloadedCount = PrivateProfileRedirector::GetInstance().RefreshINI();
			bool ret = instance.m_OriginalRefreshINIHandler(paramInfo, scriptData, thisObj, containingObj, scriptObj, locals, result, opcodeOffsetPtr);

			// Apparently Fallout 4 doesn't support '%zu' format specifier.
			Console_Print("Executing 'RefreshINI' done with result '%d', %u files reloaded.", (int)ret, (unsigned int)reloadedCount);
			return ret;
		};

		xSE_ConsoleCommandInfo newCommand = *m_RefreshINICommand;
		newCommand.helpText = "[PrivateProfileRedirector] Reloads INIs content from disk and calls original 'RefreshINI' after it";
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

RedirectorSEInterface::RedirectorSEInterface()
	:m_PluginHandle(kPluginHandle_Invalid)
{
}
RedirectorSEInterface::~RedirectorSEInterface()
{
}

bool RedirectorSEInterface::CanUseSEFunctions() const
{
	return m_PluginHandle != kPluginHandle_Invalid;
}
