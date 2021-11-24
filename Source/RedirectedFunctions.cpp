#include "stdafx.h"
#include "RedirectedFunctions.h"
#include "PrivateProfileRedirector.h"
#include <strsafe.h>
#pragma warning(disable: 4267) // conversion from 'x' to 'y', possible loss of data
#pragma warning(disable: 4244) // ~

#undef PPR_API
#define PPR_API(retType) retType WINAPI

namespace
{
	template<class T, class TChar>
	void CopyToBuffer(TChar* buffer, const T& data)
	{
		std::memcpy(buffer, data.data(), data.size() * sizeof(TChar));
	}
	
	const wchar_t* DataOrNull(const KxDynamicStringW& string)
	{
		return string.empty() ? nullptr : string.data();
	}
}

namespace PPR::PrivateProfile
{
	PPR_API(DWORD) GetStringA(LPCSTR appName, LPCSTR keyName, LPCSTR defaultValue, LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName)
	{
		Redirector& redirector = Redirector::GetInstance();

		KxDynamicStringW appNameW = redirector.ConvertFromACP(appName);
		KxDynamicStringW keyNameW = redirector.ConvertFromACP(keyName);
		KxDynamicStringW defaultValueW = redirector.ConvertFromACP(defaultValue);
		KxDynamicStringW lpFileNameW = redirector.ConvertFromACP(lpFileName);

		redirector.Log(L"[GetPrivateProfileStringA] Redirecting to 'GetPrivateProfileStringW'");

		KxDynamicStringW lpReturnedStringW;
		lpReturnedStringW.resize(nSize + 1);

		const DWORD length = GetStringW(DataOrNull(appNameW),
										DataOrNull(keyNameW),
										DataOrNull(defaultValueW),
										lpReturnedStringW.data(),
										nSize,
										DataOrNull(lpFileNameW)
		);
		if (length != 0)
		{
			KxDynamicStringA result = redirector.ConvertToACP(lpReturnedStringW.data(), length);
			StringCchCopyNA(lpReturnedString, nSize, result.data(), result.length());
		}
		else
		{
			StringCchCopyNA(lpReturnedString, nSize, "", 1);
		}
		return length;
	}
	PPR_API(DWORD) GetStringW(LPCWSTR appName, LPCWSTR keyName, LPCWSTR defaultValue, LPWSTR lpReturnedString, DWORD nSize, LPCWSTR lpFileName)
	{
		Redirector& redirector = Redirector::GetInstance();
		redirector.Log(L"[GetPrivateProfileStringW] Section: '%s', Key: '%s', Default: '%s', Buffer size: '%u', Path: '%s'", appName, keyName, defaultValue, nSize, lpFileName);

		if (lpFileName)
		{
			if (!lpReturnedString || nSize < 2)
			{
				::SetLastError(ERROR_INSUFFICIENT_BUFFER);
				return 0;
			}

			ConfigObject& configObject = redirector.GetOrLoadFile(lpFileName);
			auto lock = configObject.LockShared();
			const INIWrapper& ini = configObject.GetINI();

			// Enum all sections
			if (!appName)
			{
				redirector.Log(L"[GetPrivateProfileStringW] Enum all sections of file '%s'", lpFileName);

				size_t count = 0;
				bool truncated = false;
				KxDynamicStringW sections = ini.GetSectionNamesZSSTRZZ(nSize, &truncated, &count);
				redirector.Log(L"[GetPrivateProfileStringW] Enumerated '%zu' sections of '%zu' characters ('%zu' bytes), is truncated: %d",
							   count,
							   sections.length(),
							   sections.length() * sizeof(wchar_t),
							   static_cast<int>(truncated)
				);
				redirector.Log(sections);

				StringCchCopyNW(lpReturnedString, nSize, sections.data(), sections.length());
				return truncated ? nSize - 2 : std::min<DWORD>(sections.length(), nSize);
			}

			// Enum all keys in section
			if (!keyName)
			{
				redirector.Log(L"[GetPrivateProfileStringW] Enum all keys in '%s' section of file '%s'", appName, lpFileName);

				size_t count = 0;
				bool truncated = false;
				KxDynamicStringW keys = ini.GetKeyNamesZSSTRZZ(appName, nSize, &truncated, &count);
				redirector.Log(L"[GetPrivateProfileStringW] Enumerated '%zu' keys of '%zu' characters ('%zu' bytes), is truncated: %d",
							   count,
							   keys.length(),
							   keys.length() * sizeof(wchar_t),
							   static_cast<int>(truncated)
				);
				redirector.Log(keys);

				StringCchCopyNW(lpReturnedString, nSize, keys.data(), keys.length());
				return truncated ? nSize - 2 : std::min<DWORD>(keys.length(), nSize);
			}

			if (auto value = ini.QueryValue(appName, keyName, defaultValue))
			{
				redirector.Log(L"[GetPrivateProfileStringW] Value found: '%s'", value->data());

				StringCchCopyNW(lpReturnedString, nSize, value->data(), value->length());
				return std::min<DWORD>(value->length(), nSize);
			}
			else
			{
				redirector.Log(L"[GetPrivateProfileStringW] Couldn't find requested data, returning default: '<null>'");

				StringCchCopyNW(lpReturnedString, nSize, L"", 1);
				return 0;
			}
		}

		SetLastError(ERROR_FILE_NOT_FOUND);
		return 0;
	}

