#pragma once
#include "Common.h"

namespace PPR::Internal
{
	struct PrivateProfileFunctionTable
	{
		DWORD(WINAPI* GetStringA)(LPCSTR, LPCSTR, LPCSTR, LPSTR, DWORD, LPCSTR) = nullptr;
		DWORD(WINAPI* GetStringW)(LPCWSTR, LPCWSTR, LPCWSTR, LPWSTR, DWORD, LPCWSTR) = nullptr;

		BOOL(WINAPI* WriteStringA)(LPCSTR, LPCSTR, LPCSTR, LPCSTR) = nullptr;
		BOOL(WINAPI* WriteStringW)(LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR) = nullptr;

		UINT(WINAPI* GetIntA)(LPCSTR, LPCSTR, INT, LPCSTR) = nullptr;
		UINT(WINAPI* GetIntW)(LPCWSTR, LPCWSTR, INT, LPCWSTR) = nullptr;

		DWORD(WINAPI* GetSectionNamesA)(LPSTR, DWORD, LPCSTR) = nullptr;
		DWORD(WINAPI* GetSectionNamesW)(LPWSTR, DWORD, LPCWSTR) = nullptr;

		DWORD(WINAPI* GetSectionA)(LPCSTR, LPSTR, DWORD, LPCSTR) = nullptr;
		DWORD(WINAPI* GetSectionW)(LPCWSTR, LPWSTR, DWORD, LPCWSTR) = nullptr;
	};
}

namespace PPR
{
	struct FunctionTable
	{
		Internal::PrivateProfileFunctionTable PrivateProfile;
	};
}
