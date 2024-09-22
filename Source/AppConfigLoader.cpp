#include "stdafx.h"
#include "AppConfigLoader.h"
#include "PrivateProfileRedirector.h"

namespace PPR
{
	bool AppConfigLoader::LoadRedirectorOption(kxf::FlagSet<RedirectorOption>& result, RedirectorOption option, const wchar_t* name, RedirectorOption disableIf) const
	{
		constexpr kxf::FlagSet<RedirectorOption> defaultOptions = RedirectorOption::SaveOnWrite|RedirectorOption::ProcessInlineComments;

		bool value = m_General.GetAttributeBool(name, defaultOptions.Contains(option));
		if (disableIf != RedirectorOption::None && result.Contains(disableIf))
		{
			value = false;
		}

		result.Mod(option, value);
		return value;
	}
}
