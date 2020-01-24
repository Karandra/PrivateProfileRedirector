#pragma once
#include "stdafx.h"
#include "Utility\SimpleINI.h"
#include "Utility\KxOptionSet.h"
#include "Utility\KxDynamicString.h"
#include "Utility\EnumClassOperations.h"

namespace PPR
{
	enum class RedirectorOption: uint32_t
	{
		None = 0,
		AllowSEVersionMismatch = 1 << 0,
		WriteProtected = 1 << 1,
		NativeWrite = 1 << 2,
		SaveOnWrite = 1 << 3,
		SaveOnThreadDetach = 1 << 4,
		SaveOnProcessDetach = 1 << 5,
		SaveOnGameSave = 1 << 6,
		ProcessInlineComments = 1 << 7,
	};
	PPR_AllowEnumCastOp(RedirectorOption);
	PPR_AllowEnumBitwiseOp(RedirectorOption);

	using RedirectorOptionSet = KxOptionSet<RedirectorOption, RedirectorOption::SaveOnWrite|RedirectorOption::ProcessInlineComments>;
}

namespace PPR
{
	class RedirectorConfigLoader final
	{
		private:
			CSimpleIniW m_Config;
			RedirectorOptionSet& m_OptionSet;

		private:
			const wchar_t* DoGetOption(const wchar_t* section, const wchar_t* key, const wchar_t* defaultValue = nullptr) const;
			int DoGetOptionInt(const wchar_t* section, const wchar_t* key, int defaultValue = 0) const;

		public:
			RedirectorConfigLoader(RedirectorOptionSet& optionSet, KxDynamicStringRefW filePath);
			
		public:
			const wchar_t* GetString(const wchar_t* name, const wchar_t* defaultValue = nullptr) const
			{
				return DoGetOption(L"General", name, defaultValue);
			}
			int GetInt(const wchar_t* name, int defaultValue = -1) const
			{
				return DoGetOptionInt(L"General", name, defaultValue);
			}
			bool GetBool(const wchar_t* name, bool defaultValue = false) const
			{
				return DoGetOptionInt(L"General", name, defaultValue);
			}

			bool LoadOption(RedirectorOption option, const wchar_t* name, RedirectorOption disableIf = RedirectorOption::None) const;

		public:
			void LogOption(int value, const wchar_t* name);
			void LogOption(bool value, const wchar_t* name)
			{
				LogOption((int)value, name);
			}
			void LogOption(RedirectorOption option, const wchar_t* name)
			{
				LogOption(m_OptionSet.IsEnabled(option), name);
			}
	};
}
