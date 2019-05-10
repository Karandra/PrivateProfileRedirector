#include "stdafx.h"
#include "PrivateProfileRedirector.h"
#include "ScriptExtenderDefines.h"
#include <strsafe.h>
#include <detours.h>
#include <detver.h>
#pragma comment(lib, "detours.lib")

#pragma warning(disable: 4267) // conversion from 'x' to 'y', possible loss of data

#define LibraryName	"PrivateProfileRedirector"
#define InitFunctionN(name)		m_##name = &::name;
#define AttachFunctionN(name)	AttachFunction(&m_##name, &On_##name, L#name)
#define DetachFunctionN(name)	DetachFunction(&m_##name, &On_##name, L#name)

#define InitNtDLLFunctionN(name)	m_##name = reinterpret_cast<decltype(m_##name)>(GetProcAddress(m_NtDLL, #name));

//////////////////////////////////////////////////////////////////////////
bool INIObject::LoadFile()
{
	FILE* stream = _wfopen(m_Path, L"rb");
	if (stream)
	{
		PrivateProfileRedirector& instance = PrivateProfileRedirector::GetInstance();
		KxCallAtScopeExit atExit([stream]()
		{
			fclose(stream);
		});

		if (instance.ShouldProcessByteOrderMark() && SkipByteOrderMark(stream))
		{
			// If BOM was skipped, we can't use 'LoadFile' because it rewinds the stream before read
			// and reads content as narrow chars. So read data ourselves, convert it to UTF-8 and load
			// converted data instead.
			constexpr size_t bomLength = 2;

			// Get file size
			_fseeki64(stream, 0, SEEK_END);
			const int64_t fileSize = _ftelli64(stream);

			if (fileSize > bomLength)
			{
				// Seek after BOM
				_fseeki64(stream, bomLength, SEEK_SET);
				const size_t effectiveSize = (size_t)fileSize - bomLength;

				KxDynamicStringW fileData;
				fileData.resize((effectiveSize / sizeof(wchar_t)) + 1);

				if (fread(fileData.data(), 1, effectiveSize, stream) == effectiveSize)
				{
					// SimpleINI requires UTF-8, so convert file content
					KxDynamicStringA utf8 = KxDynamicStringW::to_codepage(fileData.data(), fileData.size(), CP_UTF8);
					m_INI.LoadData(utf8.data(), utf8.size());
				}
			}
		}
		else
		{
			// Load the file as usual
			m_INI.LoadFile(stream);
		}

		// Set properties
		m_ExistOnDisk = true;
		m_IsChanged = false;

		// Process inline comments if allowed
		if (!m_INI.IsEmpty() && instance.ShouldProcessInlineComments())
		{
			ProcessInlineComments();
		}
		return true;
	}
	return false;
}
bool INIObject::SaveFile()
{
	PrivateProfileRedirector& instance = PrivateProfileRedirector::GetInstance();
	if (instance.IsWriteProtected())
	{
		PrivateProfileRedirector::GetInstance().Log(L"[WriteProtected] Attempt to write data to '%s", m_Path.data());
		m_IsChanged = false;
		return false;
	}

	if (!m_INI.IsEmpty() && m_INI.SaveFile(m_Path, false) == SI_OK)
	{
		m_IsChanged = false;
		m_ExistOnDisk = true;
		return true;
	}
	return false;
}

bool INIObject::SkipByteOrderMark(FILE* stream) const
{
	uint8_t readBOM[2] = {0};
	if (fread(readBOM, 1, std::size(readBOM), stream) == std::size(readBOM))
	{
		constexpr const uint8_t bom[] = {0xFF, 0xFE};
		if (memcmp(readBOM, bom, std::size(bom)) == 0)
		{
			return true;
		}
	}

	rewind(stream);
	return false;
}
void INIObject::ProcessInlineComments()
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
		return std::wstring_view::npos;
	};

	INIFile::TNamesDepend sectionList;
	m_INI.GetAllSections(sectionList);
	for (const auto& section: sectionList)
	{
		INIFile::TNamesDepend keyList;
		m_INI.GetAllKeys(section.pItem, keyList);
		for (const auto& key: keyList)
		{
			std::wstring_view value = m_INI.GetValue(section.pItem, key.pItem);
			size_t anchor = FindCommentStart(value);
			if (anchor != std::wstring_view::npos)
			{
				KxDynamicStringW newValue = value.substr(0, anchor);
				PrivateProfileRedirector::TrimSpaceCharsLR(newValue);
				m_INI.SetValue(section.pItem, key.pItem, newValue.data(), nullptr, true);
			}
		}
	}
}

INIObject::INIObject(const KxDynamicStringW& path)
	:m_INI(false, false, true), m_Path(path)
{
	m_INI.SetSpaces(false);
}

