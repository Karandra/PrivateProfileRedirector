#pragma once
#include "Common.h"
#include <kxf/Serialization/INI.h>

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
		ProcessInlineComments = 1 << 7
	};
}

namespace kxf
{
	KxFlagSet_Declare(PPR::RedirectorOption);
}

namespace PPR
{
	class RedirectorConfigLoader final
	{
		private:
			kxf::INIDocument m_Config;
			kxf::INIDocumentSection m_General;
			kxf::FlagSet<RedirectorOption> m_OptionSet;

		public:
			RedirectorConfigLoader(std::unique_ptr<kxf::IInputStream> stream)
			{
				if (stream && m_Config.Load(*stream))
				{
					m_General = m_Config.QueryElement("General");
				}
			}
			
		public:
			bool IsNull() const
			{
				return m_Config.IsNull();
			}
			kxf::FlagSet<RedirectorOption> GetOptions() const noexcept
			{
				return m_OptionSet;
			}

			const kxf::INIDocumentSection& GetGeneral() const noexcept
			{
				return m_General;
			}
			bool LoadOption(RedirectorOption option, const wchar_t* name, RedirectorOption disableIf = RedirectorOption::None);
	};
}
