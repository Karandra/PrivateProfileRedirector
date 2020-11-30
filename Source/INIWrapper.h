#pragma once
#include "stdafx.h"
#include "Utility/SimpleINI.h"
#include "Utility/KxDynamicString.h"
#include "Utility/EnumClassOperations.h"

namespace PPR
{
	class INIWrapper final
	{
		public:
			using TStringRefVector = std::vector<KxDynamicStringRefW>;

			enum class Options
			{
				None = 0,
				RemoveInlineComments = 1 << 0,
				WithBOM = 1 << 1,
			};
			enum class Encoding
			{
				Auto = 0,
				UTF8,
				UTF16LE,
			};

		private:
			CSimpleIniW m_INI;
			Options m_Options = Options::None;

		private:
			bool LoadUTF8(FILE* stream, size_t fileSize);
			bool LoadUTF16LE(FILE* stream, size_t fileSize);

			KxDynamicStringRefW ExtractValue(KxDynamicStringRefW value) const;

		public:
			INIWrapper()
				:m_INI(false, false, true)
			{
				m_INI.SetSpaces(false);
			}

		public:
			bool IsEmpty() const
			{
				return m_INI.IsEmpty();
			}
			bool Load(KxDynamicStringRefW path, Options options = Options::None, Encoding encoding = Encoding::Auto);
			bool Save(KxDynamicStringRefW path, Options options = Options::None, Encoding encoding = Encoding::Auto);

			std::optional<KxDynamicStringRefW> QueryValue(KxDynamicStringRefW section, KxDynamicStringRefW key) const;
			std::optional<KxDynamicStringRefW> QueryValue(KxDynamicStringRefW section, KxDynamicStringRefW key, const wchar_t* defaultValue) const;
			std::optional<KxDynamicStringRefW> QueryValue(KxDynamicStringRefW section, KxDynamicStringRefW key, KxDynamicStringRefW defaultValue) const
			{
				auto value = QueryValue(section, key);
				return value ? value : defaultValue;
			}

			KxDynamicStringRefW GetValue(KxDynamicStringRefW section, KxDynamicStringRefW key, KxDynamicStringRefW defaultValue = {}) const
			{
				auto value = QueryValue(section, key);
				return value ? *value : defaultValue;
			}
			bool SetValue(KxDynamicStringRefW section, KxDynamicStringRefW key, KxDynamicStringRefW value);

			size_t GetSectionNames(TStringRefVector& sectionNames) const;
			KxDynamicStringW GetSectionNamesZSSTRZZ(size_t maxSize = KxDynamicStringW::npos) const;
			TStringRefVector GetSectionNames() const
			{
				std::vector<KxDynamicStringRefW> sectionNames;
				GetSectionNames(sectionNames);
				return sectionNames;
			}

			size_t GetKeyNames(KxDynamicStringRefW section, TStringRefVector& keyNames) const;
			KxDynamicStringW GetKeyNamesZSSTRZZ(KxDynamicStringRefW section, size_t maxSize = KxDynamicStringW::npos) const;
			TStringRefVector GetKeyNames(KxDynamicStringRefW section) const
			{
				std::vector<KxDynamicStringRefW> keyNames;
				GetKeyNames(section, keyNames);
				return keyNames;
			}

			bool DeleteSection(KxDynamicStringRefW section);
			bool DeleteKey(KxDynamicStringRefW section, KxDynamicStringRefW key);
	};
}

namespace PPR
{
	PPR_AllowEnumCastOp(INIWrapper::Options);
	PPR_AllowEnumBitwiseOp(INIWrapper::Options);

	PPR_AllowEnumCastOp(INIWrapper::Encoding);
}