void INIObject::OnWrite()
{
	m_IsChanged = true;
	
	PrivateProfileRedirector& instance = PrivateProfileRedirector::GetInstance();
	if (instance.ShouldSaveOnWrite())
	{
		PrivateProfileRedirector::GetInstance().Log(L"Saving file on write: '%s, Is empty: %d'", m_Path.data(), (int)m_INI.IsEmpty());
		SaveFile();
	}
}

//////////////////////////////////////////////////////////////////////////
PrivateProfileRedirector* PrivateProfileRedirector::ms_Instance = nullptr;
const int PrivateProfileRedirector::ms_VersionMajor = 0;
const int PrivateProfileRedirector::ms_VersionMinor = 3;
const int PrivateProfileRedirector::ms_VersionPatch = 4;

PrivateProfileRedirector& PrivateProfileRedirector::CreateInstance()
{
	DestroyInstance();
	ms_Instance = new PrivateProfileRedirector();

	return *ms_Instance;
}
void PrivateProfileRedirector::DestroyInstance()
{
	delete ms_Instance;
	ms_Instance = nullptr;
}

const wchar_t* PrivateProfileRedirector::GetLibraryNameW()
{
	return _CRT_WIDE(LibraryName);
}
const wchar_t* PrivateProfileRedirector::GetLibraryVersionW()
{
	static wchar_t ms_VersionW[16] = {0};
	if (*ms_VersionW == L'\000')
	{
		swprintf_s(ms_VersionW, L"%d.%d.%d", ms_VersionMajor, ms_VersionMinor, ms_VersionPatch);
	}
	return ms_VersionW;
}
const char* PrivateProfileRedirector::GetLibraryNameA()
{
	return LibraryName;
}
const char* PrivateProfileRedirector::GetLibraryVersionA()
{
	static char ms_VersionA[16] = {0};
	if (*ms_VersionA == '\000')
	{
		sprintf_s(ms_VersionA, "%d.%d.%d", ms_VersionMajor, ms_VersionMinor, ms_VersionPatch);
	}
	return ms_VersionA;
}

int PrivateProfileRedirector::GetLibraryVersionInt()
{
	// 1.2.3 -> 1 * 100 + 2 * 10 + 3 * 1 = 123
	// 0.1 -> (0 * 100) + (1 * 10) + (0 * 1) = 10
	return (ms_VersionMajor * 100) + (ms_VersionMinor * 10) + (ms_VersionPatch * 1);
}

//////////////////////////////////////////////////////////////////////////
void PrivateProfileRedirector::InitFunctions()
{
	InitNtDLLFunctionN(RtlInitUnicodeString);
	InitNtDLLFunctionN(RtlUnicodeStringToInteger);

	InitFunctionN(GetPrivateProfileStringA);
	InitFunctionN(GetPrivateProfileStringW);

	InitFunctionN(GetPrivateProfileIntA);
	InitFunctionN(GetPrivateProfileIntW);

	InitFunctionN(GetPrivateProfileSectionNamesA);
	InitFunctionN(GetPrivateProfileSectionNamesW);

	InitFunctionN(GetPrivateProfileSectionA);
	InitFunctionN(GetPrivateProfileSectionW);

	InitFunctionN(WritePrivateProfileStringA);
	InitFunctionN(WritePrivateProfileStringW);
}

LONG PrivateProfileRedirector::DetourAttachFunction(void** originalFunc, void* overrideFunc)
{
	return ::DetourAttach(originalFunc, overrideFunc);
}
LONG PrivateProfileRedirector::DetourDetachFunction(void** originalFunc, void* overrideFunc)
{
	return ::DetourDetach(originalFunc, overrideFunc);
}
void PrivateProfileRedirector::LogAttachDetachStatus(LONG status, const wchar_t* operation, const FunctionInfo& info)
{
	switch (status)
	{
		case NO_ERROR:
		{
			Log(L"[%s]: %s -> NO_ERROR", operation, info.Name);
			break;
		}
		case ERROR_INVALID_BLOCK:
		{
			Log(L"[%s]: %s -> ERROR_INVALID_BLOCK", operation, info.Name);
			break;
		}
		case ERROR_INVALID_HANDLE:
		{
			Log(L"[%s]: %s -> ERROR_INVALID_HANDLE", operation, info.Name);
			break;
		}
		case ERROR_INVALID_OPERATION:
		{
			Log(L"[%s]: %s -> ERROR_INVALID_OPERATION", operation, info.Name);
			break;
		}
		case ERROR_NOT_ENOUGH_MEMORY:
		{
			Log(L"[%s]: %s -> ERROR_NOT_ENOUGH_MEMORY", operation, info.Name);
			break;
		}
		default:
		{
			Log(L"[%s]: %s -> <Unknown>", operation, info.Name);
			break;
		}
	};
}

