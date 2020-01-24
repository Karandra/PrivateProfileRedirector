#pragma once
#include "Utility/KxDynamicString.h"
class QxEvtHandler;

namespace PPR
{
	class IConsoleCommandOverrider
	{
		public:
			virtual ~IConsoleCommandOverrider() = default;

		public:
			virtual bool OverrideCommand(KxDynamicStringRefA commandName, KxDynamicStringRefA commandHelp) = 0;
	};
}
