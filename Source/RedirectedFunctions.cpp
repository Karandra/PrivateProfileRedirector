#include "stdafx.h"
#include "RedirectedFunctions.h"
#include "PrivateProfileRedirector.h"
#include <kxf/Log/Categories.h>
#include <kxf/System/Win32Error.h>
#include <strsafe.h>

#undef PPR_API
#define PPR_API(retType) retType WINAPI

namespace
{
	namespace LogCategory
	{
		KX_DefineLogCategory(GetPrivateProfileStringA);
		KX_DefineLogCategory(GetPrivateProfileStringW);
		KX_DefineLogCategory(GetPrivateProfileIntA);
		KX_DefineLogCategory(GetPrivateProfileIntW);
		KX_DefineLogCategory(GetPrivateProfileSectionNamesA);
		KX_DefineLogCategory(GetPrivateProfileSectionNamesW);
		KX_DefineLogCategory(GetPrivateProfileSectionA);
		KX_DefineLogCategory(GetPrivateProfileSectionW);
		KX_DefineLogCategory(WritePrivateProfileStringA);
		KX_DefineLogCategory(WritePrivateProfileStringW);
	}

	template<class TChar>
	HRESULT StringCopyBuffer(TChar* dst, size_t dstSize, const TChar* src, size_t srcLength) noexcept
	{
		if (dst && src)
		{
			size_t toCopySize = std::min(dstSize, srcLength);
			if (toCopySize == 0)
			{
				return STRSAFE_E_INSUFFICIENT_BUFFER;
			}

			std::memcpy(dst, src, toCopySize * sizeof(TChar));
			if (dstSize > srcLength)
			{
				// Null-terminate at the position past copied data
				dst[srcLength] = 0;
				return S_OK;
			}
			else
			{
				// Replace the last copied character with the null-terminator
				dst[toCopySize - 1] = 0;
				return STRSAFE_E_INSUFFICIENT_BUFFER;
			}
		}
		return STRSAFE_E_INVALID_PARAMETER;
	}
}

