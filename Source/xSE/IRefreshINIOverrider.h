#pragma once

namespace PPR
{
	class IRefreshINIOverrider
	{
		public:
			virtual ~IRefreshINIOverrider() = default;

		public:
			virtual void Execute() = 0;
	};
}
