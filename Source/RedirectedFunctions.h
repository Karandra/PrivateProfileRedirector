#pragma once
#include "stdafx.h"

#define PPR_API(retType) __declspec(noinline) retType WINAPI

namespace PPR::PrivateProfile
{
	PPR_API(DWORD) GetStringA(LPCSTR appName, LPCSTR keyName, LPCSTR defaultValue, LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName);
	PPR_API(DWORD) GetStringW(LPCWSTR appName, LPCWSTR keyName, LPCWSTR defaultValue, LPWSTR lpReturnedString, DWORD nSize, LPCWSTR lpFileName);

	PPR_API(UINT) GetIntA(LPCSTR appName, LPCSTR keyName, INT defaultValue, LPCSTR lpFileName);
	PPR_API(UINT) GetIntW(LPCWSTR appName, LPCWSTR keyName, INT defaultValue, LPCWSTR lpFileName);

	PPR_API(DWORD) GetSectionNamesA(LPSTR lpszReturnBuffer, DWORD nSize, LPCSTR lpFileName);
	PPR_API(DWORD) GetSectionNamesW(LPWSTR lpszReturnBuffer, DWORD nSize, LPCWSTR lpFileName);

	PPR_API(DWORD) GetSectionA(LPCSTR appName, LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName);
	PPR_API(DWORD) GetSectionW(LPCWSTR appName, LPWSTR lpReturnedString, DWORD nSize, LPCWSTR lpFileName);

	PPR_API(BOOL) WriteStringA(LPCSTR appName, LPCSTR keyName, LPCSTR lpString, LPCSTR lpFileName);
	PPR_API(BOOL) WriteStringW(LPCWSTR appName, LPCWSTR keyName, LPCWSTR lpString, LPCWSTR lpFileName);
}