namespace PPR::PrivateProfile
{
	template<class TChar>
	DWORD GetStringT(kxf::StringView logCategory, const TChar* appName, const TChar* keyName, const TChar* defaultValue, TChar* lpReturnedString, DWORD nSize, const TChar* lpFileName)
	{
		KX_SCOPEDLOG_AUTO;
		KX_SCOPEDLOG.Trace(logCategory).Format("Section: '{}', Key: '{}', Default: '{}', Buffer size: '{}', Path: '{}'", appName, keyName, defaultValue, nSize, lpFileName);

		if (!lpFileName)
		{
			::SetLastError(ERROR_FILE_NOT_FOUND);
			return 0;
		}
		if (!lpReturnedString || nSize < 2)
		{
			::SetLastError(ERROR_INSUFFICIENT_BUFFER);
			return 0;
		}

		Redirector& redirector = Redirector::GetInstance();
		kxf::IEncodingConverter& converter = redirector.GetEncodingConverter();
		ConfigObject& configObject = redirector.GetOrLoadFile(INIWrapper::EncodingTo(lpFileName, converter));
		auto lock = configObject.LockShared();
		const INIWrapper& ini = configObject.GetINI();

		// Enum all sections
		if (!appName)
		{
			KX_SCOPEDLOG.Trace(logCategory).Format("Enum all sections of file '{}'", lpFileName);

			size_t count = 0;
			bool truncated = false;
			auto sections = ini.GetSectionNamesZSSTRZZ<TChar>(converter, nSize, &truncated, &count);
			KX_SCOPEDLOG.Trace(logCategory).Format("Enumerated {} sections of {} characters ({} bytes), is truncated: {}",
												   count,
												   sections.length(),
												   sections.length() * sizeof(TChar),
												   truncated
			);
			KX_SCOPEDLOG.Trace(logCategory).Format("Sections: '{}'", sections);

			HRESULT hr = StringCopyBuffer(lpReturnedString, nSize, sections.data(), sections.length());
			if (hr == STRSAFE_E_INSUFFICIENT_BUFFER)
			{
				KX_SCOPEDLOG.Warning(logCategory).Log("STRSAFE_E_INSUFFICIENT_BUFFER");
				return nSize - 2;
			}
			else if (truncated)
			{
				return nSize - 2;
			}
			return sections.length() - 1;
		}

		// Enum all keys in the section
		if (!keyName)
		{
			KX_SCOPEDLOG.Trace(logCategory).Format("Enum all keys in '{}' section of file '{}'", keyName, lpFileName);

			size_t count = 0;
			bool truncated = false;
			auto keys = ini.GetKeyNamesZSSTRZZ<TChar>(converter, INIWrapper::EncodingTo(appName, converter), nSize, &truncated, &count);
			KX_SCOPEDLOG.Trace(logCategory).Format("Enumerated {} keys of {} characters ({} bytes), is truncated: {}",
												   count,
												   keys.length(),
												   keys.length() * sizeof(TChar),
												   truncated
			);
			KX_SCOPEDLOG.Trace(logCategory).Format("Keys: '{}'", keys);

			HRESULT hr = StringCopyBuffer(lpReturnedString, nSize, keys.data(), keys.length());
			if (hr == STRSAFE_E_INSUFFICIENT_BUFFER)
			{
				KX_SCOPEDLOG.Warning(logCategory).Log("STRSAFE_E_INSUFFICIENT_BUFFER");
				return nSize - 2;
			}
			else if (truncated)
			{
				return nSize - 2;
			}
			return keys.length() - 1;
		}

		// Get the value
		if (auto value = ini.QueryValue(INIWrapper::EncodingTo(appName, converter), INIWrapper::EncodingTo(keyName, converter)))
		{
			KX_SCOPEDLOG.Trace(logCategory).Format("Value found: '{}'", *value);

			auto valueRef = INIWrapper::EncodingFrom<TChar>(*value, converter);
			HRESULT hr = StringCopyBuffer(lpReturnedString, nSize, valueRef.data(), valueRef.length());
			if (hr == STRSAFE_E_INSUFFICIENT_BUFFER)
			{
				KX_SCOPEDLOG.Trace(logCategory).Log("STRSAFE_E_INSUFFICIENT_BUFFER");
				return nSize - 1;
			}
			return valueRef.length();
		}
		else if (defaultValue)
		{
			KX_SCOPEDLOG.Trace(logCategory).Format("Couldn't find the requested data, returning default: '{}'", defaultValue);

			auto length = std::char_traits<TChar>::length(defaultValue);
			HRESULT hr = StringCopyBuffer(lpReturnedString, nSize, defaultValue, length);
			if (hr == STRSAFE_E_INSUFFICIENT_BUFFER)
			{
				return nSize - 1;
			}
			return length;
		}
		else
		{
			KX_SCOPEDLOG.Trace(logCategory).Format("Couldn't find the requested data, returning empty string", defaultValue);

			TChar c = 0;
			StringCopyBuffer(lpReturnedString, nSize, &c, 1);
			return 0;
		}
	}

	template<class TChar>
	UINT GetIntT(kxf::StringView logCategory, const TChar* appName, const TChar* keyName, INT defaultValue, const TChar* lpFileName)
	{
		KX_SCOPEDLOG_AUTO;
		KX_SCOPEDLOG.Trace(logCategory).Format("Section: '{}', Key: '{}', Default: '{}', Path: '{}'", appName, keyName, defaultValue, lpFileName);

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

		Redirector& redirector = Redirector::GetInstance();
		kxf::IEncodingConverter& converter = redirector.GetEncodingConverter();

		ConfigObject& configObject = redirector.GetOrLoadFile(INIWrapper::EncodingTo(lpFileName, converter));
		auto lock = configObject.LockShared();

		if (auto value = configObject.GetINI().QueryValue(INIWrapper::EncodingTo(appName, converter), INIWrapper::EncodingTo(keyName, converter)))
		{
			if (auto intValue = value->ToInteger<INT>(-1))
			{
				KX_SCOPEDLOG.Trace(logCategory).Format("String '{}' converted to an integer: '{}'", *value, *intValue);
				return *intValue;
			}
			else
			{
				KX_SCOPEDLOG.Trace(logCategory).Format("Couldn't convert string '{}' to an integer, returning default: {}", *value, defaultValue);
				return defaultValue;
			}
		}

		KX_SCOPEDLOG.Trace(logCategory).Format("Couldn't find the requested data, returning default: '{}'", defaultValue);
		return defaultValue;
	}

	template<class TChar>
	DWORD GetSectionNamesT(kxf::StringView logCategory, TChar* lpszReturnBuffer, DWORD nSize, const TChar* lpFileName)
	{
		return GetStringT<TChar>(logCategory, nullptr, nullptr, nullptr, lpszReturnBuffer, nSize, lpFileName);
	}

