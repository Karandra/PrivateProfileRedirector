#include "stdafx.h"
#include "PrivateProfileRedirector.h"
#include "RedirectedFunctions.h"
#include "xSE/ScriptExtenderInterfaceIncludes.h"
#include "xSE/ScriptExtenderInterface.h"

#include <detours.h>
#include <detver.h>
#pragma comment(lib, "detours.lib")

#include <Shlobj.h>

namespace PPR
{
	static Redirector* g_Instance = nullptr;

	constexpr int g_VersionMajor = 0;
	constexpr int g_VersionMinor = 5;
	constexpr int g_VersionPatch = 3;
	char g_VersionStringA[32] = {0};
	wchar_t g_VersionStringW[32] = {0};

	constexpr int g_SaveOnWriteBufferMin = 2;
	constexpr int g_SaveOnWriteBufferMax = 2000;
}

namespace PPR
{
	bool Redirector::HasInstance()
	{
		return g_Instance != nullptr;
	}
	Redirector& Redirector::GetInstance()
	{
		return *g_Instance;
	}
	Redirector& Redirector::CreateInstance()
	{
		DestroyInstance();
		new Redirector();

		return *g_Instance;
	}
	void Redirector::DestroyInstance()
	{
		delete g_Instance;
		g_Instance = nullptr;
	}

	const char* Redirector::GetLibraryNameA()
	{
		return "PrivateProfileRedirector";
	}
	const wchar_t* Redirector::GetLibraryNameW()
	{
		return L"PrivateProfileRedirector";
	}
	
	const char* Redirector::GetLibraryVersionA()
	{
		static char ms_VersionA[32] = {0};
		if (*ms_VersionA == '\0')
		{
			sprintf_s(ms_VersionA, "%d.%d.%d", g_VersionMajor, g_VersionMinor, g_VersionPatch);
		}
		return ms_VersionA;
	}
	const wchar_t* Redirector::GetLibraryVersionW()
	{
		if (*g_VersionStringW == L'\0')
		{
			swprintf_s(g_VersionStringW, L"%d.%d.%d", g_VersionMajor, g_VersionMinor, g_VersionPatch);
		}
		return g_VersionStringW;
	}
	int Redirector::GetLibraryVersionInt()
	{
		return MakeFullVersion(g_VersionMajor, g_VersionMinor, g_VersionPatch);
	}

	bool Redirector::DllMain(HMODULE module, DWORD event)
	{
		switch (event)
		{
			case DLL_PROCESS_ATTACH:
			{
				CreateInstance();
				break;
			}
			case DLL_THREAD_DETACH:
			{
				if (g_Instance && g_Instance->IsOptionEnabled(RedirectorOption::SaveOnThreadDetach))
				{
					g_Instance->SaveChnagedFiles(L"On thread detach");
				}
				break;
			}
			case DLL_PROCESS_DETACH:
			{
				if (g_Instance && g_Instance->IsOptionEnabled(RedirectorOption::SaveOnProcessDetach))
				{
					g_Instance->SaveChnagedFiles(L"On process detach");
				}
				DestroyInstance();

				break;
			}
		};
		return true;
	}
}

namespace PPR
{
	void Redirector::DoLog(KxDynamicStringRefA logString) const
	{
		auto logStringW = ConvertFromACP(logString);
		DoLog(logStringW);
	}
	void Redirector::DoLog(KxDynamicStringRefW logString) const
	{
		ExclusiveSRWLocker lock(m_LogLock);

		fwrite(logString.data(), sizeof(wchar_t), logString.size(), m_Log);
		fputws(L"\n", m_Log);
		fflush(m_Log);
	}

	KxDynamicStringW Redirector::GetShellDirectory(const GUID& guid) const
	{
		wchar_t* pathBuffer = nullptr;
		KxCallAtScopeExit atExit = [&]()
		{
			if (pathBuffer)
			{
				::CoTaskMemFree(pathBuffer);
			}
		};

		if (SUCCEEDED(::SHGetKnownFolderPath(guid, KF_FLAG_DONT_VERIFY, nullptr, &pathBuffer)))
		{
			return pathBuffer;
		}
		return {};
	}
	KxDynamicStringW Redirector::GetGameUserProfileDirectory() const
	{
		KxDynamicStringW path = GetShellDirectory(FOLDERID_Documents);
		if (!path.empty())
		{
			path += L"\\My Games\\";
			path += xSE_USER_PROFILE_FOLDER_NAME_W;
			path += L"\\";
			path += xSE_FOLDER_NAME_W;

			return path;
		}
		return {};
	}

