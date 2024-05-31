#pragma once
#include "stdafx.h"
#include "xSE\IConsoleCommandOverrider.h"
#include "xSE\ScriptExtenderInterfaceIncludes.h"

namespace PPR
{
	class ConsoleCommandOverrider_SKSE64 final: public IConsoleCommandOverrider
	{
		private:
			struct CommandInfo
			{
				ObScriptCommand OriginalCommand;
				kxf::String HelpString;
			};

		private:
			kxf::IEvtHandler& m_EvtHandler;
			std::unordered_map<uint32_t, CommandInfo> m_Commands;

		private:
			ObScriptCommand* FindCommand(const kxf::String& fullName) const;
			bool OnCommand(const ObScriptParam* paramInfo, ScriptData* scriptData, TESObjectREFR* thisObj, TESObjectREFR* containingObj, Script* scriptObj, ScriptLocals* locals, double& result, UInt32& opcodeOffset);
		
		public:
			ConsoleCommandOverrider_SKSE64(kxf::IEvtHandler& evtHandler)
				:m_EvtHandler(evtHandler)
			{
			}

		public:
			bool OverrideCommand(const kxf::String& commandName, const kxf::String& commandHelp) override;
	};
}