	PPR_API(UINT) GetIntA(LPCSTR appName, LPCSTR keyName, INT defaultValue, LPCSTR lpFileName)
	{
		Redirector& redirector = Redirector::GetInstance();
		redirector.Log(L"[GetPrivateProfileIntA] Redirecting to 'GetPrivateProfileIntW'");

		KxDynamicStringW appNameW = redirector.ConvertFromACP(appName);
		KxDynamicStringW keyNameW = redirector.ConvertFromACP(keyName);
		KxDynamicStringW lpFileNameW = redirector.ConvertFromACP(lpFileName);

		return GetIntW(DataOrNull(appNameW),
					   DataOrNull(keyNameW),
					   defaultValue,
					   DataOrNull(lpFileNameW)
		);
	}
	PPR_API(UINT) GetIntW(LPCWSTR appName, LPCWSTR keyName, INT defaultValue, LPCWSTR lpFileName)
	{
		Redirector& redirector = Redirector::GetInstance();
		redirector.Log(L"[GetPrivateProfileIntW] Section: '%s', Key: '%s', Default: '%d', Path: '%s'", appName, keyName, defaultValue, lpFileName);

		if (!lpFileName)
		{
			::SetLastError(ERROR_FILE_NOT_FOUND);
			return defaultValue;
		}
		if (!appName || !keyName)
		{
			::SetLastError(ERROR_INVALID_PARAMETER);
			return defaultValue;
		}

		ConfigObject& configObject = redirector.GetOrLoadFile(lpFileName);
		auto lock = configObject.LockShared();

		if (auto value = configObject.GetINI().QueryValue(appName, keyName))
		{
			if (auto intValue = Utility::String::ToInteger(*value))
			{
				redirector.Log(L"[GetPrivateProfileIntW] ValueString: '%s', ValueInt: '%d'", value->data(), (int)*intValue);
				return *intValue;
			}
			else
			{
				redirector.Log(L"[GetPrivateProfileIntW] Couldn't convert string '%s' to integer value", value->data());
				return defaultValue;
			}
		}

		redirector.Log(L"[GetPrivateProfileIntW] Couldn't find requested data, returning default: '%d'", defaultValue);
		return defaultValue;
	}