	void Redirector::InitConfig()
	{
		// Load config
		RedirectorConfigLoader config(m_Options, L"Data\\" xSE_FOLDER_NAME_W "\\Plugins\\PrivateProfileRedirector.ini");

		// Open log
		if (config.GetBool(L"EnableLog"))
		{
			OpenLog();
		}

		// Load options
		config.LoadOption(RedirectorOption::AllowSEVersionMismatch, L"AllowSEVersionMismatch");
		config.LoadOption(RedirectorOption::WriteProtected, L"WriteProtected");
		config.LoadOption(RedirectorOption::NativeWrite, L"NativeWrite", RedirectorOption::WriteProtected);
		config.LoadOption(RedirectorOption::SaveOnWrite, L"SaveOnWrite", RedirectorOption::WriteProtected);
		config.LoadOption(RedirectorOption::SaveOnThreadDetach, L"SaveOnThreadDetach", RedirectorOption::NativeWrite);
		config.LoadOption(RedirectorOption::SaveOnProcessDetach, L"SaveOnProcessDetach", RedirectorOption::NativeWrite);
		config.LoadOption(RedirectorOption::SaveOnGameSave, L"SaveOnGameSave", RedirectorOption::NativeWrite);
		config.LoadOption(RedirectorOption::ProcessInlineComments, L"ProcessInlineComments");
		m_ANSICodePage = config.GetInt(L"ANSICodePage", m_ANSICodePage);
		m_SaveOnWriteBuffer = config.GetInt(L"SaveOnWriteBuffer", m_SaveOnWriteBuffer);

		// Correct options
		if (m_SaveOnWriteBuffer < g_SaveOnWriteBufferMin || m_SaveOnWriteBuffer > g_SaveOnWriteBufferMax || !m_Options.IsEnabled(RedirectorOption::SaveOnWrite))
		{
			m_SaveOnWriteBuffer = 0;
		}

		// Print options
		Log(L"Loaded options:");
		config.LogOption(m_Log != nullptr, L"EnableLog");
		config.LogOption(RedirectorOption::AllowSEVersionMismatch, L"AllowSEVersionMismatch");
		config.LogOption(RedirectorOption::WriteProtected, L"WriteProtected");
		config.LogOption(RedirectorOption::NativeWrite, L"NativeWrite");
		config.LogOption(RedirectorOption::SaveOnWrite, L"SaveOnWrite");
		config.LogOption(RedirectorOption::SaveOnThreadDetach, L"SaveOnThreadDetach");
		config.LogOption(RedirectorOption::SaveOnProcessDetach, L"SaveOnProcessDetach");
		config.LogOption(RedirectorOption::SaveOnGameSave, L"SaveOnGameSave");
		config.LogOption(RedirectorOption::ProcessInlineComments, L"ProcessInlineComments");
		config.LogOption(m_ANSICodePage, L"ANSICodePage");
		config.LogOption(m_SaveOnWriteBuffer, L"SaveOnWriteBuffer");
	}
	bool Redirector::OpenLog()
	{
		// Get the path to the user's profile directory and try to create the log there
		KxDynamicStringW userPath = GetGameUserProfileDirectory();
		userPath += L"\\PrivateProfileRedirector.log";

		_wfopen_s(&m_Log, userPath.data(), L"w+b");
		if (!m_Log)
		{
			// If failed, try to create the file in the game's directory
			_wfopen_s(&m_Log, L"Data\\" xSE_FOLDER_NAME_W L"\\Plugins\\PrivateProfileRedirector.log", L"w+b");
		}

		if (m_Log)
		{
			Log(L"Log opened");
			Log(L"%s v%s", GetLibraryNameW(), GetLibraryVersionW());
			Log(L"Script Extender platform: %s v%s", xSE_NAME_W, xSE_VERSION_W);

			return true;
		}
		return false;
	}
	void Redirector::CloseLog()
	{
		if (m_Log)
		{
			Log(L"Log closed");
			fclose(m_Log);
			m_Log = nullptr;
		}
	}

