#include "stdafx.h"
#include "PrivateProfileRedirector.h"
#include "RedirectedFunctions.h"
#include "xSE/ScriptExtenderInterfaceIncludes.h"
#include "xSE/ScriptExtenderInterface.h"
#include <kxf/Core/EncodingConverter/NativeEncodingConverter.h>
#include <kxf/Log/ScopedLoggerContext.h>
#include <kxf/System/ShellOperations.h>
#include <detours/detours.h>
#include <detours/detver.h>

namespace
{
	std::unique_ptr<PPR::Redirector> g_Instance;
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

	kxf::String Redirector::GetLibraryName()
	{
		return kxf::StringViewOf(ProjectName);
	}
	kxf::String Redirector::GetLibraryAuthor()
	{
		return kxf::StringViewOf(ProjectAuthor);
	}
	kxf::Version Redirector::GetLibraryVersion()
	{
		return {VersionMajor, VersionMinor, VersionPatch};
	}

	bool Redirector::DllMain(HMODULE module, DWORD event)
	{
		switch (event)
		{
			case DLL_PROCESS_ATTACH:
			{
				if (!g_Instance)
				{
					g_Instance = std::make_unique<PPR::Redirector>();
					kxf::Log::Info("Created PPR::Redirector instance");
				}
				else
				{
					kxf::Log::Warning("An instance of PPR::Redirector already exists");
				}
				break;
			}
			case DLL_THREAD_DETACH:
			{
				if (g_Instance && g_Instance->IsOptionEnabled(RedirectorOption::SaveOnThreadDetach))
				{
					g_Instance->SaveChangedFiles(L"On thread detach");
				}
				break;
			}
			case DLL_PROCESS_DETACH:
			{
				if (g_Instance && g_Instance->IsOptionEnabled(RedirectorOption::SaveOnProcessDetach))
				{
					g_Instance->SaveChangedFiles(L"On process detach");
				}
				g_Instance = {};

				break;
			}
		};
		return true;
	}