	PPR_API(DWORD) GetSectionNamesA(LPSTR lpszReturnBuffer, DWORD nSize, LPCSTR lpFileName)
	{
		Redirector& redirector = Redirector::GetInstance();
		redirector.Log(L"[GetPrivateProfileSectionNamesA] Redirecting to 'GetPrivateProfileSectionNamesW'");

		if (lpszReturnBuffer == nullptr || nSize <= 2)
		{
			if (lpszReturnBuffer)
			{
				*lpszReturnBuffer = '\0';
			}
			return 0;
		}

		KxDynamicStringW lpFileNameW = redirector.ConvertFromACP(lpFileName);
		KxDynamicStringW lpszReturnBufferW;
		lpszReturnBufferW.resize(nSize);

		const DWORD length = GetSectionNamesW(lpszReturnBufferW.data(), nSize, DataOrNull(lpFileNameW));
		if (length <= nSize)
		{
			KxDynamicStringA result = redirector.ConvertToACP(lpszReturnBufferW.data(), length);
			StringCchCopyNA(lpszReturnBuffer, nSize, result.data(), result.length());
		}
		else
		{
			StringCchCopyNA(lpszReturnBuffer, nSize, "", 1);
		}
		return length;
	}
	PPR_API(DWORD) GetSectionNamesW(LPWSTR lpszReturnBuffer, DWORD nSize, LPCWSTR lpFileName)
	{
		Redirector& redirector = Redirector::GetInstance();
		redirector.Log(L"[GetPrivateProfileSectionNamesW]: Buffer size: '%u', Path: '%s'", nSize, lpFileName);

		if (lpszReturnBuffer == nullptr || nSize <= 2)
		{
			::SetLastError(ERROR_INSUFFICIENT_BUFFER);
			if (lpszReturnBuffer)
			{
				*lpszReturnBuffer = L'\0';
			}
			return 0;
		}
		if (!lpFileName)
		{
			::SetLastError(ERROR_FILE_NOT_FOUND);
			return 0;
		}

		ConfigObject& configObject = redirector.GetOrLoadFile(lpFileName);
		auto lock = configObject.LockShared();
		const INIWrapper& ini = configObject.GetINI();

		size_t count = 0;
		bool truncated = false;
		KxDynamicStringW sectionNames = ini.GetSectionNamesZSSTRZZ(nSize, &truncated, &count);
		redirector.Log(L"[GetPrivateProfileSectionNamesW] Enumerated '%zu' sections of '%zu' characters ('%zu' bytes), is truncated: %d",
					   count,
					   sectionNames.length(),
					   sectionNames.length() * sizeof(wchar_t),
					   static_cast<int>(truncated)
		);
		redirector.Log(sectionNames);

		if (!sectionNames.empty())
		{
			if (sectionNames.length() <= nSize)
			{
				CopyToBuffer(lpszReturnBuffer, sectionNames);
				return sectionNames.length() - 1;
			}
			else
			{
				redirector.Log(L"[GetPrivateProfileSectionNamesW]: Buffer is not large enough to contain all section names, '%zu' required, only '%u' available.", sectionNames.length(), nSize);

				StringCchCopyNW(lpszReturnBuffer, nSize, L"\0\0", 2);
				::SetLastError(ERROR_INSUFFICIENT_BUFFER);
				return nSize - 2;
			}
		}
		else
		{
			redirector.Log(L"No sections found", sectionNames.length());
			StringCchCopyNW(lpszReturnBuffer, nSize, L"\0\0", 2);
			return 0;
		}
	}

	PPR_API(DWORD) GetSectionA(LPCSTR appName, LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName)
	{
		Redirector& redirector = Redirector::GetInstance();
		redirector.Log(L"[GetPrivateProfileSectionA] Redirecting to 'GetPrivateProfileSectionW'");

		if (lpReturnedString == nullptr || nSize == 0)
		{
			return 0;
		}
		if (nSize == 1)
		{
			*lpReturnedString = '\0';
			return 0;
		}

		auto appNameW = redirector.ConvertFromACP(appName);
		auto lpFileNameW = redirector.ConvertFromACP(lpFileName);

		KxDynamicStringW lpReturnedStringW;
		lpReturnedStringW.resize(nSize);

		const DWORD length = GetSectionW(appNameW, lpReturnedStringW.data(), nSize, DataOrNull(lpFileNameW));
		if (length <= nSize)
		{
			KxDynamicStringA result = redirector.ConvertToACP(lpReturnedStringW.data(), length);
			CopyToBuffer(lpReturnedString, result);
		}
		else
		{
			StringCchCopyNA(lpReturnedString, nSize, "", 1);
		}
		return length;
	}
	PPR_API(DWORD) GetSectionW(LPCWSTR appName, LPWSTR lpReturnedString, DWORD nSize, LPCWSTR lpFileName)
	{
		Redirector& redirector = Redirector::GetInstance();
		redirector.Log(L"[GetPrivateProfileSectionW] Section: '%s', Buffer size: '%u', Path: '%s'", appName, nSize, lpFileName);

		if (lpReturnedString == nullptr || nSize <= 2)
		{
			::SetLastError(ERROR_INSUFFICIENT_BUFFER);
			if (lpReturnedString)
			{
				*lpReturnedString = L'\0';
			}
			return 0;
		}
		if (!lpFileName)
		{
			::SetLastError(ERROR_FILE_NOT_FOUND);
			return 0;
		}

		ConfigObject& configObject = redirector.GetOrLoadFile(lpFileName);
		auto lock = configObject.LockShared();
		const INIWrapper& ini = configObject.GetINI();

		size_t count = 0;
		bool truncated = false;
		KxDynamicStringW keyValuePairs;
		for (KxDynamicStringRefW keyName: ini.GetKeyNames(appName))
		{
			count++;
			KxDynamicStringRefW value = ini.GetValue(appName, keyName);

			keyValuePairs.append(keyName);
			keyValuePairs.append(1, L'=');
			keyValuePairs.append(value);
			keyValuePairs.append(1, L'\0');

			if (keyValuePairs.length() >= nSize)
			{
				truncated = true;
				break;
			}
		}

		if (count != 0)
		{
			keyValuePairs.append(1, L'\0');
		}
		else
		{
			keyValuePairs.append(2, L'\0');
		}

		redirector.Log(L"[GetPrivateProfileSectionW] Enumerated '%zu' key-value pairs of '%zu' characters ('%zu' bytes), is truncated: %d",
					   count,
					   keyValuePairs.length(),
					   keyValuePairs.length() * sizeof(wchar_t),
					   static_cast<int>(truncated)
		);
		redirector.Log(keyValuePairs);

		if (!truncated)
		{
			CopyToBuffer(lpReturnedString, keyValuePairs);
			return keyValuePairs.length() - 1;
		}
		else
		{
			redirector.Log(L"[GetPrivateProfileSectionW]: Buffer is not large enough to contain all key-value pairs, '%zu' required, only '%u' available.", keyValuePairs.length(), nSize);

			StringCchCopyNW(lpReturnedString, nSize, L"\0\0", 2);
			::SetLastError(ERROR_INSUFFICIENT_BUFFER);
			return nSize - 2;
		}
	}

