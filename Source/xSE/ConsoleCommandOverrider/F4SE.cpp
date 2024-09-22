#include "stdafx.h"
#include "F4SE.h"
#include "PrivateProfileRedirector.h"
#include "xSE/ScriptExtenderInterface.h"

namespace PPR
{
	ObScriptCommand* ConsoleCommandOverrider_F4SE::FindCommand(const kxf::String& fullName) const
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
	bool ConsoleCommandOverrider_F4SE::OnCommand(void* paramInfo, void* scriptData, TESObjectREFR* thisObj, void* containingObj, void* scriptObj, void* locals, double* result, void* opcodeOffset)
	{
		if (scriptData)
		{
			// It's a struct with opcode as its first member in Skyrim SE
			const uint16_t opcode = *reinterpret_cast<uint16_t*>(scriptData);

			if (auto it = m_Commands.find(opcode); it != m_Commands.end())
			{
				const ObScriptCommand& originalCommand = it->second.OriginalCommand;

				ConsoleEvent event;
				event.SetCommandName(originalCommand.longName);
				event.SetCommandAlias(originalCommand.shortName);
				event.SetCommandHelp(originalCommand.helpText);

				if (!m_EvtHandler.ProcessEvent(event, ConsoleEvent::EvtCommand) || event.IsSkipped())
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

	bool ConsoleCommandOverrider_F4SE::OverrideCommand(const kxf::String& commandName, const kxf::String& commandHelp)
	{
		if (ObScriptCommand* command = FindCommand(commandName))
		{
			// Save original command and help string
			const CommandInfo& commandInfo = m_Commands.insert_or_assign(command->opcode, CommandInfo{*command, commandHelp}).first->second;

			// Make new command
			ObScriptCommand newCommand = commandInfo.OriginalCommand;
			newCommand.helpText = commandInfo.HelpString.utf8_str();
			newCommand.execute = [](void* paramInfo, void* scriptData, TESObjectREFR* thisObj, void* containingObj, void* scriptObj, void* locals, double* result, void* opcodeOffset)
			{
				auto& instance = *XSEInterface::GetInstance().GetConsoleCommandOverrider<ConsoleCommandOverrider_F4SE>();
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
