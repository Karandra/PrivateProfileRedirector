#include "stdafx.h"
#include "INIWrapper.h"
#include "Utility/KxCallAtScopeExit.h"
#include "Utility/String.h"

namespace
{
	constexpr uint8_t BOM_UTF8[] = {0xEF, 0xBB, 0xBF};
	constexpr uint8_t BOM_UTF16_LE[] = {0xFF, 0xFE};
	constexpr uint8_t BOM_UTF16_BE[] = {0xFE, 0xFF};

	template<class T, size_t size>
	bool SkipBOM(FILE* stream, const T(&bom)[size], size_t* bomLength = nullptr, bool alwaysReset = false)
	{
		if (bomLength)
		{
			*bomLength = size;
		}

		bool hasBOM = false;
		uint8_t buffer[size] = {0};
		if (fread(buffer, 1, size, stream) == size)
		{
			hasBOM = memcmp(buffer, bom, size) == 0;
		}

		if (!hasBOM || alwaysReset)
		{
			_fseeki64(stream, -(int64_t)size, SEEK_CUR);
		}
		return hasBOM;
	}

	template<class T, size_t size>
	bool TestBOM(FILE* stream, const T(&bom)[size])
	{
		return SkipBOM(stream, bom, nullptr, true);
	}

	template<size_t count>
	using ArgsBuffer = std::array<KxDynamicStringW, count>;

	template<size_t arg, size_t argsCount>
	void ProcessArgument(ArgsBuffer<argsCount>& argsBuffer, KxDynamicStringRefW& value)
	{
		using namespace PPR::Utility;
		static_assert(arg < argsCount, "invalid argument index");

		auto TrimSpaceCharsLR = [](KxDynamicStringRefW& value)
		{
			const size_t oldLength = value.length();
			value = String::TrimSpaceCharsLR(value);
			return oldLength != value.length();
		};
		if (TrimSpaceCharsLR(value))
		{
			// Copy string to the buffer from string_view to make it null terminated if we trimmed it
			argsBuffer[(size_t)arg] = value;

			// Update string_view to refer to the new null-terminated buffer
			value = argsBuffer[(size_t)arg];
		}
	}

	size_t FindCommentStart(KxDynamicStringRefW value)
	{
		if (!value.empty())
		{
			for (size_t i = value.size() - 1; i != 0; i--)
			{
				const wchar_t c = value[i];
				if (c == L';' || c == L'#')
				{
					return i;
				}
			}
		}
		return KxDynamicStringRefW::npos;
	};
	KxDynamicStringRefW TrimAll(KxDynamicStringRefW value)
	{
		using namespace PPR::Utility;

		value = String::TrimSpaceCharsLR(value);
		value = String::TrimQuoteCharsLR(value);
		return value;
	};
	KxDynamicStringRefW ExtractValueAndComment(KxDynamicStringRefW source, bool removeInlineComments, KxDynamicStringRefW* comment = nullptr)
	{
		if (!source.empty())
		{
			const size_t anchor = removeInlineComments ? FindCommentStart(source) : KxDynamicStringRefW::npos;
			if (anchor != KxDynamicStringRefW::npos)
			{
				KxDynamicStringRefW trimmed = source.substr(0, anchor);
				if (comment && source.length() > anchor)
				{
					*comment = source.substr(anchor + 1);
				}
				return TrimAll(trimmed);
			}
			return TrimAll(source);
		}
		return {};
	}

	KxDynamicStringW ToZSSTRZZ(const CSimpleIniW::TNamesDepend& valuesList, size_t maxSize, bool removeInlineComments)
	{
		KxDynamicStringW result;
		if (!valuesList.empty())
		{
			for (const auto& value: valuesList)
			{
				result.append(ExtractValueAndComment(value.pItem, removeInlineComments));
				result.append(1, L'\0');

				if (result.length() >= maxSize)
				{
					break;
				}
			}
			result.append(1, L'\0');
		}
		else
		{
			result.append(2, L'\0');
		}

		if (maxSize != KxDynamicStringW::npos && result.length() > maxSize)
		{
			result.resize(maxSize);
			if (maxSize >= 2)
			{
				result[maxSize - 2] = L'\0';
			}
			if (maxSize >= 1)
			{
				result[maxSize - 1] = L'\0';
			}
		}
		return result;
	}
}