void PrivateProfileRedirector::OverrideFunctions()
{
	// 1
	{
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());

		AttachFunctionN(GetPrivateProfileStringA);
		AttachFunctionN(GetPrivateProfileStringW);

		AttachFunctionN(GetPrivateProfileIntA);
		AttachFunctionN(GetPrivateProfileIntW);

		DetourTransactionCommit();
	}

	// 2
	{
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());

		AttachFunctionN(WritePrivateProfileStringA);
		AttachFunctionN(WritePrivateProfileStringW);

		AttachFunctionN(GetPrivateProfileSectionNamesA);
		AttachFunctionN(GetPrivateProfileSectionNamesW);

		DetourTransactionCommit();
	}

	// 3
	{
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());

		AttachFunctionN(GetPrivateProfileSectionA);
		AttachFunctionN(GetPrivateProfileSectionW);

		DetourTransactionCommit();
	}
}
void PrivateProfileRedirector::RestoreFunctions()
{
	// 1
	{
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());

		DetachFunctionN(GetPrivateProfileStringA);
		DetachFunctionN(GetPrivateProfileStringW);

		DetachFunctionN(GetPrivateProfileIntA);
		DetachFunctionN(GetPrivateProfileIntW);

		DetourTransactionCommit();
	}

	// 2
	{
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());

		DetachFunctionN(WritePrivateProfileStringA);
		DetachFunctionN(WritePrivateProfileStringW);

		DetachFunctionN(GetPrivateProfileSectionNamesA);
		DetachFunctionN(GetPrivateProfileSectionNamesW);

		DetourTransactionCommit();
	}

	// 3
	{
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());

		DetachFunctionN(GetPrivateProfileSectionA);
		DetachFunctionN(GetPrivateProfileSectionW);

		DetourTransactionCommit();
	}
}

const wchar_t* PrivateProfileRedirector::GetConfigOption(const wchar_t* section, const wchar_t* key, const wchar_t* defaultValue) const
{
	return m_Config.GetValue(section, key, defaultValue);
}
int PrivateProfileRedirector::GetConfigOptionInt(const wchar_t* section, const wchar_t* key, int defaultValue) const
{
	const wchar_t* value = GetConfigOption(section, key, nullptr);
	if (value)
	{
		int valueInt = defaultValue;
		swscanf(value, L"%d", &valueInt);
		return valueInt;
	}
	return defaultValue;
}

PrivateProfileRedirector::PrivateProfileRedirector()
	:m_ThreadID(GetCurrentThreadId()), m_Config(false, false, false)
{
	m_NtDLL = ::LoadLibraryW(L"NtDLL.dll");

	// Load config
	m_Config.LoadFile(L"Data\\" xSE_FOLDER_NAME_W "\\Plugins\\PrivateProfileRedirector.ini");

	// Open log
	if (GetConfigOptionBool(L"General", L"EnableLog", false))
	{
		_wfopen_s(&m_Log, L"Data\\" xSE_FOLDER_NAME_W L"\\Plugins\\PrivateProfileRedirector.log", L"wb+");
	}
	Log(L"Log opened");
	Log(L"Version: %s", GetLibraryVersionW());
	Log(L"Script Extender platform: %s", xSE_NAME_W);

	// Load options
	m_AllowSEVersionMismatch = GetConfigOptionBool(L"General", L"AllowSEVersionMismatch", m_AllowSEVersionMismatch);
	m_WriteProtected = GetConfigOptionBool(L"General", L"WriteProtected", m_WriteProtected);
	m_NativeWrite = !m_WriteProtected && GetConfigOptionBool(L"General", L"NativeWrite", m_NativeWrite);
	m_ShouldSaveOnWrite = !m_NativeWrite && GetConfigOptionBool(L"General", L"SaveOnWrite", m_ShouldSaveOnWrite);
	m_ShouldSaveOnThreadDetach =!m_NativeWrite && GetConfigOptionBool(L"General", L"SaveOnThreadDetach", m_ShouldSaveOnThreadDetach);
	m_TrimKeyNamesA = GetConfigOptionBool(L"General", L"TrimKeyNamesA", m_TrimKeyNamesA);
	m_TrimValueQuotes = GetConfigOptionBool(L"General", L"TrimValueQuotes", m_TrimValueQuotes);
	m_ProcessInlineComments = GetConfigOptionBool(L"General", L"ProcessInlineComments", m_ProcessInlineComments);
	m_ProcessByteOrderMark = GetConfigOptionBool(L"General", L"ProcessByteOrderMark", m_ProcessByteOrderMark);
	m_DisableCCUnsafeA = GetConfigOptionBool(L"General", L"DisableCCUnsafeA", m_DisableCCUnsafeA);
	m_ANSICodePage = GetConfigOptionInt(L"General", L"ANSICodePage", m_ANSICodePage);

	// Print options
	Log(L"Loaded options:");
	Log(L"General/EnableLog -> '%d'", (int)(m_Log != nullptr));
	Log(L"General/NativeWrite -> '%d'", (int)m_NativeWrite);
	Log(L"General/SaveOnWrite -> '%d'", (int)m_ShouldSaveOnWrite);
	Log(L"General/SaveOnThreadDetach -> '%d'", (int)m_ShouldSaveOnThreadDetach);
	Log(L"General/TrimKeyNamesA -> '%d'", (int)m_TrimKeyNamesA);
	Log(L"General/TrimValueQuotes -> '%d'", (int)m_TrimValueQuotes);
	Log(L"General/ProcessInlineComments -> '%d'", (int)m_ProcessInlineComments);
	Log(L"General/WriteProtected -> '%d'", (int)m_WriteProtected);
	Log(L"General/ANSICodePage -> '%d'", (int)m_ANSICodePage);

	// Save function pointers
	InitFunctions();

	// Initialize detour
	DetourRestoreAfterWith();
	OverrideFunctions();
}
PrivateProfileRedirector::~PrivateProfileRedirector()
{
	// Uninitializing
	RestoreFunctions();

	// Save files
	if (ShouldSaveOnProcessDetach())
	{
		SaveChnagedFiles(L"On process detach");
	}

	// Close log
	Log(L"Log closed");
	if (m_Log)
	{
		fclose(m_Log);
	}

	if (m_NtDLL)
	{
		::FreeLibrary(m_NtDLL);
	}
}