	template<class TChar>
	DWORD GetSectionT(kxf::StringView logCategory, const TChar* appName, TChar* lpReturnedString, DWORD nSize, const TChar* lpFileName)
	{
		KX_SCOPEDLOG_AUTO;
		KX_SCOPEDLOG.Trace(logCategory).Format("Section: '{}', Buffer size: '{}', Path: '{}'", appName, nSize, lpFileName);

		if (!lpFileName)
		{
			::SetLastError(ERROR_FILE_NOT_FOUND);
			return 0;
		}
		if (!appName)
		{
			::SetLastError(ERROR_INVALID_PARAMETER);
			return 0;
		}
		if (!lpReturnedString || nSize < 2)
		{
			::SetLastError(ERROR_INSUFFICIENT_BUFFER);
			return 0;
		}

		Redirector& redirector = Redirector::GetInstance();
		kxf::IEncodingConverter& converter = redirector.GetEncodingConverter();

		ConfigObject& configObject = redirector.GetOrLoadFile(INIWrapper::EncodingTo(lpFileName, converter));
		auto lock = configObject.LockShared();
		const INIWrapper& ini = configObject.GetINI();

		KX_SCOPEDLOG.Trace(logCategory).Format("Enum all key-value from section '{}' of file '{}'", appName, lpFileName);

		size_t count = 0;
		bool truncated = false;
		auto sectionName = INIWrapper::EncodingTo(appName, converter);
		auto keyValuePairs = INIWrapper::CreateZSSTRZZ([&](std::basic_string<TChar>& buffer, const kxf::String& keyName)
		{
			if (auto value = ini.QueryValue(sectionName, keyName))
			{
				buffer.append(INIWrapper::EncodingFrom<TChar>(keyName, converter));
				buffer.append(1, '=');
				buffer.append(INIWrapper::EncodingFrom<TChar>(*value, converter));

				return kxf::CallbackCommand::Continue;
			}
			return kxf::CallbackCommand::Discard;
		}, ini.GetKeyNames(sectionName), nSize, &truncated, &count);

		KX_SCOPEDLOG.Trace(logCategory).Format("Enumerated {} key-value pairs of {} characters ({} bytes), is truncated: {}",
											   count,
											   keyValuePairs.length(),
											   keyValuePairs.length() * sizeof(TChar),
											   truncated
		);
		KX_SCOPEDLOG.Trace(logCategory).Format("Key-value pairs: '{}'", keyValuePairs);

		HRESULT hr = StringCopyBuffer(lpReturnedString, nSize, keyValuePairs.data(), keyValuePairs.length());
		if (hr == STRSAFE_E_INSUFFICIENT_BUFFER)
		{
			KX_SCOPEDLOG.Warning(logCategory).Log("STRSAFE_E_INSUFFICIENT_BUFFER");
			return nSize - 2;
		}
		else if (truncated)
		{
			return nSize - 2;
		}
		return keyValuePairs.length() - 1;
	}

	template<class TChar>
	BOOL WriteStringT(kxf::StringView logCategory, const TChar* appName, const TChar* keyName, const TChar* lpString, const TChar* lpFileName)
	{
		KX_SCOPEDLOG_AUTO;
		KX_SCOPEDLOG.Trace(logCategory).Format("Section: '{}', Key: '{}', Value: '{}', Path: '{}'", appName, keyName, lpString, lpFileName);

		Redirector& redirector = Redirector::GetInstance();

		// When 'NativeWrite' or 'WriteProtected' options are enabled, it will not flush updated file to the disk.
		auto WriteStringToMemoryFile = [&](const TChar* appName, const TChar* keyName, const TChar* lpString, const TChar* lpFileName)
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

			kxf::IEncodingConverter& converter = redirector.GetEncodingConverter();
			ConfigObject& configObject = redirector.GetOrLoadFile(INIWrapper::EncodingTo(lpFileName, converter));
			auto lock = configObject.LockExclusive();
			INIWrapper& ini = configObject.GetINI();

			// Delete section
			if (!keyName)
			{
				if (ini.DeleteSection(INIWrapper::EncodingTo(appName, converter)))
				{
					KX_SCOPEDLOG.Trace(logCategory).Format("Section '{}' deleted", appName);
					configObject.OnWrite();

					return true;
				}
				return false;
			}

			// Delete value
			if (!lpString)
			{
				if (ini.DeleteKey(INIWrapper::EncodingTo(appName, converter), INIWrapper::EncodingTo(keyName, converter)))
				{
					KX_SCOPEDLOG.Trace(logCategory).Format("Key '{}' in section '{}' deleted", keyName, appName);
					configObject.OnWrite();

					return true;
				}
				return false;
			}

			// Set value
			if (ini.SetValue(appName, keyName, lpString))
			{
				KX_SCOPEDLOG.Trace(logCategory).Format("Assigned value '{}' to key '{}' in section '{}'", lpString, keyName, appName);
				configObject.OnWrite();

				return true;
			}
			return false;
		};
		bool memoryWriteSuccess = WriteStringToMemoryFile(appName, keyName, lpString, lpFileName);