namespace PPR
{
	bool INIWrapper::LoadUTF8(FILE* stream, size_t fileSize)
	{
		return m_INI.LoadFile(stream) == SI_OK;
	}
	bool INIWrapper::LoadUTF16LE(FILE* stream, size_t fileSize)
	{
		size_t bomLength = 0;
		if (SkipBOM(stream, BOM_UTF16_LE, &bomLength))
		{
			fileSize -= bomLength;
		}

		KxDynamicStringW fileData;
		fileData.resize((fileSize / sizeof(wchar_t)) + 1);

		if (fread(fileData.data(), 1, fileSize, stream) == fileSize)
		{
			KxDynamicStringA utf8 = fileData.to_utf8();
			return m_INI.LoadData(utf8.data(), utf8.size()) == SI_OK;
		}
		return false;
	}

	KxDynamicStringRefW INIWrapper::ExtractValue(KxDynamicStringRefW value) const
	{
		return ExtractValueAndComment(value, m_Options & Options::RemoveInlineComments);
	}

	bool INIWrapper::Load(KxDynamicStringRefW path, Options options, Encoding encoding)
	{
		m_Options = options;

		ArgsBuffer<1> argsBuffer;
		ProcessArgument<0>(argsBuffer, path);

		FILE* stream = _wfopen(path.data(), L"rb");
		if (stream)
		{
			KxCallAtScopeExit atExit([stream]()
			{
				fclose(stream);
			});

			// Get file size
			_fseeki64(stream, 0, SEEK_END);
			const int64_t fileSize = _ftelli64(stream);
			rewind(stream);

			switch (encoding)
			{
				case Encoding::Auto:
				{
					if (TestBOM(stream, BOM_UTF16_LE))
					{
						return LoadUTF16LE(stream, fileSize);
					}
					else if (TestBOM(stream, BOM_UTF16_BE))
					{
						return false;
					}
					else
					{
						return LoadUTF8(stream, fileSize);
					}
				}
				case Encoding::UTF8:
				{
					return LoadUTF8(stream, fileSize);
				}
				case Encoding::UTF16LE:
				{
					return LoadUTF16LE(stream, fileSize);
				}
			};
			return false;
		}
		return false;
	}
	bool INIWrapper::Save(KxDynamicStringRefW path, Options options, Encoding encoding)
	{
		ArgsBuffer<1> argsBuffer;
		ProcessArgument<0>(argsBuffer, path);

		const bool addSignature = options & Options::WithBOM;
		switch (encoding)
		{
			case Encoding::Auto:
			case Encoding::UTF8:
			{
				return m_INI.SaveFile(path.data(), addSignature) == SI_OK;
			}
			case Encoding::UTF16LE:
			{
				std::string buffer;
				if (m_INI.Save(buffer) == SI_OK)
				{
					FILE* stream = _wfopen(path.data(), L"w+b");
					if (stream)
					{
						KxCallAtScopeExit atExit([stream]()
						{
							fclose(stream);
						});

						KxDynamicStringW utf16 = KxDynamicStringW::to_utf16(buffer.data(), buffer.length(), CP_UTF8);
						if (addSignature)
						{
							const size_t length = std::size(BOM_UTF16_LE);
							if (fwrite(BOM_UTF16_LE, 1, length, stream) != length)
							{
								return false;
							}
						}

						const size_t length = utf16.length() * sizeof(wchar_t);
						return fwrite(utf16.data(), 1, length, stream) == length;
					}
				}
			}
		};
		return false;
	}

