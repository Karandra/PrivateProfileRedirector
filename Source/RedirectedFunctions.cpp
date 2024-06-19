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
	kxf::String MemoryToHex(const TChar* src, size_t srcSize1, size_t srcSize2 = 0)
	{
		kxf::String buffer;
		if (srcSize1 + srcSize2 != 0)
		{
			buffer.reserve(8 + (srcSize1 + srcSize2) * 3);

			auto ptr = reinterpret_cast<const uint8_t*>(src);
			for (size_t i = 0; i < srcSize1 + srcSize2; i++)
			{
				if (i == srcSize1)
				{
					buffer += "| ";
				}
				buffer.Format("{:02x} ", ptr[i]);
			}
			buffer.TrimRight();
		}
		return buffer;
	}

	template<class TChar>
	HRESULT StringLength(size_t& length, const TChar* src, size_t srcMaxSize) noexcept
	{
		length = 0;

		if (src)
		{
			if constexpr(std::is_same_v<TChar, char>)
			{
				return ::StringCchLengthA(src, srcMaxSize, &length);
			}
			else if constexpr(std::is_same_v<TChar, wchar_t>)
			{
				return ::StringCchLengthW(src, srcMaxSize, &length);
			}
			else
			{
				static_assert(false);
			}
		}
		return STRSAFE_E_INVALID_PARAMETER;
	}

	template<class TChar>
	HRESULT StringCopyBuffer(TChar* dst, size_t dstSize, const TChar* src, size_t srcSize, size_t* copiedSize = nullptr) noexcept
	{
		kxf::Utility::SetIfNotNull(copiedSize, 0);

		if (dst && src)
		{
			// Zero out the dst buffer first
			std::memset(dst, 0, dstSize * sizeof(TChar));

			// See how much we need to copy
			size_t copySize = std::min(dstSize, srcSize);
			kxf::Utility::ScopeGuard atExit = [&]()
			{
				if (kxf::Log::IsLevelEnabled(kxf::LogLevel::Trace))
				{
					// Make the function print a few bytes after the src length if the dst has the space for it
					size_t padding = 0;
					if (dstSize > copySize)
					{
						padding++;
					}
					if (dstSize > copySize + 1)
					{
						padding++;
					}

					auto dstHex = MemoryToHex(dst, copySize * sizeof(TChar), padding);
					kxf::Log::TraceCategory("StringCopyBuffer", "srcSize: {}, dstSize: {}, copySize: {} ({} bytes), dst contents: [{}]",
											srcSize,
											dstSize,
											copySize,
											copySize * sizeof(TChar),
											dstHex
					);
				}
			};

			// We can have zero characters to copy either because the dst buffer size is zero
			// or because the src buffer itself is zero sized. In this case we can still return
			// success. The dst buffer is zeroed out anyway, whatever size it is.
			if (copySize == 0)
			{
				kxf::Utility::SetIfNotNull(copiedSize, 0);
				return S_OK;
			}

			// Copy the data to dst
			std::memcpy(dst, src, copySize * sizeof(TChar));
			kxf::Utility::SetIfNotNull(copiedSize, copySize);

			if (dstSize > srcSize)
			{
				// Null-terminate at the position past copied data
				dst[srcSize] = 0;
				return S_OK;
			}
			else if (dstSize == srcSize && dst[dstSize - 1] == 0)
			{
				// There's no need to null-terminate anything as the copied data
				// is already null-terminated. This way we can return S_OK here
				// instead of STRSAFE_E_INSUFFICIENT_BUFFER
				return S_OK;
			}
			else
			{
				// Replace the last copied character with the null-terminator
				dst[copySize - 1] = 0;
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

			size_t copiedSize = 0;
			HRESULT hr = StringCopyBuffer(lpReturnedString, nSize, sections.data(), sections.length(), &copiedSize);
			DWORD result = sections.length() - 1;

			if (hr == STRSAFE_E_INSUFFICIENT_BUFFER)
			{
				KX_SCOPEDLOG.Warning(logCategory).Log("STRSAFE_E_INSUFFICIENT_BUFFER");
				result = nSize - 2;
			}
			else if (truncated)
			{
				result = nSize - 2;
			}
			
			KX_SCOPEDLOG.Trace(logCategory).Format("Result: {}, copied: {}, sections: '{}'", result, copiedSize, sections);
			return result;
		}

		// Enum all keys in the section
		if (!keyName)
		{
			KX_SCOPEDLOG.Trace(logCategory).Format("Enum all keys in '{}' section of file '{}'", appName, lpFileName);

			size_t count = 0;
			bool truncated = false;
			auto keys = ini.GetKeyNamesZSSTRZZ<TChar>(converter, INIWrapper::EncodingTo(appName, converter), nSize, &truncated, &count);
			KX_SCOPEDLOG.Trace(logCategory).Format("Enumerated {} keys of {} characters ({} bytes), is truncated: {}",
												   count,
												   keys.length(),
												   keys.length() * sizeof(TChar),
												   truncated
			);

			size_t copiedSize = 0;
			HRESULT hr = StringCopyBuffer(lpReturnedString, nSize, keys.data(), keys.length(), &copiedSize);
			DWORD result = keys.length() - 1;
			if (hr == STRSAFE_E_INSUFFICIENT_BUFFER)
			{
				KX_SCOPEDLOG.Warning(logCategory).Log("STRSAFE_E_INSUFFICIENT_BUFFER");
				result = nSize - 2;
			}
			else if (truncated)
			{
				result = nSize - 2;
			}

			KX_SCOPEDLOG.Trace(logCategory).Format("Result: {}, copied: {}, keys: '{}'", result, copiedSize, keys);
			return result;
		}

		// Get the value
		if (auto value = ini.QueryValue(INIWrapper::EncodingTo(appName, converter), INIWrapper::EncodingTo(keyName, converter)))
		{
			auto valueRef = INIWrapper::EncodingFrom<TChar>(*value, converter);

			size_t copiedSize = 0;
			HRESULT hr = StringCopyBuffer(lpReturnedString, nSize, valueRef.data(), valueRef.length(), &copiedSize);
			DWORD result = valueRef.length();
			if (hr == STRSAFE_E_INSUFFICIENT_BUFFER)
			{
				KX_SCOPEDLOG.Trace(logCategory).Log("STRSAFE_E_INSUFFICIENT_BUFFER");
				result = nSize - 1;
			}

			KX_SCOPEDLOG.Trace(logCategory).Format("Value found: '{}', result: {}, copied: {}", *value, result, copiedSize);
			return result;
		}
		else if (defaultValue)
		{
			size_t length = 0;
			StringLength(length, defaultValue, nSize);

			size_t copiedSize = 0;
			HRESULT hr = StringCopyBuffer(lpReturnedString, nSize, defaultValue, length, &copiedSize);
			DWORD result = length;
			if (hr == STRSAFE_E_INSUFFICIENT_BUFFER)
			{
				KX_SCOPEDLOG.Trace(logCategory).Log("STRSAFE_E_INSUFFICIENT_BUFFER");
				result = nSize - 1;
			}

			KX_SCOPEDLOG.Trace(logCategory).Format("Couldn't find the requested data, returning default: '{}', result: {}, copied: {}", defaultValue, result, copiedSize);
			return result;
		}
		else
		{
			TChar c = 0;
			size_t copiedSize = 0;
			StringCopyBuffer(lpReturnedString, nSize, &c, 1, &copiedSize);
			DWORD result = 0;

			KX_SCOPEDLOG.Trace(logCategory).Format("Couldn't find the requested data, returning empty string, result: {}, copied: {}", result, copiedSize);
			return result;
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
		auto keyValuePairs = INIWrapper::CreateZSSTRZZ<TChar>([&](std::basic_string<TChar>& buffer, const kxf::String& keyName)
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

		size_t copiedSize = 0;
		HRESULT hr = StringCopyBuffer(lpReturnedString, nSize, keyValuePairs.data(), keyValuePairs.length(), &copiedSize);
		DWORD result = keyValuePairs.length() - 1;
		if (hr == STRSAFE_E_INSUFFICIENT_BUFFER)
		{
			KX_SCOPEDLOG.Warning(logCategory).Log("STRSAFE_E_INSUFFICIENT_BUFFER");
			result = nSize - 2;
		}
		else if (truncated)
		{
			result = nSize - 2;
		}

		KX_SCOPEDLOG.Trace(logCategory).Format("Result: {}, copied: {}, key-value pairs: '{}'", result, copiedSize, keyValuePairs);
		return result;
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
			bool isSameData = false;
			if (ini.SetValue(appName, keyName, INIWrapper::EncodingTo(lpString, converter), &isSameData))
			{
				if (isSameData)
				{
					KX_SCOPEDLOG.Trace(logCategory).Format("Attempt to assign already existing value '{}' to key '{}' in section '{}', write request ignored", lpString, keyName, appName);
				}
				else
				{
					KX_SCOPEDLOG.Trace(logCategory).Format("Assigned value '{}' to key '{}' in section '{}'", lpString, keyName, appName);
					configObject.OnWrite();
				}
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
		return GetSectionT(LogCategory::GetPrivateProfileSectionA, appName, lpReturnedString, nSize, lpFileName);
	}
	PPR_API(DWORD) GetSectionW(LPCWSTR appName, LPWSTR lpReturnedString, DWORD nSize, LPCWSTR lpFileName)
	{
		return GetSectionT(LogCategory::GetPrivateProfileSectionW, appName, lpReturnedString, nSize, lpFileName);
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