	void Redirector::InitConfig()
	{
		m_PluginFS.SetLookupDirectory(kxf::NativeFileSystem::GetCurrentModuleRootDirectory());
		m_ConfigFS.SetLookupDirectory(kxf::Shell::GetKnownDirectory(kxf::KnownDirectoryID::Documents) / "My Games" / xSE_CONFIG_FOLDER_NAME_W / xSE_FOLDER_NAME_W);

		// Load config
		RedirectorConfigLoader config(m_PluginFS.OpenToRead("PrivateProfileRedirector.ini"));

		// Open log
		if (auto enableLog = config.GetGeneral().QueryAttributeBool(L"EnableLog"); enableLog == true)
		{
			OpenLog(kxf::LogLevel::Unknown);
		}
		else if (auto logLevel = config.GetGeneral().QueryAttributeInt<kxf::LogLevel>(L"LogLevel"))
		{
			OpenLog(*logLevel);
		}
		KX_SCOPEDLOG_FUNC;
		KX_SCOPEDLOG.Info().Format("{} v{} by {} loaded", Redirector::GetLibraryName(), Redirector::GetLibraryVersion().ToString(), GetLibraryAuthor());
		KX_SCOPEDLOG.Info()
			KX_SCOPEDLOG_VALUE_AS(m_PluginFS, m_PluginFS.GetLookupDirectory().GetFullPath())
			KX_SCOPEDLOG_VALUE_AS(m_ConfigFS, m_ConfigFS.GetLookupDirectory().GetFullPath());

		if (config.IsNull())
		{
			KX_SCOPEDLOG.Warning().Format("Failed to load config file, using default options");
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
		m_Options = config.GetOptions();

		m_SaveOnWriteBuffer = config.GetGeneral().GetAttributeInt(L"SaveOnWriteBuffer", m_SaveOnWriteBuffer);
		if (!m_Options.Contains(RedirectorOption::SaveOnWrite) || std::clamp(m_SaveOnWriteBuffer, 2, 4096) != m_SaveOnWriteBuffer)
		{
			m_SaveOnWriteBuffer = 0;
		}

		auto encodingConverter = std::make_unique<kxf::NativeEncodingConverter>(config.GetGeneral().GetAttributeInt(L"CodePage", CP_ACP));

		// Print options
		KX_SCOPEDLOG.Info().Format("LogLevel: {} -> {}", kxf::ScopedLoggerGlobalContext::GetInstance().GetLogLevel(), kxf::Log::IsEnabled());
		KX_SCOPEDLOG.Info().Format("AllowSEVersionMismatch: {}", m_Options.Contains(RedirectorOption::AllowSEVersionMismatch));
		KX_SCOPEDLOG.Info().Format("WriteProtected: {}", m_Options.Contains(RedirectorOption::WriteProtected));
		KX_SCOPEDLOG.Info().Format("NativeWrite: {}", m_Options.Contains(RedirectorOption::NativeWrite));
		KX_SCOPEDLOG.Info().Format("SaveOnWrite: {}", m_Options.Contains(RedirectorOption::SaveOnWrite));
		KX_SCOPEDLOG.Info().Format("SaveOnThreadDetach: {}", m_Options.Contains(RedirectorOption::SaveOnThreadDetach));
		KX_SCOPEDLOG.Info().Format("SaveOnProcessDetach: {}", m_Options.Contains(RedirectorOption::SaveOnProcessDetach));
		KX_SCOPEDLOG.Info().Format("SaveOnGameSave: {}", m_Options.Contains(RedirectorOption::SaveOnGameSave));
		KX_SCOPEDLOG.Info().Format("ProcessInlineComments: {}", m_Options.Contains(RedirectorOption::ProcessInlineComments));
		KX_SCOPEDLOG.Info().Format("SaveOnWriteBuffer: {}", m_SaveOnWriteBuffer);
		KX_SCOPEDLOG.Info().Format("CodePage: '{}'/{}", encodingConverter->GetEncodingName(), encodingConverter->GetCodePage());

		m_EncodingConverter = std::move(encodingConverter);
		KX_SCOPEDLOG.SetSuccess();
	}
	bool Redirector::OpenLog(kxf::LogLevel logLevel)
	{
		if (kxf::ToInt(logLevel) > 0)
		{
			auto stream = m_ConfigFS.OpenToWrite("PrivateProfileRedirector.log");
			if (!stream)
			{
				stream = m_PluginFS.OpenToWrite("PrivateProfileRedirector.log");
			}

			if (stream)
			{
				kxf::ScopedLoggerGlobalContext::Initialize(std::make_shared<kxf::ScopedLoggerSingleFileContext>(std::move(stream)), logLevel);

				kxf::Log::Info("{} v{}", GetLibraryName(), GetLibraryVersion().ToString());
				kxf::Log::Info(L"Script Extender platform: {} [0x{:08x}]", xSE_NAME_W, static_cast<uint32_t>(xSE_PACKED_VERSION));

				return true;
			}
		}
		return false;
	}

	void Redirector::InitFunctions()
	{
		KX_SCOPEDLOG_FUNC;

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

		KX_SCOPEDLOG.SetSuccess();
	}
	void Redirector::OverrideFunctions()
	{
		KX_SCOPEDLOG_FUNC;
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
		KX_SCOPEDLOG.SetSuccess();
	}
	void Redirector::RestoreFunctions()
	{
		KX_SCOPEDLOG_FUNC;
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
		KX_SCOPEDLOG.SetSuccess();
	}

	Redirector::Redirector()
	{
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
		KX_SCOPEDLOG_FUNC;

		RestoreFunctions();
		FunctionRedirector::Uninitialize();

		KX_SCOPEDLOG.SetSuccess();
	}

	SEInterface& Redirector::GetSEInterface() const noexcept
	{
		return SEInterface::GetInstance();
	}

	ConfigObject& Redirector::GetOrLoadFile(const kxf::String& filePath)
	{
		// Get loaded file
		if (kxf::ReadLockGuard lock(m_INIMapLock); !m_INIMap.empty())
		{
			if (auto it = m_INIMap.find(filePath); it != m_INIMap.end())
			{
				return *it->second;
			}
		}

		// Load the file
		KX_SCOPEDLOG_ARGS(filePath);
		kxf::WriteLockGuard lock(m_INIMapLock);

		auto& config = m_INIMap.insert_or_assign(filePath, std::make_unique<ConfigObject>(filePath)).first->second;
		config->LoadFile();

		KX_SCOPEDLOG.Info().Format("Attempt to access file: '{}' -> file object initialized. Exist on disk: {}", filePath, config->IsExistOnDisk());
		
		KX_SCOPEDLOG.SetSuccess();
		return *config;
	}
	size_t Redirector::SaveChangedFiles(const wchar_t* message)
	{
		KX_SCOPEDLOG_ARGS(message, m_TotalWriteCount.load());

		size_t changedCount = 0;
		if (kxf::ReadLockGuard lock(m_INIMapLock); !m_INIMap.empty())
		{
			for (const auto& [path, config]: m_INIMap)
			{
				auto lock = config->LockExclusive();

				if (config->HasChanges())
				{
					if (config->SaveFile())
					{
						changedCount++;
						KX_SCOPEDLOG.Info().Format("File saved: '{}', is empty: {}", path, config->IsEmpty());
					}
				}
				else
				{
					KX_SCOPEDLOG.Info().Format("No changes: '{}', is empty: {}", path, config->IsEmpty());
				}
			}
			KX_SCOPEDLOG.Info().Format("All changed files saved. Total: {}, Changed: {}", m_INIMap.size(), changedCount);
		}
		m_TotalWriteCount = 0;

		KX_SCOPEDLOG.LogReturn(changedCount);
		return changedCount;
	}
	size_t Redirector::OnFileWrite(ConfigObject& configObject) noexcept
	{
		auto count = ++m_TotalWriteCount;
		#if 0
		if (m_SaveOnWriteBuffer > 0)
		{
			if (count >= m_SaveOnWriteBuffer)
			{
				kxf::Log::TraceCategory("SaveOnWrite:Total", "Total changes amount {} reached buffer capacity ({}), flushing changes", count, m_SaveOnWriteBuffer);
				SaveChangedFiles(L"Total file writes");
			}
			else
			{
				kxf::Log::TraceCategory("SaveOnWrite:Total", "Accumulated {} changes (out of {}) for all files", count, m_SaveOnWriteBuffer);
			}
		}
		#endif

		return count;
	}
	size_t Redirector::RefreshINI()
	{
		KX_SCOPEDLOG_FUNC;

		size_t count = 0;
		if (kxf::WriteLockGuard lock(m_INIMapLock); !m_INIMap.empty())
		{
			for (const auto& [path, config]: m_INIMap)
			{
				KX_SCOPEDLOG.Info().Format(L"Reloading '{}'", path);

				auto lock = config->LockExclusive();
				config->LoadFile();
				count++;
			}
			KX_SCOPEDLOG.Info().Format(L"Executing 'RefreshINI' done, {} files reloaded", m_INIMap.size());
		}

		KX_SCOPEDLOG.LogReturn(count);
		return count;
	}
}
