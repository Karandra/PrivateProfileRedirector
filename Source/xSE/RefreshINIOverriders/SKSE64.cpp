#include "stdafx.h"
#include "SKSE64.h"
#include "xSE\ScriptExtenderInterface.h"
#include "PrivateProfileRedirector.h"

namespace PPR
{
	ObScriptCommand* RefreshINIOverrider_SKSE64::FindCommand(KxDynamicStringRefA fullName) const
	{
		for (ObScriptCommand* command = g_firstConsoleCommand; command->opcode < kObScript_NumConsoleCommands + kObScript_ConsoleOpBase; ++command)
		{
			if (command->longName && KxDynamicStringRefA(command->longName) == fullName)
			{
				return command;
			}
		}
		return nullptr;
	}

	void RefreshINIOverrider_SKSE64::Execute()
	{
		xSE_LOG("Overriding 'RefreshINI' console command to refresh INIs from disk");

		if (ObScriptCommand* command = FindCommand("RefreshINI"))
		{
			// Save original command
			m_OriginalCommand = *command;

			// Make new command
			m_NewCommand = *command;
			m_NewCommand.helpText = "[Redirector] Reloads INIs content from disk and calls original 'RefreshINI' after it";
			m_NewCommand.execute = [](const ObScriptParam* paramInfo, ScriptData* scriptData, TESObjectREFR* thisObj, TESObjectREFR* containingObj, Script* scriptObj, ScriptLocals* locals, double& result, UInt32& opcodeOffset)
			{
				RefreshINIOverrider_SKSE64& instance = *SEInterface::GetInstance().GetRefreshINIOverrider<RefreshINIOverrider_SKSE64>();
				Console_Print("Executing 'RefreshINI'");

				const size_t reloadedCount = Redirector::GetInstance().RefreshINI();
				const bool originalResult = instance.CallOriginal(paramInfo, scriptData, thisObj, containingObj, scriptObj, locals, result, opcodeOffset);

				Console_Print("Executing 'RefreshINI' done with result '%d', %u files reloaded.", (int)originalResult, (unsigned int)reloadedCount);
				return originalResult;
			};

			SafeWriteBuf(reinterpret_cast<uintptr_t>(command), &m_NewCommand, sizeof(m_NewCommand));
			xSE_LOG("Command 'RefreshINI' is overridden");
		}
		else
		{
			xSE_LOG("Can't find 'RefreshINI' command to override");
		}
	}
}
