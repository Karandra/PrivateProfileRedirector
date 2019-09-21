#pragma once
#include "stdafx.h"
#include "Utility\KxOptionSet.h"
#include "Utility\KxDynamicString.h"
#include "Utility\KxCallAtScopeExit.h"
#include "Utility\SRWLock.h"
#include "Utility\String.h"
#include "INIWrapper.h"
#include "RedirectorConfig.h"
#include "FunctionRedirector.h"
#include "FunctionTable.h"
#include "ConfigObject.h"

namespace PPR
{
	class Redirector final
	{
		friend class RedirectorConfigLoader;

		public:
			static bool HasInstance();
			static Redirector& GetInstance();
			static Redirector* GetInstancePtr();
			static Redirector& CreateInstance();
			static void DestroyInstance();
			
			static const char* GetLibraryNameA();
			static const wchar_t* GetLibraryNameW();

			static const char* GetLibraryVersionA();
			static const wchar_t* GetLibraryVersionW();
			static int GetLibraryVersionInt();

			static bool DllMain(HMODULE module, DWORD event);

		private:
			FunctionTable m_Functions;
			const DWORD m_InitialThreadID = 0;

			TRedirectorOptionSet m_Options;
			int m_ANSICodePage = CP_ACP;

			Utility::String::UnorderedMapWNoCase<std::unique_ptr<ConfigObject>> m_INIMap;
			SRWLock m_INIMapLock;
			FILE* m_Log = nullptr;

		private:
			void LogBase(const wchar_t* string) const
			{
				fputws(string, m_Log);
				fputws(L"\r\n", m_Log);
				fflush(m_Log);
			}

			void InitConfig();
			bool OpenLog();
			void CloseLog();

			void InitFunctions();
			void OverrideFunctions();
			void RestoreFunctions();

		public:
			Redirector();
			~Redirector();

		public:
			const FunctionTable& GetFunctionTable() const
			{
				return m_Functions;
			}
			
			bool IsInitialThread(DWORD threadID) const
			{
				return m_InitialThreadID == threadID;
			}
			bool IsCurrentThreadInitial() const
			{
				return m_InitialThreadID == ::GetCurrentThreadId();
			}
			bool IsLogEnabled() const
			{
				return m_Log != nullptr;
			}
			int GetANSICodePage() const
			{
				return m_ANSICodePage;
			}
			bool IsOptionEnabled(RedirectorOption option) const
			{
				return m_Options.IsEnabled(option);
			}

			ConfigObject& GetOrLoadFile(KxDynamicStringRefW path);
			void SaveChnagedFiles(const wchar_t* message) const;
			size_t RefreshINI();
			
			KxDynamicStringW ConvertFromACP(const char* value, size_t length = KxDynamicStringA::npos) const
			{
				return KxDynamicStringW::to_utf16(value, length, m_ANSICodePage);
			}
			KxDynamicStringW ConvertFromACP(const KxDynamicStringA& value) const
			{
				return ConvertFromACP(value.data(), value.length());
			}
			KxDynamicStringW ConvertFromACP(KxDynamicStringRefA value) const
			{
				return ConvertFromACP(value.data(), value.length());
			}

			KxDynamicStringA ConvertToACP(const wchar_t* value, size_t length = KxDynamicStringW::npos) const
			{
				return KxDynamicStringW::to_codepage(value, length, m_ANSICodePage);
			}
			KxDynamicStringA ConvertToACP(const KxDynamicStringW& value) const
			{
				return ConvertToACP(value.data(), value.length());
			}
			KxDynamicStringA ConvertToACP(KxDynamicStringRefW value) const
			{
				return ConvertToACP(value.data(), value.length());
			}
			
			template<class ...Args> void Log(const wchar_t* format, Args&&... arg) const
			{
				if (m_Log)
				{
					if constexpr((sizeof...(Args)) != 0)
					{
						KxDynamicStringW logString = KxDynamicStringW::Format(format, std::forward<Args>(arg)...);
						LogBase(logString.data());
					}
					else
					{
						LogBase(format);
					}
				}
			}
	};
}
