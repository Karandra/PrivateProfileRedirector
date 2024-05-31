#pragma once
#include "stdafx.h"
#include "xSE\IConsoleCommandOverrider.h"
#include "xSE\ScriptExtenderInterfaceIncludes.h"

namespace PPR
{
	class ConsoleCommandOverrider_F4SE final: public IConsoleCommandOverrider
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
			bool OnCommand(void* paramInfo, void* scriptData, TESObjectREFR* thisObj, void* containingObj, void* scriptObj, void* locals, double* result, void* opcodeOffset);
		
		public:
			ConsoleCommandOverrider_F4SE(kxf::IEvtHandler& evtHandler)
				:m_EvtHandler(evtHandler)
			{
			}

		public:
			bool OverrideCommand(const kxf::String& commandName, const kxf::String& commandHelp) override;
	};
}