INIObject& PrivateProfileRedirector::GetOrLoadFile(KxDynamicStringRefW path)
{
	auto it = m_INIMap.find(path);
	if (it != m_INIMap.end())
	{
		return *it->second;
	}
	else
	{
		KxCriticalSectionLocker lock(m_INIMapCS);

		auto& ini = m_INIMap.insert_or_assign(path, std::make_unique<INIObject>(path)).first->second;
		ini->LoadFile();

		Log(L"Attempt to access file: '%s' -> file object initialized. Exist on disk: %d", path.data(), ini->IsExistOnDisk());
		return *ini;
	}
}
void PrivateProfileRedirector::SaveChnagedFiles(const wchar_t* message) const
{
	Log(L"Saving files: %s", message);

	size_t changedCount = 0;
	for (const auto& v: m_INIMap)
	{
		const auto& iniObject = v.second;
		if (iniObject->IsChanged())
		{
			changedCount++;
			iniObject->SaveFile();
			Log(L"File saved: '%s', Is empty: %d", v.first.data(), (int)iniObject->IsEmpty());
		}
		else
		{
			Log(L"File wasn't changed: '%s', Is empty: %d", v.first.data(), (int)iniObject->IsEmpty());
		}
	}
	Log(L"All changed files saved. Total: %zu, Changed: %zu", m_INIMap.size(), changedCount);
}
size_t PrivateProfileRedirector::RefreshINI()
{
	Log(L"Executing 'RefreshINI'");
	KxCriticalSectionLocker mapLock(m_INIMapCS);

	for (const auto& v: m_INIMap)
	{
		auto& iniObject = v.second;
		
		Log(L"Reloading '%s'", v.first.data());
		KxCriticalSectionLocker lock(iniObject->GetLock());
		iniObject->LoadFile();
	}

	Log(L"Executing 'RefreshINI' done, %zu files reloaded.", m_INIMap.size());
	return m_INIMap.size();
}

KxDynamicStringW& PrivateProfileRedirector::TrimCharsL(KxDynamicStringW& value, KxDynamicStringW::TChar c1, KxDynamicStringW::TChar c2)
{
	if (!value.empty())
	{
		size_t trimLeft = 0;
		for (size_t i = 0; i < value.size(); i++)
		{
			if (value[i] == c1 || value[i] == c2)
			{
				trimLeft++;
			}
			else
			{
				break;
			}
		}
		value.erase(0, trimLeft);
	}
	return value;
}
KxDynamicStringW& PrivateProfileRedirector::TrimCharsR(KxDynamicStringW& value, KxDynamicStringW::TChar c1, KxDynamicStringW::TChar c2)
{
	if (!value.empty())
	{
		size_t trimRight = 0;
		for (size_t i = value.size() - 1; i != 0; i--)
		{
			if (value[i] == c1 || value[i] == c2)
			{
				trimRight++;
			}
			else
			{
				break;
			}
		}
		value.erase(value.size() - trimRight, trimRight);
	}
	return value;
}

