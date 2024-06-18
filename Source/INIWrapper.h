#pragma once
#include "stdafx.h"
#include <kxf/Serialization/INI.h>
#include <kxf/Core/IEncodingConverter.h>

namespace PPR
{
	class INIWrapper final
	{
		public:
			enum class Options: uint32_t
			{
				None = 0,
				WithBOM = 1u << 1
			};
			enum class Encoding
			{
				None = -1,
				Auto,

				UTF8,
				UTF16LE,
				UTF16BE,
				UTF32LE,
				UTF32BE
			};

			template<class TChar>
			static auto EncodingFrom(const kxf::String& str, kxf::IEncodingConverter& converter)
			{
				if constexpr(std::is_same_v<TChar, char>)
				{
					return converter.ToMultiByte(kxf::StringViewOf(str));
				}
				else if constexpr(std::is_same_v<TChar, wchar_t>)
				{
					return str.wc_view();
				}
				else
				{
					static_assert(sizeof(TChar*) == 0, "invalid type");
				}
			}

			static kxf::String EncodingTo(const char* str, kxf::IEncodingConverter& converter)
			{
				if (str)
				{
					return converter.ToWideChar(str);
				}
				return {};
			}
			static kxf::String EncodingTo(const wchar_t* str, kxf::IEncodingConverter& converter)
			{
				if (str)
				{
					return str;
				}
				return {};
			}

			template<class TChar, class TFunc>
			requires(std::is_invocable_r_v<kxf::CallbackCommand, TFunc, std::basic_string<TChar>&, const kxf::String&>)
			static std::basic_string<TChar> CreateZSSTRZZ(TFunc&& func, const std::vector<kxf::String>& items, size_t maxSize, bool* truncated, size_t* count)
			{
				kxf::Utility::SetIfNotNull(count, 0);
				kxf::Utility::SetIfNotNull(truncated, false);

				if (maxSize == 0)
				{
					return {};
				}

				size_t addedCount = 0;
				std::basic_string<TChar> buffer;
				buffer.reserve(maxSize <= 8192 ? maxSize : 0);

				for (const kxf::String& item: items)
				{
					kxf::CallbackCommand result = std::invoke(func, buffer, item);
					if (result == kxf::CallbackCommand::Continue || result == kxf::CallbackCommand::Discard)
					{
						if (result == kxf::CallbackCommand::Continue)
						{
							addedCount++;
							buffer.append(1, 0);
						}

						if (buffer.length() >= maxSize)
						{
							break;
						}
					}
					else if (result == kxf::CallbackCommand::Terminate)
					{
						break;
					}
				}
				buffer.append(addedCount != 0 ? 1 : 2, 0);

				kxf::Utility::SetIfNotNull(count, addedCount);
				if (TruncateZSSTRZZ(buffer, maxSize))
				{
					kxf::Utility::SetIfNotNull(truncated, true);
				}
				return buffer;
			}
			
		private:
			template<class TChar>
			static bool TruncateZSSTRZZ(std::basic_string<TChar>& buffer, size_t maxSize)
			{
				if (buffer.length() > maxSize)
				{
					buffer.resize(maxSize);
					if (maxSize >= 2)
					{
						buffer[maxSize - 2] = 0;
					}
					if (maxSize >= 1)
					{
						buffer[maxSize - 1] = 0;
					}

					return true;
				}
				return false;
			}

			template<class TChar>
			static std::basic_string<TChar> ToZSSTRZZ(const std::vector<kxf::String>& items, size_t maxSize, bool* truncated, size_t* count, kxf::IEncodingConverter& converter)
			{
				return CreateZSSTRZZ<TChar>([&](std::basic_string<TChar>& buffer, const kxf::String& item)
				{
					if (!item.empty())
					{
						buffer.append(EncodingFrom<TChar>(item, converter));
						return kxf::CallbackCommand::Continue;
					}
					return kxf::CallbackCommand::Discard;
				}, items, maxSize, truncated, count);
			}

		private:
			kxf::INIDocument m_INI;
			kxf::FlagSet<Options> m_Options;
			Encoding m_Encoding = Encoding::None;

		private:
			std::optional<kxf::String> ExtractValue(kxf::String value) const;

		public:
			INIWrapper() = default;

		public:
			bool IsEmpty() const noexcept
			{
				return m_INI.IsNull();
			}
			Encoding GetEncoding() const noexcept
			{
				return m_Encoding;
			}
			kxf::FlagSet<Options> GetOptions() const noexcept
			{
				return m_Options;
			}

			bool Load(const kxf::FSPath& path, kxf::FlagSet<kxf::INIDocumentOption> options);
			bool Save(const kxf::FSPath& path, Encoding encoding = Encoding::None);

			std::optional<kxf::String> QueryValue(const kxf::String& section, const kxf::String& key) const
			{
				if (auto value = m_INI.IniQueryValue(section, key))
				{
					return ExtractValue(std::move(*value));
				}
				return {};
			}
			kxf::String GetValue(const kxf::String& section, const kxf::String& key, kxf::String defaultValue = {}) const
			{
				auto value = QueryValue(section, key);
				return value ? std::move(*value) : std::move(defaultValue);
			}
			bool SetValue(const kxf::String& section, const kxf::String& key, const kxf::String& value)
			{
				return m_INI.IniSetValue(section, key, value);
			}

			std::vector<kxf::String> GetSectionNames() const;
			std::vector<kxf::String> GetKeyNames(const kxf::String& section) const;
			bool DeleteSection(const kxf::String& section)
			{
				return m_INI.RemoveSection(section);
			}
			bool DeleteKey(const kxf::String& section, const kxf::String& key)
			{
				return m_INI.RemoveValue(section, key);
			}

		public:
			template<class TChar>
			std::basic_string<TChar> GetSectionNamesZSSTRZZ(kxf::IEncodingConverter& converter, size_t maxSize = kxf::String::npos, bool* truncated = nullptr, size_t* count = nullptr) const
			{
				return ToZSSTRZZ<TChar>(GetSectionNames(), maxSize, truncated, count, converter);
			}

			template<class TChar>
			std::basic_string<TChar> GetKeyNamesZSSTRZZ(kxf::IEncodingConverter& converter, const kxf::String& section, size_t maxSize = kxf::String::npos, bool* truncated = nullptr, size_t* count = nullptr) const
			{
				return ToZSSTRZZ<TChar>(GetKeyNames(section), maxSize, truncated, count, converter);
			}
	};
}

namespace kxf
{
	KxFlagSet_Declare(PPR::INIWrapper::Options);
}
