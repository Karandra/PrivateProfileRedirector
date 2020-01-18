#include "stdafx.h"
#include "PrivateProfileRedirector.h"
#include "RedirectedFunctions.h"
#include "xSE\ScriptExtenderInterfaceIncludes.h"
#include "xSE\ScriptExtenderDefines.h"

#include <detours.h>
#include <detver.h>
#pragma comment(lib, "detours.lib")

namespace PPR
{
	static Redirector* g_Instance = nullptr;
	constexpr int g_VersionMajor = 0;
	constexpr int g_VersionMinor = 4;
	constexpr int g_VersionPatch = 1;
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
	Redirector* Redirector::GetInstancePtr()
	{
		return g_Instance;
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
		static char ms_VersionA[16] = {0};
		if (*ms_VersionA == '\000')
		{
			sprintf_s(ms_VersionA, "%d.%d.%d", g_VersionMajor, g_VersionMinor, g_VersionPatch);
		}
		return ms_VersionA;
	}
	const wchar_t* Redirector::GetLibraryVersionW()
	{
		static wchar_t ms_VersionW[16] = {0};
		if (*ms_VersionW == L'\000')
		{
			swprintf_s(ms_VersionW, L"%d.%d.%d", g_VersionMajor, g_VersionMinor, g_VersionPatch);
		}
		return ms_VersionW;
	}
	int Redirector::GetLibraryVersionInt()
	{
		// 1.2.3 -> 1 * 100 + 2 * 10 + 3 * 1 = 123
		// 0.1 -> (0 * 100) + (1 * 10) + (0 * 1) = 10
		return (g_VersionMajor * 100) + (g_VersionMinor * 10) + (g_VersionPatch * 1);
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
				DestroyInstance();
				break;
			}
		};
		return true;
	}
}

namespace PPR
{
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
		config.LoadOption(RedirectorOption::ProcessInlineComments, L"ProcessInlineComments");
		m_ANSICodePage = config.GetInt(L"ANSICodePage", m_ANSICodePage);

		// Print options
		Log(L"Loaded options:");
		config.LogOption(m_Log != nullptr, L"EnableLog");
		config.LogOption(RedirectorOption::AllowSEVersionMismatch, L"AllowSEVersionMismatch");
		config.LogOption(RedirectorOption::WriteProtected, L"WriteProtected");
		config.LogOption(RedirectorOption::NativeWrite, L"NativeWrite");
		config.LogOption(RedirectorOption::SaveOnWrite, L"SaveOnWrite");
		config.LogOption(RedirectorOption::SaveOnThreadDetach, L"SaveOnThreadDetach");
		config.LogOption(RedirectorOption::ProcessInlineComments, L"ProcessInlineComments");
		config.LogOption(m_ANSICodePage, L"ANSICodePage");
	}
	bool Redirector::OpenLog()
	{
		_wfopen_s(&m_Log, L"Data\\" xSE_FOLDER_NAME_W L"\\Plugins\\PrivateProfileRedirector.log", L"w+b");
		if (m_Log)
		{
			Log(L"Log opened");
			Log(L"%s v%s", GetLibraryNameW(), GetLibraryVersionW());
			Log(L"Script Extender platform: %s v%s", xSE_NAME_W, xSE_VERSION_W);
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
		m_Functions.PrivateProfile.GetStringA = GetPrivateProfileStringA;
		m_Functions.PrivateProfile.GetStringW = GetPrivateProfileStringW;

		m_Functions.PrivateProfile.GetIntA = GetPrivateProfileIntA;
		m_Functions.PrivateProfile.GetIntW = GetPrivateProfileIntW;

		m_Functions.PrivateProfile.GetSectionNamesA = GetPrivateProfileSectionNamesA;
		m_Functions.PrivateProfile.GetSectionNamesW = GetPrivateProfileSectionNamesW;

		m_Functions.PrivateProfile.GetSectionA = GetPrivateProfileSectionA;
		m_Functions.PrivateProfile.GetSectionW = GetPrivateProfileSectionW;

		m_Functions.PrivateProfile.WriteStringA = WritePrivateProfileStringA;
		m_Functions.PrivateProfile.WriteStringW = WritePrivateProfileStringW;
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
		FunctionRedirector::Uninitialize();
		RestoreFunctions();

		if (m_Options.IsEnabled(RedirectorOption::SaveOnThreadDetach))
		{
			SaveChnagedFiles(L"On process detach");
		}

		CloseLog();
		g_Instance = nullptr;
	}

	ConfigObject& Redirector::GetOrLoadFile(KxDynamicStringRefW path)
	{
		auto it = m_INIMap.find(path);
		if (it != m_INIMap.end())
		{
			return *it->second;
		}
		else
		{
			ExclusiveSRWLocker lock(m_INIMapLock);

			auto& ini = m_INIMap.insert_or_assign(path, std::make_unique<ConfigObject>(path)).first->second;
			ini->LoadFile();

			Log(L"Attempt to access file: '%s' -> file object initialized. Exist on disk: %d", path.data(), ini->IsExistOnDisk());
			return *ini;
		}
	}
	void Redirector::SaveChnagedFiles(const wchar_t* message) const
	{
		Log(L"Saving files: %s", message);

		size_t changedCount = 0;
		for (const auto& [path, config]: m_INIMap)
		{
			if (config->HasChanges())
			{
				changedCount++;
				config->SaveFile();
				Log(L"File saved: '%s', Is empty: %d", path.data(), (int)config->IsEmpty());
			}
			else
			{
				Log(L"File wasn't changed: '%s', Is empty: %d", path.data(), (int)config->IsEmpty());
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