//////////////////////////////////////////////////////////////////////////
#undef PPR_API
#define PPR_API(retType) retType WINAPI

namespace
{
	// Zero Separated STRing Zero Zero
	size_t KeysSectionsToZSSTRZZ(const INIFile::TNamesDepend& valuesList, std::wstring& zsstrzz, size_t maxSize)
	{
		if (!valuesList.empty())
		{
			for (const auto& v: valuesList)
			{
				zsstrzz.append(v.pItem);
				zsstrzz.append(1, L'\000');
			}
			zsstrzz.append(1, L'\000');
		}
		else
		{
			zsstrzz.append(2, L'\000');
		}

		if (zsstrzz.length() >= maxSize)
		{
			zsstrzz.resize(maxSize);
			if (zsstrzz.size() >= 2)
			{
				zsstrzz[zsstrzz.size() - 2] = L'\000';
			}
			if (zsstrzz.size() >= 1)
			{
				zsstrzz[zsstrzz.size() - 1] = L'\000';
			}
		}
		return zsstrzz.length();
	}

	template<class T, class CharT> size_t CopyToBufferT(CharT* buffer, const T& data)
	{
		std::memcpy(buffer, data.data(), data.size() * sizeof(CharT));
		return data.size();
	}
	template<class T> size_t CopyToBuffer(wchar_t* buffer, const T& data)
	{
		return CopyToBufferT(buffer, data);
	}
	template<class T> size_t CopyToBuffer(char* buffer, const T& data)
	{
		return CopyToBufferT(buffer, data);
	}
}

PPR_API(DWORD) On_GetPrivateProfileStringA(LPCSTR appName, LPCSTR keyName, LPCSTR defaultValue, LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName)
{
	PrivateProfileRedirector& instance = PrivateProfileRedirector::GetInstance();

	auto appNameW = instance.ConvertToUTF16(appName);
	auto keyNameW = instance.ConvertToUTF16(keyName);
	auto defaultValueW = instance.ConvertToUTF16(defaultValue);
	auto lpFileNameW = instance.ConvertToUTF16(lpFileName);

	instance.Log(L"[GetPrivateProfileStringA] Redirecting to 'GetPrivateProfileStringW'");
	
	KxDynamicStringW lpReturnedStringW;
	lpReturnedStringW.resize(nSize + 1);

	if (instance.ShouldTrimKeyNamesA())
	{
		PrivateProfileRedirector::TrimSpaceCharsLR(keyNameW);
		instance.Log(L"[GetPrivateProfileStringA] Trimmed key name: '%s'", keyNameW.data());
	}
	DWORD length = On_GetPrivateProfileStringW(appNameW, keyNameW, defaultValueW, lpReturnedStringW.data(), nSize, lpFileNameW);
	if (length != 0)
	{
		KxDynamicStringA result = instance.ConvertToCodePage(lpReturnedStringW.data());
		StringCchCopyNA(lpReturnedString, nSize, result.data(), result.length());
	}
	else
	{
		if (!instance.ShouldDisableCCUnsafeA())
		{
			StringCchCopyNA(lpReturnedString, nSize, "", 1);
		}
	}
	return length;
}
PPR_API(DWORD) On_GetPrivateProfileStringW(LPCWSTR appName, LPCWSTR keyName, LPCWSTR defaultValue, LPWSTR lpReturnedString, DWORD nSize, LPCWSTR lpFileName)
{
	PrivateProfileRedirector& instance = PrivateProfileRedirector::GetInstance();
	instance.Log(L"[GetPrivateProfileStringW] Section: '%s', Key: '%s', Default: '%s', Buffer size: '%u', Path: '%s'", appName, keyName, defaultValue, nSize, lpFileName);

	if (lpFileName)
	{
		if (lpReturnedString == nullptr || nSize == 0)
		{
			SetLastError(ERROR_INSUFFICIENT_BUFFER);
			return 0;
		}

		const INIObject& iniObject = instance.GetOrLoadFile(lpFileName);
		const INIFile& iniFile = iniObject.GetFile();

		// Enum all sections
		if (appName == nullptr)
		{
			instance.Log(L"[GetPrivateProfileStringW] Enum all sections of '%s'", lpFileName);

			std::wstring sectionsList;
			INIFile::TNamesDepend sections;
			iniFile.GetAllSections(sections);
			sections.sort(INIFile::Entry::LoadOrder());

			size_t length = KeysSectionsToZSSTRZZ(sections, sectionsList, nSize);
			StringCchCopyNW(lpReturnedString, nSize, sectionsList.data(), sectionsList.length());
			return static_cast<DWORD>(length);
		}

		// Enum all keys in section
		if (keyName == nullptr)
		{
			instance.Log(L"[GetPrivateProfileStringW] Enum all keys is '%s' section of '%s'", appName, lpFileName);

			std::wstring keysList;
			INIFile::TNamesDepend keys;
			iniFile.GetAllKeys(appName, keys);
			keys.sort(INIFile::Entry::LoadOrder());

			size_t length = KeysSectionsToZSSTRZZ(keys, keysList, nSize);
			StringCchCopyNW(lpReturnedString, nSize, keysList.data(), keysList.length());
			return static_cast<DWORD>(length);
		}

		LPCWSTR value = iniFile.GetValue(appName, keyName, defaultValue);
		if (value)
		{
			instance.Log(L"[GetPrivateProfileStringW] Value: '%s'", value);

			if (instance.ShouldTrimValueQuotes())
			{
				KxDynamicStringW valueTrimmed(value);
				PrivateProfileRedirector::TrimQuoteCharsLR(valueTrimmed);
				StringCchCopyNW(lpReturnedString, nSize, valueTrimmed.data(), valueTrimmed.length());

				instance.Log(L"[GetPrivateProfileStringW] Trimmed value: '%s'", valueTrimmed.data());
				return static_cast<DWORD>(valueTrimmed.length());
			}
			else
			{
				size_t length = 0;
				StringCchLengthW(value, nSize, &length);
				StringCchCopyNW(lpReturnedString, nSize, value, length);
				return static_cast<DWORD>(length);
			}
		}
		else
		{
			instance.Log(L"[GetPrivateProfileStringW] Value: '<null>'");

			StringCchCopyNW(lpReturnedString, nSize, L"", 1);
			return 0;
		}
	}

	SetLastError(ERROR_FILE_NOT_FOUND);
	return 0;
}

