#pragma once
#include "stdafx.h"
#include "xSE\IConsoleCommandOverrider.h"
#include "xSE\ScriptExtenderInterfaceIncludes.h"
#include "Utility/KxDynamicString.h"

namespace PPR
{
	class ConsoleCommandOverrider_SKSEVR final: public IConsoleCommandOverrider
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
			bool OnCommand(void* paramInfo, void* scriptData, TESObjectREFR* thisObj, void* containingObj, void* scriptObj, void* locals, double* result, void* opcodeOffset);
		
		public:
			ConsoleCommandOverrider_SKSEVR(QxEvtHandler& evtHandler)
				:m_EvtHandler(evtHandler)
			{
			}

		public:
			bool OverrideCommand(KxDynamicStringRefA commandName, KxDynamicStringRefA commandHelp) override;
	};
}
