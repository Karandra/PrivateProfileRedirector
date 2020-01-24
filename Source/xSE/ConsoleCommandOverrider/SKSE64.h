#pragma once
#include "stdafx.h"
#include "xSE\IConsoleCommandOverrider.h"
#include "xSE\ScriptExtenderInterfaceIncludes.h"
#include "Utility/KxDynamicString.h"

namespace PPR
{
	class ConsoleCommandOverrider_SKSE64 final: public IConsoleCommandOverrider
	{
		private:
			struct CommandInfo
			{
				ObScriptCommand m_OriginalCommand;
				KxDynamicStringA m_HelpString;
			};

		private:
			QxEvtHandler& m_EvtHandler;
			std::unordered_map<uint32_t, CommandInfo> m_Commands;

		private:
			ObScriptCommand* FindCommand(KxDynamicStringRefA fullName) const;
			bool OnCommand(const ObScriptParam* paramInfo, ScriptData* scriptData, TESObjectREFR* thisObj, TESObjectREFR* containingObj, Script* scriptObj, ScriptLocals* locals, double& result, UInt32& opcodeOffset);
		
		public:
			ConsoleCommandOverrider_SKSE64(QxEvtHandler& evtHandler)
				:m_EvtHandler(evtHandler)
			{
			}

		public:
			bool OverrideCommand(KxDynamicStringRefA commandName, KxDynamicStringRefA commandHelp) override;
	};
}