	PPR_API(BOOL) WriteStringA(LPCSTR appName, LPCSTR keyName, LPCSTR lpString, LPCSTR lpFileName)
	{
		Redirector& redirector = Redirector::GetInstance();
		redirector.Log(L"[WritePrivateProfileStringA] Redirecting to 'WritePrivateProfileStringW'");

		KxDynamicStringW appNameW = redirector.ConvertFromACP(appName);
		KxDynamicStringW keyNameW = redirector.ConvertFromACP(keyName);
		KxDynamicStringW lpStringW = redirector.ConvertFromACP(lpString);
		KxDynamicStringW lpFileNameW = redirector.ConvertFromACP(lpFileName);

		return WriteStringW(DataOrNull(appNameW), DataOrNull(keyNameW), DataOrNull(lpStringW), DataOrNull(lpFileNameW));
	}
	PPR_API(BOOL) WriteStringW(LPCWSTR appName, LPCWSTR keyName, LPCWSTR lpString, LPCWSTR lpFileName)
	{
		Redirector& redirector = Redirector::GetInstance();
		redirector.Log(L"[WritePrivateProfileStringW] Section: '%s', Key: '%s', Value: '%s', Path: '%s'", appName, keyName, lpString, lpFileName);

		ConfigObject& configObject = Redirector::GetInstance().GetOrLoadFile(lpFileName);
		auto lock = configObject.LockExclusive();
		INIWrapper& ini = configObject.GetINI();

		// This function will write value to the in-memory file.
		// When 'NativeWrite' or 'WriteProtected' options are enabled, it will not flush updated file to the disk.
		auto WriteStringToMemoryFile = [&](LPCWSTR appName, LPCWSTR keyName, LPCWSTR lpString, LPCWSTR lpFileName)
		{
			if (!lpFileName)
			{
				::SetLastError(ERROR_FILE_NOT_FOUND);
				return false;
			}
			if (!appName)
			{
				::SetLastError(ERROR_INVALID_PARAMETER);
				return false;
			}

			// Delete section
			if (keyName == nullptr)
			{
				if (ini.DeleteSection(appName))
				{
					redirector.Log(L"[WritePrivateProfileStringW] Section '%s' deleted", appName);
					configObject.OnWrite();
					return true;
				}
				return false;
			}

			// Delete value
			if (lpString == nullptr)
			{
				if (ini.DeleteKey(appName, keyName))
				{
					redirector.Log(L"[WritePrivateProfileStringW] Key '%s' in section '%s' deleted", keyName, appName);
					configObject.OnWrite();
					return true;
				}
				return false;
			}

			// Set value
			if (ini.SetValue(appName, keyName, lpString))
			{
				redirector.Log(L"[WritePrivateProfileStringW] Assigned value '%s' to key '%s' in section '%s'", lpString, keyName, appName);
				configObject.OnWrite();
				return true;
			}
			return false;
		};
		const bool memoryWriteSuccess = WriteStringToMemoryFile(appName, keyName, lpString, lpFileName);

		// Call native function or proceed with our own implementation
		if (redirector.IsOptionEnabled(RedirectorOption::NativeWrite))
		{
			redirector.Log(L"[WritePrivateProfileStringW]: Calling native 'WritePrivateProfileStringW'");
			return redirector.GetFunctionTable().PrivateProfile.WriteStringW(appName, keyName, lpString, lpFileName);
		}
		return memoryWriteSuccess;
	}
}