PPR_API(UINT) On_GetPrivateProfileIntA(LPCSTR appName, LPCSTR keyName, INT defaultValue, LPCSTR lpFileName)
{
	PrivateProfileRedirector& instance = PrivateProfileRedirector::GetInstance();
	instance.Log(L"[GetPrivateProfileIntA] Redirecting to 'GetPrivateProfileIntW'");

	auto appNameW = instance.ConvertToUTF16(appName);
	auto keyNameW = instance.ConvertToUTF16(keyName);
	auto lpFileNameW = instance.ConvertToUTF16(lpFileName);

	if (instance.ShouldTrimKeyNamesA())
	{
		PrivateProfileRedirector::TrimSpaceCharsLR(keyNameW);
	}
	return On_GetPrivateProfileIntW(appNameW, keyNameW, defaultValue, lpFileNameW);
}
PPR_API(UINT) On_GetPrivateProfileIntW(LPCWSTR appName, LPCWSTR keyName, INT defaultValue, LPCWSTR lpFileName)
{
	PrivateProfileRedirector& instance = PrivateProfileRedirector::GetInstance();
	instance.Log(L"[GetPrivateProfileIntW]: Section: '%s', Key: '%s', Default: '%d', Path: '%s'", appName, keyName, defaultValue, lpFileName);
	
	if (lpFileName && appName && keyName)
	{
		INIObject& ini = instance.GetOrLoadFile(lpFileName);
		LPCWSTR value = ini.GetFile().GetValue(appName, keyName);
		if (value)
		{
			UNICODE_STRING string = {0};
			instance.m_RtlInitUnicodeString(&string, value);

			ULONG intValue = defaultValue;
			instance.m_RtlUnicodeStringToInteger(&string, 0, &intValue);

			instance.Log(L"[GetPrivateProfileIntW] ValueString: '%s', ValueInt: '%d'", value, intValue);
			return intValue;
		}

		instance.Log(L"[GetPrivateProfileIntW] ValueString: '<null>', ValueInt: '%d'", defaultValue);
		return defaultValue;
	}

	SetLastError(ERROR_FILE_NOT_FOUND);
	return defaultValue;
}

