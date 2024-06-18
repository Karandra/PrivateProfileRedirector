#include "stdafx.h"
#include "RedirectorConfig.h"
#include "PrivateProfileRedirector.h"

namespace PPR
{
	bool RedirectorConfigLoader::LoadOption(RedirectorOption option, const wchar_t* name, RedirectorOption disableIf)
	{
		constexpr kxf::FlagSet<RedirectorOption> defaultOptions = RedirectorOption::SaveOnWrite|RedirectorOption::ProcessInlineComments;

		bool value = m_General.GetAttributeBool(name, defaultOptions.Contains(option));
		if (disableIf != RedirectorOption::None && m_OptionSet.Contains(disableIf))
		{
			value = false;
		}

		m_OptionSet.Mod(option, value);
		return value;
	}
}
