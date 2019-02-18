#pragma once
#include "stdafx.h"
#include "Utility\SimpleINI.h"
#include "Utility\RtlDefines.h"
#include "Utility\KxDynamicString.h"
#include "Utility\KxCriticalSection.h"
#include "Utility\KxCallAtScopeExit.h"
#include "Utility\String.h"

using INIFile = CSimpleIniW;
class INIObject
{
	friend class PrivateProfileRedirector;

	private:
		INIFile m_INI;
		KxDynamicStringW m_Path;
		KxCriticalSection m_CS;
		bool m_IsChanged = false;
		bool m_ExistOnDisk = false;

	private:
		bool LoadFile();
		bool SaveFile();

		bool SkipByteOrderMark(FILE* stream) const;
		void ProcessInlineComments();

	public:
		INIObject(const KxDynamicStringW& path);

	public:
		const INIFile& GetFile() const
		{
			return m_INI;
		}
		INIFile& GetFile()
		{
			return m_INI;
		}
		KxDynamicStringW GetFilePath() const
		{
			return m_Path;
		}

		void OnWrite();
		bool IsExistOnDisk() const
		{
			return m_ExistOnDisk;
		}
		bool IsChanged() const
		{
			return m_IsChanged;
		}
		bool IsEmpty() const
		{
			return m_INI.IsEmpty();
		}

		KxCriticalSection& GetLock()
		{
			return m_CS;
		}
};

//////////////////////////////////////////////////////////////////////////
class PrivateProfileRedirector
{
	private:
		static PrivateProfileRedirector* ms_Instance;
		static const int ms_VersionMajor;
		static const int ms_VersionMinor;
		static const int ms_VersionPatch;

	public:
		static bool HasInstance()
		{
			return ms_Instance != nullptr;
		}
		static PrivateProfileRedirector& GetInstance()
		{
			return *ms_Instance;
		}
		static PrivateProfileRedirector* GetInstancePtr()
		{
			return ms_Instance;
		}
		static PrivateProfileRedirector& CreateInstance();
		static void DestroyInstance();
		
		static const wchar_t* GetLibraryNameW();
		static const wchar_t* GetLibraryVersionW();
		static const char* GetLibraryNameA();
		static const char* GetLibraryVersionA();

		static int GetLibraryVersionInt();

	public:
		DWORD(WINAPI* m_GetPrivateProfileStringA)(LPCSTR, LPCSTR, LPCSTR, LPSTR, DWORD, LPCSTR) = nullptr;
		DWORD(WINAPI* m_GetPrivateProfileStringW)(LPCWSTR, LPCWSTR, LPCWSTR, LPWSTR, DWORD, LPCWSTR) = nullptr;

		BOOL(WINAPI* m_WritePrivateProfileStringA)(LPCSTR, LPCSTR, LPCSTR, LPCSTR) = nullptr;
		BOOL(WINAPI* m_WritePrivateProfileStringW)(LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR) = nullptr;

		UINT(WINAPI* m_GetPrivateProfileIntA)(LPCSTR, LPCSTR, INT, LPCSTR) = nullptr;
		UINT(WINAPI* m_GetPrivateProfileIntW)(LPCWSTR, LPCWSTR, INT, LPCWSTR) = nullptr;

		DWORD(WINAPI* m_GetPrivateProfileSectionNamesA)(LPSTR, DWORD, LPCSTR) = nullptr;
		DWORD(WINAPI* m_GetPrivateProfileSectionNamesW)(LPWSTR, DWORD, LPCWSTR) = nullptr;

		DWORD(WINAPI* m_GetPrivateProfileSectionA)(LPCSTR, LPSTR, DWORD, LPCSTR) = nullptr;
		DWORD(WINAPI* m_GetPrivateProfileSectionW)(LPCWSTR, LPWSTR, DWORD, LPCWSTR) = nullptr;

		NTSTATUS(WINAPI* m_RtlInitUnicodeString)(PCUNICODE_STRING, PCWSTR) = nullptr;
		NTSTATUS(WINAPI* m_RtlUnicodeStringToInteger)(PCUNICODE_STRING, ULONG, PULONG) = nullptr;

