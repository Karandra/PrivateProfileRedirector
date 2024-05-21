#pragma once
class QxEvtHandler;

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