		if (redirector.IsOptionEnabled(RedirectorOption::NativeWrite))
		{
			if constexpr(std::is_same_v<TChar, char>)
			{
				KX_SCOPEDLOG.Trace(logCategory).Format("Calling native 'WritePrivateProfileStringA'");
				return redirector.GetFunctionTable().PrivateProfile.WriteStringA(appName, keyName, lpString, lpFileName);
			}
			else if constexpr(std::is_same_v<TChar, wchar_t>)
			{
				KX_SCOPEDLOG.Trace(logCategory).Format("Calling native 'WritePrivateProfileStringW'");
				return redirector.GetFunctionTable().PrivateProfile.WriteStringW(appName, keyName, lpString, lpFileName);
			}
		}
		return memoryWriteSuccess ? TRUE : FALSE;
	}

	PPR_API(DWORD) GetStringA(LPCSTR appName, LPCSTR keyName, LPCSTR defaultValue, LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName)
	{
		return GetStringT(LogCategory::GetPrivateProfileStringA, appName, keyName, defaultValue, lpReturnedString, nSize, lpFileName);
	}
	PPR_API(DWORD) GetStringW(LPCWSTR appName, LPCWSTR keyName, LPCWSTR defaultValue, LPWSTR lpReturnedString, DWORD nSize, LPCWSTR lpFileName)
	{
		return GetStringT(LogCategory::GetPrivateProfileStringA, appName, keyName, defaultValue, lpReturnedString, nSize, lpFileName);
	}

	PPR_API(UINT) GetIntA(LPCSTR appName, LPCSTR keyName, INT defaultValue, LPCSTR lpFileName)
	{
		return GetIntT(LogCategory::GetPrivateProfileIntA, appName, keyName, defaultValue, lpFileName);
	}
	PPR_API(UINT) GetIntW(LPCWSTR appName, LPCWSTR keyName, INT defaultValue, LPCWSTR lpFileName)
	{
		return GetIntT(LogCategory::GetPrivateProfileIntW, appName, keyName, defaultValue, lpFileName);
	}

	PPR_API(DWORD) GetSectionNamesA(LPSTR lpszReturnBuffer, DWORD nSize, LPCSTR lpFileName)
	{
		return GetSectionNamesT(LogCategory::GetPrivateProfileSectionNamesA, lpszReturnBuffer, nSize, lpFileName);
	}
	PPR_API(DWORD) GetSectionNamesW(LPWSTR lpszReturnBuffer, DWORD nSize, LPCWSTR lpFileName)
	{
		return GetSectionNamesT(LogCategory::GetPrivateProfileSectionNamesW, lpszReturnBuffer, nSize, lpFileName);
	}

	PPR_API(DWORD) GetSectionA(LPCSTR appName, LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName)
	{
		return GetSectionNamesT(LogCategory::GetPrivateProfileSectionA, lpReturnedString, nSize, lpFileName);
	}
	PPR_API(DWORD) GetSectionW(LPCWSTR appName, LPWSTR lpReturnedString, DWORD nSize, LPCWSTR lpFileName)
	{
		return GetSectionNamesT(LogCategory::GetPrivateProfileSectionW, lpReturnedString, nSize, lpFileName);
	}

	PPR_API(BOOL) WriteStringA(LPCSTR appName, LPCSTR keyName, LPCSTR lpString, LPCSTR lpFileName)
	{
		return WriteStringT(LogCategory::WritePrivateProfileStringW, appName, keyName, lpString, lpFileName);
	}
	PPR_API(BOOL) WriteStringW(LPCWSTR appName, LPCWSTR keyName, LPCWSTR lpString, LPCWSTR lpFileName)
	{
		return WriteStringT(LogCategory::WritePrivateProfileStringW, appName, keyName, lpString, lpFileName);
	}
}