	private:
		struct FunctionInfo
		{
			const wchar_t* Name = nullptr;
			void** Original = nullptr;
			void* Override = nullptr;

			FunctionInfo(void** original, void* override, const wchar_t* name)
				:Original(original), Override(override), Name(name)
			{
			}
			FunctionInfo(const wchar_t* name = L"")
				:Name(name)
			{
			}
		};

		const DWORD m_ThreadID = 0;
		HMODULE m_NtDLL = nullptr;

		INIFile m_Config;
		bool m_AllowSEVersionMismatch = false;
		bool m_WriteProtected = false;
		bool m_NativeWrite = false;
		bool m_ShouldSaveOnWrite = true;
		bool m_ShouldSaveOnThreadDetach = false;
		bool m_TrimKeyNamesA = true;
		bool m_TrimValueQuotes = true;
		bool m_ProcessInlineComments = true;
		bool m_SkipByteOrderMark = true;
		bool m_DisableCCUnsafeA = false;
		int m_ANSICodePage = CP_ACP;

		PPR::Utility::String::UnorderedMapWNoCase<std::unique_ptr<INIObject>> m_INIMap;
		KxCriticalSection m_INIMapCS;
		FILE* m_Log = nullptr;

	private:
		void LogBase(const wchar_t* string) const
		{
			fputws(string, m_Log);
			fputws(L"\r\n", m_Log);
			fflush(m_Log);
		}
		
		static LONG DetourAttachFunction(void** originalFunc, void* overrideFunc);
		static LONG DetourDetachFunction(void** originalFunc, void* overrideFunc);

		template<class T> LONG AttachFunction(T* originalFunc, T overrideFunc, const FunctionInfo& info)
		{
			LONG status = DetourAttachFunction(reinterpret_cast<void**>(originalFunc), reinterpret_cast<void*>(overrideFunc));
			LogAttachDetachStatus(status, L"AttachFunction", info);
			return status;
		}
		template<class T> LONG DetachFunction(T* originalFunc, T overrideFunc, const FunctionInfo& info)
		{
			LONG status = DetourDetachFunction(reinterpret_cast<void**>(originalFunc), reinterpret_cast<void*>(overrideFunc));
			LogAttachDetachStatus(status, L"DetachFunction", info);
			return status;
		}
		void LogAttachDetachStatus(LONG status, const wchar_t* operation, const FunctionInfo& info);

		void InitFunctions();
		void OverrideFunctions();
		void RestoreFunctions();

	private:
		const wchar_t* GetConfigOption(const wchar_t* section, const wchar_t* key, const wchar_t* defaultValue = nullptr) const;
		int GetConfigOptionInt(const wchar_t* section, const wchar_t* key, int defaultValue = 0) const;
		bool GetConfigOptionBool(const wchar_t* section, const wchar_t* key, bool defaultValue = false)
		{
			return GetConfigOptionInt(section, key, defaultValue);
		}

	public:
		PrivateProfileRedirector();
		~PrivateProfileRedirector();

	public:
		bool IsInitialThread(DWORD threadID) const
		{
			return m_ThreadID == threadID;
		}
		
		bool IsLogEnabled() const
		{
			return m_Log != nullptr;
		}
		bool IsSEVersionMismatchAllowed() const
		{
			return m_AllowSEVersionMismatch;
		}
		bool IsWriteProtected() const
		{
			return m_WriteProtected;
		}
		bool IsNativeWrite() const
		{
			return m_NativeWrite;
		}
		bool ShouldSaveOnWrite() const
		{
			return m_ShouldSaveOnWrite;
		}
		bool ShouldSaveOnThreadDetach() const
		{
			return m_ShouldSaveOnThreadDetach;
		}
		bool ShouldSaveOnProcessDetach() const
		{
			return !m_NativeWrite;
		}
		bool ShouldTrimKeyNamesA() const
		{
			return m_TrimKeyNamesA;
		}
		bool ShouldTrimValueQuotes() const
		{
			return m_TrimValueQuotes;
		}
		bool ShouldProcessInlineComments() const
		{
			return m_ProcessInlineComments;
		}
		bool ShouldSkipByteOrderMark() const
		{
			return m_SkipByteOrderMark;
		}
		bool ShouldDisableCCUnsafeA() const
		{
			return m_DisableCCUnsafeA;
		}

