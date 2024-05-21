#include "stdafx.h"
#include "SKSE64.h"
#include "PrivateProfileRedirector.h"
#include "xSE/ScriptExtenderInterface.h"
#include "Qx/EventSystem/EvtHandler.h"
#include "Qx/EventSystem/Events/QxConsoleEvent.h"

namespace PPR
{
	ObScriptCommand* ConsoleCommandOverrider_SKSE64::FindCommand(const kxf::String& fullName) const
	{
		for (ObScriptCommand* command = g_firstConsoleCommand; command->opcode < kObScript_NumConsoleCommands + kObScript_ConsoleOpBase; ++command)
		{
			if (command->longName && command->longName == fullName)
			{
				return command;
			}
		}
		return nullptr;
	}
	bool ConsoleCommandOverrider_SKSE64::OnCommand(const ObScriptParam* paramInfo, ScriptData* scriptData, TESObjectREFR* thisObj, TESObjectREFR* containingObj, Script* scriptObj, ScriptLocals* locals, double& result, UInt32& opcodeOffset)
	{
		if (scriptData)
		{
			if (auto it = m_Commands.find(scriptData->opcode); it != m_Commands.end())
			{
				const ObScriptCommand& originalCommand = it->second.OriginalCommand;

				QxConsoleEvent event;
				event.SetEventID(QxConsoleEvent::EvtCommand);
				event.SetCommandName(originalCommand.longName);
				event.SetCommandAlias(originalCommand.shortName);
				event.SetCommandHelp(originalCommand.helpText);

				if (!m_EvtHandler.ProcessEvent(event) || event.IsSkipped())
				{
					return originalCommand.execute(paramInfo, scriptData, thisObj, containingObj, scriptObj, locals, result, opcodeOffset);
				}
				else
				{
					result = event.GetResult();
					return true;
				}
			}
		}
		return false;
	}

	bool ConsoleCommandOverrider_SKSE64::OverrideCommand(const kxf::String& commandName, const kxf::String& commandHelp)
	{
		if (ObScriptCommand* command = FindCommand(commandName))
		{
			// Save original command and help string
			const CommandInfo& commandInfo = m_Commands.insert_or_assign(command->opcode, CommandInfo{*command, commandHelp}).first->second;

			// Make new command
			ObScriptCommand newCommand = commandInfo.OriginalCommand;
			newCommand.helpText = commandInfo.HelpString.utf8_str();
			newCommand.execute = [](const ObScriptParam* paramInfo, ScriptData* scriptData, TESObjectREFR* thisObj, TESObjectREFR* containingObj, Script* scriptObj, ScriptLocals* locals, double& result, UInt32& opcodeOffset)
			{
				auto& instance = *SEInterface::GetInstance().GetConsoleCommandOverrider<ConsoleCommandOverrider_SKSE64>();
				return instance.OnCommand(paramInfo, scriptData, thisObj, containingObj, scriptObj, locals, result, opcodeOffset);
			};

			SafeWriteBuf(reinterpret_cast<uintptr_t>(command), &newCommand, sizeof(newCommand));
			xSE_LOG("Command '{}' is overridden successfully", command->longName);

			return true;
		}
		
		xSE_LOG_WARNING("Can't find '{}' command to override", commandName);
		return false;
	}
}