PPR_API(DWORD) On_GetPrivateProfileSectionNamesA(LPSTR lpszReturnBuffer, DWORD nSize, LPCSTR lpFileName)
{
	PrivateProfileRedirector& instance = PrivateProfileRedirector::GetInstance();
	instance.Log(L"[GetPrivateProfileSectionNamesA] Redirecting to 'GetPrivateProfileSectionNamesW'");

	if (lpszReturnBuffer == nullptr || nSize == 0)
	{
		return 0;
	}
	if (nSize == 1)
	{
		*lpszReturnBuffer = '\000';
		return 0;
	}

	KxDynamicStringW lpFileNameW = instance.ConvertToUTF16(lpFileName);
	KxDynamicStringW lpszReturnBufferW;
	lpszReturnBufferW.resize(nSize);

	DWORD length = On_GetPrivateProfileSectionNamesW(lpszReturnBufferW.data(), nSize, lpFileNameW);
	if (length <= nSize)
	{
		KxDynamicStringA result = instance.ConvertToCodePage(lpszReturnBufferW.data(), lpszReturnBufferW.length());
		CopyToBuffer(lpszReturnBuffer, result);
	}
	else
	{
		StringCchCopyNA(lpszReturnBuffer, nSize, "", 1);
	}
	return length;
}
PPR_API(DWORD) On_GetPrivateProfileSectionNamesW(LPWSTR lpszReturnBuffer, DWORD nSize, LPCWSTR lpFileName)
{
	PrivateProfileRedirector& instance = PrivateProfileRedirector::GetInstance();
	instance.Log(L"[GetPrivateProfileSectionNamesW]: Buffer size: '%u', Path: '%s'", nSize, lpFileName);

	if (lpszReturnBuffer == nullptr || nSize == 0)
	{
		return 0;
	}
	if (nSize == 1)
	{
		*lpszReturnBuffer = L'\000';
		return 0;
	}

	const INIObject& iniObject = instance.GetOrLoadFile(lpFileName);
	const INIFile& iniFile = iniObject.GetFile();

	INIFile::TNamesDepend sectionList;
	iniFile.GetAllSections(sectionList);
	if (!sectionList.empty())
	{
		sectionList.sort(INIFile::Entry::LoadOrder());
		std::wstring sectionNames;

		size_t length = KeysSectionsToZSSTRZZ(sectionList, sectionNames, nSize);
		if (length <= nSize)
		{
			if (instance.IsLogEnabled())
			{
				instance.Log(L"Sections:");
				for (const auto& v: sectionList)
				{
					instance.Log(L"%s", v.pItem);
				}
			}

			return CopyToBuffer(lpszReturnBuffer, sectionNames) - 1;
		}
		else
		{
			instance.Log(L"[GetPrivateProfileSectionNamesW]: Buffer is not large enough to contain all section names, '%zu' required, only '%u' available.", length, nSize);
			return nSize - 2;
		}
	}
	else
	{
		*lpszReturnBuffer = L'\000';
		return 0;
	}
}

PPR_API(DWORD) On_GetPrivateProfileSectionA(LPCSTR appName, LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName)
{
	PrivateProfileRedirector& instance = PrivateProfileRedirector::GetInstance();
	instance.Log(L"[GetPrivateProfileSectionA] Redirecting to 'GetPrivateProfileSectionW'");

	if (lpReturnedString == nullptr || nSize == 0)
	{
		return 0;
	}
	if (nSize == 1)
	{
		*lpReturnedString = '\000';
		return 0;
	}

	auto appNameW = instance.ConvertToUTF16(appName);
	auto lpFileNameW = instance.ConvertToUTF16(lpFileName);
	
	KxDynamicStringW lpReturnedStringW;
	lpReturnedStringW.resize(nSize);

	DWORD length = On_GetPrivateProfileSectionW(appNameW, lpReturnedStringW.data(), nSize, lpFileNameW);
	if (length <= nSize)
	{
		KxDynamicStringA result = instance.ConvertToCodePage(lpReturnedStringW.data(), lpReturnedStringW.length());
		CopyToBuffer(lpReturnedString, result);
	}
	else
	{
		StringCchCopyNA(lpReturnedString, nSize, "", 1);
	}
	return length;
}
PPR_API(DWORD) On_GetPrivateProfileSectionW(LPCWSTR appName, LPWSTR lpReturnedString, DWORD nSize, LPCWSTR lpFileName)
{
	PrivateProfileRedirector& instance = PrivateProfileRedirector::GetInstance();
	instance.Log(L"[GetPrivateProfileSectionW] Section: '%s', Buffer size: '%u', Path: '%s'", appName, nSize, lpFileName);

	if (lpReturnedString == nullptr || nSize == 0)
	{
		return 0;
	}
	if (nSize == 1)
	{
		*lpReturnedString = L'\000';
		return 0;
	}

	const INIObject& iniObject = instance.GetOrLoadFile(lpFileName);
	const INIFile& iniFile = iniObject.GetFile();

	INIFile::TNamesDepend keyList;
	iniFile.GetAllKeys(appName, keyList);
	if (!keyList.empty())
	{
		keyList.sort(INIFile::Entry::LoadOrder());
		std::wstring result;

		for (const auto& key: keyList)
		{
			result.append(key.pItem);
			result.append(1, L'=');
			result.append(iniFile.GetValue(appName, key.pItem));
			result.append(1, L'\000');
		}
		result.append(1, L'\000');

		if (result.length() <= nSize)
		{
			if (instance.IsLogEnabled())
			{
				instance.Log(L"Key-value pairs for section '%s':", appName);
				for (const auto& key: keyList)
				{
					instance.Log(L"%s=%s", key.pItem, iniFile.GetValue(appName, key.pItem));
				}
			}

			return CopyToBuffer(lpReturnedString, result) - 1;
		}
		else
		{
			instance.Log(L"[GetPrivateProfileSectionW]: Buffer is not large enough to contain all key-value pairs, '%zu' required, only '%u' available.", result.length(), nSize);
			return nSize - 2;
		}
	}
	else
	{
		*lpReturnedString = L'\000';
		return 0;
	}
}

