#include "stdafx.h"
#include "RedirectorConfig.h"
#include "PrivateProfileRedirector.h"

namespace PPR
{
	const wchar_t* RedirectorConfigLoader::DoGetOption(const wchar_t* section, const wchar_t* key, const wchar_t* defaultValue) const
	{
		return m_Config.GetValue(section, key, defaultValue);
	}
	int RedirectorConfigLoader::DoGetOptionInt(const wchar_t* section, const wchar_t* key, int defaultValue) const
	{
		const wchar_t* value = DoGetOption(section, key, nullptr);
		if (value)
		{
			int valueInt = defaultValue;
			swscanf(value, L"%d", &valueInt);
			return valueInt;
		}
		return defaultValue;
	}

	RedirectorConfigLoader::RedirectorConfigLoader(RedirectorOptionSet& optionSet, KxDynamicStringRefW filePath)
		:m_OptionSet(optionSet), m_Config(false, false, false)
	{
		m_Config.LoadFile(filePath.data());
	}

	bool RedirectorConfigLoader::LoadOption(RedirectorOption option, const wchar_t* name, RedirectorOption disableIf) const
	{
		bool enabledByDefault = option & RedirectorOptionSet::DefaultOptions;
		bool value = GetBool(name, enabledByDefault);
		if (disableIf != RedirectorOption::None && m_OptionSet.IsEnabled(disableIf))
		{
			value = false;
		}

		m_OptionSet.Enable(option, value);
		return value;
	}
	void RedirectorConfigLoader::LogOption(int value, const wchar_t* name)
	{
		Redirector::GetInstance().Log(L"General/%s -> '%d'", name, value);
	}
}
