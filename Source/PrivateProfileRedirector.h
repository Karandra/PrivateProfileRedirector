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
	class SEInterface;

	constexpr int MakeFullVersion(int major, int minor, int patch) noexcept
	{
		// 1.2.3 -> 1 * 100 + 2 * 10 + 3 * 1 = 123
		// 0.1 -> (0 * 100) + (1 * 10) + (0 * 1) = 10
		return (major * 100) + (minor * 10) + (patch * 1);
	}
}

namespace PPR
{
	class Redirector final
	{
		friend class RedirectorConfigLoader;

		public:
			static bool HasInstance();
			static Redirector& GetInstance();
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

			RedirectorOptionSet m_Options;
			int m_ANSICodePage = CP_ACP;
			int m_SaveOnWriteBuffer = 0;

			Utility::String::UnorderedMapWNoCase<std::unique_ptr<ConfigObject>> m_INIMap;
			mutable SRWLock m_INIMapLock;
			
			FILE* m_Log = nullptr;
			mutable SRWLock m_LogLock;

		private:
			void DoLog(KxDynamicStringRefA logString) const;
			void DoLog(KxDynamicStringRefW logString) const;

			KxDynamicStringW GetShellDirectory(const GUID& guid) const;
			KxDynamicStringW GetGameUserProfileDirectory() const;

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
			SEInterface& GetSEInterface() const;

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
			std::optional<size_t> GetSaveOnWriteBuffer() const
			{
				if (m_SaveOnWriteBuffer > 0)
				{
					return m_SaveOnWriteBuffer;
				}
				return std::nullopt;
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
			
			void Log(KxDynamicStringRefA logString) const
			{
				if (m_Log)
				{
					DoLog(logString);
				}
			}
			void Log(KxDynamicStringRefW logString) const
			{
				if (m_Log)
				{
					DoLog(logString);
				}
			}

			template<class TChar, class... Args>
			void Log(const TChar* format, Args&&... arg) const
			{
				if (m_Log)
				{
					if (format)
					{
						if constexpr((sizeof...(Args)) != 0)
						{
							auto logString = KxBasicDynamicString<TChar, KxDynamicStringW::StaticStorageLength>::Format(format, std::forward<Args>(arg)...);
							DoLog(logString);
						}
						else
						{
							DoLog(format);
						}
					}
					else
					{
						DoLog(L"<Null log string>");
					}
				}
			}
	};
}