	void Redirector::InitFunctions()
	{
		// PrivateProfile
		m_Functions.PrivateProfile.GetStringA = ::GetPrivateProfileStringA;
		m_Functions.PrivateProfile.GetStringW = ::GetPrivateProfileStringW;

		m_Functions.PrivateProfile.GetIntA = ::GetPrivateProfileIntA;
		m_Functions.PrivateProfile.GetIntW = ::GetPrivateProfileIntW;

		m_Functions.PrivateProfile.GetSectionNamesA = ::GetPrivateProfileSectionNamesA;
		m_Functions.PrivateProfile.GetSectionNamesW = ::GetPrivateProfileSectionNamesW;

		m_Functions.PrivateProfile.GetSectionA = ::GetPrivateProfileSectionA;
		m_Functions.PrivateProfile.GetSectionW = ::GetPrivateProfileSectionW;

		m_Functions.PrivateProfile.WriteStringA = ::WritePrivateProfileStringA;
		m_Functions.PrivateProfile.WriteStringW = ::WritePrivateProfileStringW;
	}
	void Redirector::OverrideFunctions()
	{
		#define AttachFunctionN(name)	FunctionRedirector::AttachFunction(&m_Functions.PrivateProfile.##name, &PrivateProfile::##name, L#name)

		// 1
		FunctionRedirector::PerformTransaction([this]()
		{
			AttachFunctionN(GetStringA);
			AttachFunctionN(GetStringW);

			AttachFunctionN(GetIntA);
			AttachFunctionN(GetIntW);
		});

		// 2
		FunctionRedirector::PerformTransaction([this]()
		{
			AttachFunctionN(WriteStringA);
			AttachFunctionN(WriteStringW);

			AttachFunctionN(GetSectionNamesA);
			AttachFunctionN(GetSectionNamesW);
		});

		// 3
		FunctionRedirector::PerformTransaction([this]()
		{
			AttachFunctionN(GetSectionA);
			AttachFunctionN(GetSectionW);
		});

		#undef AttachFunctionN
	}
	void Redirector::RestoreFunctions()
	{
		#define DetachFunctionN(name)	FunctionRedirector::DetachFunction(&m_Functions.PrivateProfile.##name, &PrivateProfile::##name, L#name)

		// 1
		FunctionRedirector::PerformTransaction([this]()
		{
			DetachFunctionN(GetStringA);
			DetachFunctionN(GetStringW);

			DetachFunctionN(GetIntA);
			DetachFunctionN(GetIntW);
		});

		// 2
		FunctionRedirector::PerformTransaction([this]()
		{
			DetachFunctionN(WriteStringA);
			DetachFunctionN(WriteStringW);

			DetachFunctionN(GetSectionNamesA);
			DetachFunctionN(GetSectionNamesW);
		});

		// 3
		FunctionRedirector::PerformTransaction([this]()
		{
			DetachFunctionN(GetSectionA);
			DetachFunctionN(GetSectionW);
		});

		#undef DetachFunctionN
	}

	Redirector::Redirector()
		:m_InitialThreadID(::GetCurrentThreadId())
	{
		g_Instance = this;

		// Load config
		InitConfig();

		// Save function pointers
		InitFunctions();

		// Initialize detour
		FunctionRedirector::Initialize();
		OverrideFunctions();
	}
	Redirector::~Redirector()
	{
		RestoreFunctions();
		FunctionRedirector::Uninitialize();

		CloseLog();
		g_Instance = nullptr;
	}

	SEInterface& Redirector::GetSEInterface() const
	{
		return SEInterface::GetInstance();
	}

	ConfigObject& Redirector::GetOrLoadFile(KxDynamicStringRefW path)
	{
		// Get loaded file
		if (SharedSRWLocker lock(m_INIMapLock); true)
		{
			if (auto it = m_INIMap.find(path); it != m_INIMap.end())
			{
				return *it->second;
			}
		}

		// Load the file
		ExclusiveSRWLocker lock(m_INIMapLock);

		auto& config = m_INIMap.insert_or_assign(path, std::make_unique<ConfigObject>(path)).first->second;
		config->LoadFile();

		Log(L"Attempt to access file: '%s' -> file object initialized. Exist on disk: %d", path.data(), config->IsExistOnDisk());
		return *config;
	}
	void Redirector::SaveChnagedFiles(const wchar_t* message) const
	{
		Log(L"Saving files: %s", message);

		SharedSRWLocker mapLock(m_INIMapLock);

		size_t changedCount = 0;
		for (const auto& [path, config]: m_INIMap)
		{
			auto lock = config->LockExclusive();

			if (config->HasChanges())
			{
				changedCount++;
				config->SaveFile();
				Log(L"File saved: '%s', Is empty: %d", path.data(), (int)config->IsEmpty());
			}
			else
			{
				Log(L"No changes: '%s', Is empty: %d", path.data(), (int)config->IsEmpty());
			}
		}
		Log(L"All changed files saved. Total: %zu, Changed: %zu", m_INIMap.size(), changedCount);
	}
	size_t Redirector::RefreshINI()
	{
		Log(L"Executing 'RefreshINI'");
		ExclusiveSRWLocker mapLock(m_INIMapLock);

		for (const auto& [path, config]: m_INIMap)
		{
			Log(L"Reloading '%s'", path.data());
			
			auto lock = config->LockExclusive();
			config->LoadFile();
		}

		Log(L"Executing 'RefreshINI' done, %zu files reloaded.", m_INIMap.size());
		return m_INIMap.size();
	}
}
