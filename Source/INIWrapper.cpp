#include "stdafx.h"
#include "INIWrapper.h"
#include "Utility/KxCallAtScopeExit.h"
#include "Utility/String.h"

namespace
{
	constexpr uint8_t BOM_UTF8[] = {0xEF, 0xBB, 0xBF};
	constexpr uint8_t BOM_UTF16_LE[] = {0xFF, 0xFE};
	constexpr uint8_t BOM_UTF16_BE[] = {0xFE, 0xFF};

	template<class T, size_t size> bool SkipBOM(FILE* stream, const T(&bom)[size], size_t* bomLength = nullptr, bool alwaysReset = false)
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
	template<class T, size_t size> bool TestBOM(FILE* stream, const T(&bom)[size])
	{
		return SkipBOM(stream, bom, nullptr, true);
	}

	bool TrimSpaceCharsLR(KxDynamicStringRefW& value)
	{
		const size_t oldLength = value.length();
		value = PPR::Utility::String::TrimSpaceCharsLR(value);
		return oldLength != value.length();
	}
	KxDynamicStringRefW TrimAll(KxDynamicStringRefW value)
	{
		using namespace PPR::Utility;
		return String::TrimSpaceCharsLR(String::TrimQuoteCharsLR(value));
	}
	
	template<size_t count> using ArgsBuffer = std::array<KxDynamicStringW, count>;
	template<size_t arg, class TArgs> void ProcessArgument(TArgs& argsBuffer, KxDynamicStringRefW& value)
	{
		if (TrimSpaceCharsLR(value))
		{
			// Copy string to the buffer from string_view to make it null terminated if we trimmed it
			argsBuffer[(size_t)arg] = value;

			// Update string_view to refer to the new null-terminated buffer
			value = argsBuffer[(size_t)arg];
		}
	}

	KxDynamicStringW ToZSSTRZZ(const CSimpleIniW::TNamesDepend& valuesList, size_t maxSize)
	{
		KxDynamicStringW result;
		if (!valuesList.empty())
		{
			for (const auto& v: valuesList)
			{
				result.append(v.pItem);
				result.append(1, L'\000');

				if (result.length() >= maxSize)
				{
					break;
				}
			}
			result.append(1, L'\000');
		}
		else
		{
			result.append(2, L'\000');
		}

		if (maxSize != KxDynamicStringW::npos && result.length() > maxSize)
		{
			result.resize(maxSize);
			if (maxSize >= 2)
			{
				result[maxSize - 2] = L'\000';
			}
			if (maxSize >= 1)
			{
				result[maxSize - 1] = L'\000';
			}
		}
		return result;
	}
}

namespace PPR
{
	void INIWrapper::RemoveInlineComments()
	{
		auto FindCommentStart = [](const std::wstring_view& value) -> size_t
		{
			for (size_t i = 0; i < value.size(); i++)
			{
				const wchar_t c = value[i];
				if (c == L';' || c == L'#')
				{
					return i;
				}
			}
			return KxDynamicStringRefW::npos;
		};

		CSimpleIniW::TNamesDepend sectionList;
		m_INI.GetAllSections(sectionList);
		for (const auto& section: sectionList)
		{
			CSimpleIniW::TNamesDepend keyList;
			m_INI.GetAllKeys(section.pItem, keyList);
			for (const auto& key: keyList)
			{
				KxDynamicStringRefW value = m_INI.GetValue(section.pItem, key.pItem);
				const size_t anchor = FindCommentStart(value);
				if (anchor != KxDynamicStringRefW::npos)
				{
					value = value.substr(0, anchor);
					KxDynamicStringRefW trimmed = Utility::String::TrimSpaceCharsLR(value);
					if (trimmed.length() != value.length())
					{
						KxDynamicStringW trimmedCopy = trimmed;
						m_INI.SetValue(section.pItem, key.pItem, trimmedCopy.data(), nullptr, true);
					}
				}
			}
		}
	}
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

	bool INIWrapper::Load(const KxDynamicStringW& path, Options options, Encoding encoding)
	{
		FILE* stream = _wfopen(path, L"rb");
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

			bool isSuccess = false;
			switch (encoding)
			{
				case Encoding::Auto:
				{
					if (TestBOM(stream, BOM_UTF16_LE))
					{
						isSuccess = LoadUTF16LE(stream, fileSize);
					}
					else if (TestBOM(stream, BOM_UTF16_BE))
					{
						isSuccess = false;
					}
					else
					{
						isSuccess = LoadUTF8(stream, fileSize);
					}
					break;
				}
				case Encoding::UTF8:
				{
					isSuccess = LoadUTF8(stream, fileSize);
					break;
				}
				case Encoding::UTF16LE:
				{
					isSuccess = LoadUTF16LE(stream, fileSize);
					break;
				}
				default:
				{
					return false;
				}
			};

			if (isSuccess && options & Options::RemoveInlineComments)
			{
				RemoveInlineComments();
			}
			return isSuccess;
		}
		return false;
	}
	bool INIWrapper::Save(const KxDynamicStringW& path, Options options, Encoding encoding)
	{
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
					FILE* stream = _wfopen(path, L"w+b");
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

	std::optional<KxDynamicStringRefW> INIWrapper::TryGetValue(KxDynamicStringRefW section, KxDynamicStringRefW key) const
	{
		ArgsBuffer<2> argsBuffer;
		ProcessArgument<0>(argsBuffer, section);
		ProcessArgument<1>(argsBuffer, key);

		const wchar_t* value = m_INI.GetValue(section.data(), key.data(), nullptr);
		if (value)
		{
			return TrimAll(value);
		}
		return std::nullopt;
	}
	std::optional<KxDynamicStringRefW> INIWrapper::TryGetValue(KxDynamicStringRefW section, KxDynamicStringRefW key, const wchar_t* defaultValue) const
	{
		auto value = TryGetValue(section, key);
		if (value)
		{
			return value;
		}
		else if (defaultValue)
		{
			return defaultValue;
		}
		return std::nullopt;
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
			sectionNames.emplace_back(entry.pItem);
		}

		// Return count of added items
		return sections.size();
	}
	KxDynamicStringW INIWrapper::GetSectionNamesZSSTRZZ(size_t maxSize) const
	{
		CSimpleIniW::TNamesDepend sections;
		m_INI.GetAllSections(sections);
		sections.sort(CSimpleIniW::Entry::LoadOrder());

		return ToZSSTRZZ(sections, maxSize);
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
			keyNames.emplace_back(entry.pItem);
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
		return ToZSSTRZZ(keys, maxSize);
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
