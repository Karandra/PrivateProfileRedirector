#pragma once
#include <kxf/EventSystem/IEvtHandler.h>
#include "ConsoleEvent.h"

namespace PPR
{
	class IConsoleCommandOverrider
	{
		public:
			virtual ~IConsoleCommandOverrider() = default;

		public:
			virtual bool OverrideCommand(const kxf::String& commandName, const kxf::String& commandHelp) = 0;
	};
}
