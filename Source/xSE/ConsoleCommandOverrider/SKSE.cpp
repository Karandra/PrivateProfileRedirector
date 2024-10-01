#include "stdafx.h"
#include "SKSE.h"
#include "xSE/ScriptExtenderInterface.h"
#include "Redirector/RedirectorInterface.h"

namespace
{
	using ObScriptCommand = ::CommandInfo;

	template<class T>
	class AddressRange final
	{
		private:
			T* m_Begin = nullptr;
			T* m_End = nullptr;

		public:
			constexpr AddressRange(size_t begin, size_t end) noexcept
				:m_Begin(reinterpret_cast<T*>(begin)), m_End(reinterpret_cast<T*>(end))
			{
			}

		public:
			constexpr T* begin() const noexcept
			{
				return m_Begin;
			}
			constexpr T* end() const noexcept
			{
				return m_End;
			}
			constexpr size_t size() const noexcept
			{
				return m_End - m_Begin;
			}
	};

	// From 'skse\skse\Hooks_ObScript.cpp'
	const AddressRange<ObScriptCommand> g_ScriptCommandsRange(0x0124E880, 0x01255B30);
	const AddressRange<ObScriptCommand> g_ConsoleCommandsRange(0x0124B748, 0x0124E858);
}

namespace PPR
{
	ObScriptCommand* ConsoleCommandOverrider_SKSE::FindCommand(const kxf::String& fullName) const
	{
		for (ObScriptCommand& command: g_ConsoleCommandsRange)
		{
			if (command.longName && command.longName == fullName)
			{
				return &command;
			}
		}
		return nullptr;
	}
	bool ConsoleCommandOverrider_SKSE::OnCommand(ParamInfo* paramInfo, void* scriptData, TESObjectREFR* thisObj, TESObjectREFR* containingObj, Script* scriptObj, ScriptLocals* locals, double* result, UInt32* opcodeOffset)
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

	bool ConsoleCommandOverrider_SKSE::OverrideCommand(const kxf::String& commandName, const kxf::String& commandHelp)
	{
		KX_SCOPEDLOG_ARGS(commandName, commandHelp);

		if (ObScriptCommand* command = FindCommand(commandName))
		{
			// Save original command and help string
			const CommandInfo& commandInfo = m_Commands.insert_or_assign(command->opcode, CommandInfo{*command, commandHelp}).first->second;

			// Make new command
			ObScriptCommand newCommand = commandInfo.OriginalCommand;
			newCommand.helpText = commandInfo.HelpString.utf8_str();
			newCommand.execute = [](ParamInfo* paramInfo, void* scriptData, TESObjectREFR* thisObj, TESObjectREFR* containingObj, Script* scriptObj, ScriptLocals* locals, double* result, UInt32* opcodeOffset)
			{
				auto& instance = *XSEInterface::GetInstance().GetConsoleCommandOverrider<ConsoleCommandOverrider_SKSE>();
				return instance.OnCommand(paramInfo, scriptData, thisObj, containingObj, scriptObj, locals, result, opcodeOffset);
			};

			SafeWriteBuf(reinterpret_cast<uintptr_t>(command), &newCommand, sizeof(newCommand));
			xSE_LOG("Command '{}' is overridden successfully", command->longName);

			KX_SCOPEDLOG.SetSuccess();
			return true;
		}

		xSE_LOG_WARNING("Can't find '{}' command to override", commandName);
		return false;
	}
}