	std::optional<KxDynamicStringRefW> INIWrapper::QueryValue(KxDynamicStringRefW section, KxDynamicStringRefW key) const
	{
		ArgsBuffer<2> argsBuffer;
		ProcessArgument<0>(argsBuffer, section);
		ProcessArgument<1>(argsBuffer, key);

		if (const wchar_t* value = m_INI.GetValue(section.data(), key.data(), nullptr))
		{
			return ExtractValue(value);
		}
		return {};
	}
	std::optional<KxDynamicStringRefW> INIWrapper::QueryValue(KxDynamicStringRefW section, KxDynamicStringRefW key, const wchar_t* defaultValue) const
	{
		if (auto value = QueryValue(section, key))
		{
			return value;
		}
		else if (defaultValue)
		{
			return defaultValue;
		}
		return {};
	}

	bool INIWrapper::SetValue(KxDynamicStringRefW section, KxDynamicStringRefW key, KxDynamicStringRefW value)
	{
		ArgsBuffer<3> argsBuffer;
		ProcessArgument<0>(argsBuffer, section);
		ProcessArgument<1>(argsBuffer, key);
		ProcessArgument<2>(argsBuffer, value);

		const SI_Error res = m_INI.SetValue(section.data(), key.data(), value.data(), nullptr, true);
		return res == SI_INSERTED || res == SI_UPDATED;
	}

	size_t INIWrapper::GetSectionNames(TStringRefVector& sectionNames) const
	{
		CSimpleIniW::TNamesDepend sections;
		m_INI.GetAllSections(sections);
		sections.sort(CSimpleIniW::Entry::LoadOrder());

		sectionNames.reserve(sections.size());
		for (const CSimpleIniW::Entry& entry: sections)
		{
			sectionNames.emplace_back(ExtractValue(entry.pItem));
		}

		// Return count of added items
		return sections.size();
	}
	KxDynamicStringW INIWrapper::GetSectionNamesZSSTRZZ(size_t maxSize) const
	{
		CSimpleIniW::TNamesDepend sections;
		m_INI.GetAllSections(sections);
		sections.sort(CSimpleIniW::Entry::LoadOrder());

		return ToZSSTRZZ(sections, maxSize, m_Options & Options::RemoveInlineComments);
	}

	size_t INIWrapper::GetKeyNames(KxDynamicStringRefW section, TStringRefVector& keyNames) const
	{
		ArgsBuffer<1> argsBuffer;
		ProcessArgument<0>(argsBuffer, section);

		CSimpleIniW::TNamesDepend keys;
		m_INI.GetAllKeys(section.data(), keys);
		keys.sort(CSimpleIniW::Entry::LoadOrder());

		keyNames.reserve(keys.size());
		for (const CSimpleIniW::Entry& entry: keys)
		{
			keyNames.emplace_back(ExtractValue(entry.pItem));
		}

		// Return count of added items
		return keys.size();
	}
	KxDynamicStringW INIWrapper::GetKeyNamesZSSTRZZ(KxDynamicStringRefW section, size_t maxSize) const
	{
		ArgsBuffer<1> argsBuffer;
		ProcessArgument<0>(argsBuffer, section);

		CSimpleIniW::TNamesDepend keys;
		m_INI.GetAllKeys(section.data(), keys);
		keys.sort(CSimpleIniW::Entry::LoadOrder());
		return ToZSSTRZZ(keys, maxSize, m_Options & Options::RemoveInlineComments);
	}

	bool INIWrapper::DeleteSection(KxDynamicStringRefW section)
	{
		ArgsBuffer<1> argsBuffer;
		ProcessArgument<0>(argsBuffer, section);

		return m_INI.Delete(section.data(), nullptr, true);
	}
	bool INIWrapper::DeleteKey(KxDynamicStringRefW section, KxDynamicStringRefW key)
	{
		ArgsBuffer<2> argsBuffer;
		ProcessArgument<0>(argsBuffer, section);
		ProcessArgument<1>(argsBuffer, key);

		return m_INI.Delete(section.data(), key.data(), true);
	}
}