PPR_API(BOOL) On_WritePrivateProfileStringA(LPCSTR appName, LPCSTR keyName, LPCSTR lpString, LPCSTR lpFileName)
{
	PrivateProfileRedirector& instance = PrivateProfileRedirector::GetInstance();
	instance.Log(L"[WritePrivateProfileStringA] Redirecting to 'WritePrivateProfileStringW'");
	
	auto appNameW = instance.ConvertToUTF16(appName);
	auto keyNameW = instance.ConvertToUTF16(keyName);
	auto lpStringW = instance.ConvertToUTF16(lpString);
	auto lpFileNameW = instance.ConvertToUTF16(lpFileName);

	if (instance.ShouldTrimKeyNamesA())
	{
		PrivateProfileRedirector::TrimSpaceCharsLR(keyNameW);
	}
	return On_WritePrivateProfileStringW(appNameW, keyNameW, lpStringW, lpFileNameW);
}
PPR_API(BOOL) On_WritePrivateProfileStringW(LPCWSTR appName, LPCWSTR keyName, LPCWSTR lpString, LPCWSTR lpFileName)
{
	PrivateProfileRedirector& instance = PrivateProfileRedirector::GetInstance();
	instance.Log(L"[WritePrivateProfileStringW] Section: '%s', Key: '%s', Value: '%s', Path: '%s'", appName, keyName, lpString, lpFileName);
	
	auto CustomWrite = [](LPCWSTR appName, LPCWSTR keyName, LPCWSTR lpString, LPCWSTR lpFileName) -> BOOL
	{
		if (appName)
		{
			INIObject& iniObject = PrivateProfileRedirector::GetInstance().GetOrLoadFile(lpFileName);
			KxCriticalSectionLocker lock(iniObject.GetLock());
			INIFile& ini = iniObject.GetFile();

			// Delete section
			if (keyName == nullptr)
			{
				if (ini.Delete(appName, nullptr, true))
				{
					iniObject.OnWrite();
					return TRUE;
				}
				return FALSE;
			}

			// Delete value
			if (lpString == nullptr)
			{
				if (ini.DeleteValue(appName, keyName, nullptr, true))
				{
					iniObject.OnWrite();
					return TRUE;
				}
				return FALSE;
			}

			// Set value
			SI_Error ret = ini.SetValue(appName, keyName, lpString, nullptr, true);
			if (ret == SI_INSERTED || ret == SI_UPDATED)
			{
				iniObject.OnWrite();
				return TRUE;
			}
		}
		return FALSE;
	};

	// This will write value into in-memory file.
	// When 'NativeWrite' or 'WriteProtected' is enabled, it will not flush updated file to disk.
	BOOL customWriteRet = CustomWrite(appName, keyName, lpString, lpFileName);

	// Call native function or proceed with own implementation
	if (instance.IsNativeWrite())
	{
		instance.Log(L"[WritePrivateProfileStringW]: Calling native 'WritePrivateProfileStringW'");
		return (*instance.m_WritePrivateProfileStringW)(appName, keyName, lpString, lpFileName);
	}
	else
	{
		if (!customWriteRet)
		{
			SetLastError(ERROR_FILE_NOT_FOUND);
		}
		return customWriteRet;
	}
}

//////////////////////////////////////////////////////////////////////////
BOOL APIENTRY DllMain(HMODULE module, DWORD event, LPVOID lpReserved)
{
    switch (event)
    {
		case DLL_PROCESS_ATTACH:
		{
			PrivateProfileRedirector::CreateInstance();
			break;
		}
		case DLL_THREAD_DETACH:
		{
			if (PrivateProfileRedirector* instance = PrivateProfileRedirector::GetInstancePtr())
			{
				if (instance->ShouldSaveOnThreadDetach())
				{
					instance->SaveChnagedFiles(L"On thread detach");
				}
			}
			break;
		}
		case DLL_PROCESS_DETACH:
		{
			PrivateProfileRedirector::DestroyInstance();
			break;
		}
    }
    return TRUE;
}

void DummyFunction()
{
}
