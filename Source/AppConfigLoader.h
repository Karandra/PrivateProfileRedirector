#pragma once
#include "Common.h"
#include <kxf/Serialization/INI.h>

namespace PPR
{
	enum class RedirectorOption: uint32_t
	{
		None = 0,

		WriteProtected = kxf::FlagSetValue<RedirectorOption>(0),
		NativeWrite = kxf::FlagSetValue<RedirectorOption>(1),
		SaveOnWrite = kxf::FlagSetValue<RedirectorOption>(2),
		SaveOnThreadDetach = kxf::FlagSetValue<RedirectorOption>(3),
		SaveOnProcessDetach = kxf::FlagSetValue<RedirectorOption>(4),
		SaveOnGameSave = kxf::FlagSetValue<RedirectorOption>(5),
		ProcessInlineComments = kxf::FlagSetValue<RedirectorOption>(6)
	};
	enum class XSEOption: uint32_t
	{
		None = 0,

		AllowVersionMismatch = kxf::FlagSetValue<XSEOption>(0),
	};
	enum class ENBOption: uint32_t
	{
		None = 0
	};
}

namespace kxf
{
	kxf_FlagSet_Declare(PPR::RedirectorOption);
	kxf_FlagSet_Declare(PPR::XSEOption);
	kxf_FlagSet_Declare(PPR::ENBOption);
}

namespace PPR
{
	class AppConfigLoader final
	{
		private:
			kxf::INIDocument m_Config;
			kxf::INIDocumentSection m_General;
			kxf::INIDocumentSection m_Redirector;
			kxf::INIDocumentSection m_XSEInterface;
			kxf::INIDocumentSection m_ENBInterface;

		private:
			template<class TOption>
			bool LoadBoolOption(const kxf::INIDocumentSection& section, kxf::FlagSet<TOption>& result, TOption option, const kxf::String& name, TOption disableIf, kxf::FlagSet<TOption> defaultOptions = {}) const
			{
				bool value = section.GetAttributeBool(name, defaultOptions.Contains(option));
				if (disableIf != TOption::None && result.Contains(disableIf))
				{
					value = false;
				}

				result.Mod(option, value);
				return value;
			}

		public:
			AppConfigLoader(std::shared_ptr<kxf::IInputStream> stream);
			
		public:
			bool IsNull() const
			{
				return m_Config.IsNull();
			}

			const kxf::INIDocumentSection& GetGeneralSection() const noexcept
			{
				return m_General;
			}
			const kxf::INIDocumentSection& GetRedirectorSection() const noexcept
			{
				return m_Redirector;
			}

			bool LoadRedirectorOption(kxf::FlagSet<RedirectorOption>& result, RedirectorOption option, const kxf::String& name, RedirectorOption disableIf = RedirectorOption::None) const
			{
				constexpr kxf::FlagSet<RedirectorOption> defaultOptions = RedirectorOption::SaveOnWrite|RedirectorOption::ProcessInlineComments;
				return LoadBoolOption(m_Redirector, result, option, name, disableIf, defaultOptions);
			}
			bool LoadXSEOption(kxf::FlagSet<XSEOption>& result, XSEOption option, const kxf::String& name, XSEOption disableIf = XSEOption::None) const
			{
				return LoadBoolOption(m_XSEInterface, result, option, name, disableIf);
			}
	};
}