		INIObject& GetOrLoadFile(KxDynamicStringRefW path);
		void SaveChnagedFiles(const wchar_t* message) const;
		size_t RefreshINI();
		
		KxDynamicStringW ConvertToUTF16(const char* string, int length = -1) const
		{
			return KxDynamicStringW::to_utf16(string, length, m_ANSICodePage);
		}
		KxDynamicStringA ConvertToCodePage(const wchar_t* string, int length = -1) const
		{
			return KxDynamicStringW::to_codepage(string, length, m_ANSICodePage);
		}
		
		static KxDynamicStringW& TrimCharsL(KxDynamicStringW& value, KxDynamicStringW::TChar c1, KxDynamicStringW::TChar c2);
		static KxDynamicStringW& TrimCharsR(KxDynamicStringW& value, KxDynamicStringW::TChar c1, KxDynamicStringW::TChar c2);
		static KxDynamicStringW& TrimCharsLR(KxDynamicStringW& value, KxDynamicStringW::TChar c1, KxDynamicStringW::TChar c2)
		{
			TrimCharsL(value, c1, c2);
			TrimCharsR(value, c1, c2);
			return value;
		}
		static KxDynamicStringW& TrimSpaceCharsLR(KxDynamicStringW& value)
		{
			return TrimCharsLR(value, L' ', L'\t');
		}
		static KxDynamicStringW& TrimQuoteCharsLR(KxDynamicStringW& value)
		{
			return TrimCharsLR(value, L'\"', L'\'');
		}
		
		template<class ...Args> void Log(const wchar_t* format, Args... args) const
		{
			if (m_Log)
			{
				KxDynamicStringW string = KxDynamicStringW::Format(format, std::forward<Args>(args)...);
				LogBase(string.data());
			}
		}
};

//////////////////////////////////////////////////////////////////////////
#define PPR_API(retType) extern "C" __declspec(dllexport, noinline) retType WINAPI

PPR_API(DWORD) On_GetPrivateProfileStringA(LPCSTR appName, LPCSTR keyName, LPCSTR defaultValue, LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName);
PPR_API(DWORD) On_GetPrivateProfileStringW(LPCWSTR appName, LPCWSTR keyName, LPCWSTR defaultValue, LPWSTR lpReturnedString, DWORD nSize, LPCWSTR lpFileName);

PPR_API(UINT) On_GetPrivateProfileIntA(LPCSTR appName, LPCSTR keyName, INT defaultValue, LPCSTR lpFileName);
PPR_API(UINT) On_GetPrivateProfileIntW(LPCWSTR appName, LPCWSTR keyName, INT defaultValue, LPCWSTR lpFileName);

PPR_API(DWORD) On_GetPrivateProfileSectionNamesA(LPSTR lpszReturnBuffer, DWORD nSize, LPCSTR lpFileName);
PPR_API(DWORD) On_GetPrivateProfileSectionNamesW(LPWSTR lpszReturnBuffer, DWORD nSize, LPCWSTR lpFileName);

PPR_API(DWORD) On_GetPrivateProfileSectionA(LPCSTR appName, LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName);
PPR_API(DWORD) On_GetPrivateProfileSectionW(LPCWSTR appName, LPWSTR lpReturnedString, DWORD nSize, LPCWSTR lpFileName);

PPR_API(BOOL) On_WritePrivateProfileStringA(LPCSTR appName, LPCSTR keyName, LPCSTR lpString, LPCSTR lpFileName);
PPR_API(BOOL) On_WritePrivateProfileStringW(LPCWSTR appName, LPCWSTR keyName, LPCWSTR lpString, LPCWSTR lpFileName);
