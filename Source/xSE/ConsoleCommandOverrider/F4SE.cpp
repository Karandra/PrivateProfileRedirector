#include "stdafx.h"
#include "F4SE.h"
#include "PrivateProfileRedirector.h"
#include "xSE/ScriptExtenderInterface.h"
#include "Qx/EventSystem/EvtHandler.h"
#include "Qx/EventSystem/Events/QxConsoleEvent.h"

namespace PPR
{
	ObScriptCommand* ConsoleCommandOverrider_F4SE::FindCommand(KxDynamicStringRefA fullName) const
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
	bool ConsoleCommandOverrider_F4SE::OnCommand(void* paramInfo, void* scriptData, TESObjectREFR* thisObj, void* containingObj, void* scriptObj, void* locals, double* result, void* opcodeOffset)
	{
		if (scriptData)
		{
			// It's a struct with opcode as its first member in Skyrim SE
			const uint16_t opcode = *reinterpret_cast<uint16_t*>(scriptData);

			if (auto it = m_Commands.find(opcode); it != m_Commands.end())
			{
				const ObScriptCommand& originalCommand = it->second.m_OriginalCommand;

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
					if (result)
					{
						*result = event.GetResult();
					}
					return true;
				}
			}
		}
		return false;
	}

	bool ConsoleCommandOverrider_F4SE::OverrideCommand(KxDynamicStringRefA commandName, KxDynamicStringRefA commandHelp)
	{
		if (ObScriptCommand* command = FindCommand(commandName))
		{
			// Save original command and help string
			const CommandInfo& commandInfo = m_Commands.insert_or_assign(command->opcode, CommandInfo{*command, commandHelp}).first->second;

			// Make new command
			ObScriptCommand newCommand = commandInfo.m_OriginalCommand;
			newCommand.helpText = commandInfo.m_HelpString.c_str();
			newCommand.execute = [](void* paramInfo, void* scriptData, TESObjectREFR* thisObj, void* containingObj, void* scriptObj, void* locals, double* result, void* opcodeOffset)
			{
				auto& instance = *SEInterface::GetInstance().GetConsoleCommandOverrider<ConsoleCommandOverrider_F4SE>();
				return instance.OnCommand(paramInfo, scriptData, thisObj, containingObj, scriptObj, locals, result, opcodeOffset);
			};

			SafeWriteBuf(reinterpret_cast<uintptr_t>(command), &newCommand, sizeof(newCommand));
			xSE_LOG("Command '%s' is overridden successfully", command->longName);

			return true;
		}

		xSE_LOG("Can't find '%s' command to override", commandName.data());
		return false;
	}
}
