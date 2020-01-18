#pragma once
#include "stdafx.h"
#include "IRefreshINIOverrider.h"
#include "ScriptExtenderInterfaceIncludes.h"
#include "Utility/KxDynamicString.h"

namespace PPR
{
	class RefreshINIOverrider_F4SE final: public IRefreshINIOverrider
	{
		private:
			ObScriptCommand m_OriginalCommand = {};
			ObScriptCommand m_NewCommand = {};

		private:
			ObScriptCommand* FindCommand(KxDynamicStringRefA fullName) const;

			template<class... Args>
			auto CallOriginal(Args&&... arg) const
			{
				return m_OriginalCommand.execute(std::forward<Args>(arg)...);
			}

		public:
			void Execute() override;
	};
}
