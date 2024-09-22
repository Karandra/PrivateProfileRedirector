#include "stdafx.h"
#include "PrivateProfileRedirector.h"
#include "RedirectedFunctions.h"
#include "DLLApplication.h"
#include "AppConfigLoader.h"
#include "DLLEvent.h"
#include <kxf/Core/EncodingConverter/NativeEncodingConverter.h>
#include <detours/detours.h>
#include <detours/detver.h>

namespace PPR
{
	Redirector& Redirector::GetInstance()
	{
		return DLLApplication::GetInstance().GetRedirector();
	}

	void Redirector::LoadConfig(DLLApplication& app, const AppConfigLoader& config)
	{
		KX_SCOPEDLOG_FUNC;

		// Load options
		config.LoadRedirectorOption(m_Options, RedirectorOption::AllowSEVersionMismatch, L"AllowSEVersionMismatch");
		config.LoadRedirectorOption(m_Options, RedirectorOption::WriteProtected, L"WriteProtected");
		config.LoadRedirectorOption(m_Options, RedirectorOption::NativeWrite, L"NativeWrite", RedirectorOption::WriteProtected);
		config.LoadRedirectorOption(m_Options, RedirectorOption::SaveOnWrite, L"SaveOnWrite", RedirectorOption::WriteProtected);
		config.LoadRedirectorOption(m_Options, RedirectorOption::SaveOnThreadDetach, L"SaveOnThreadDetach", RedirectorOption::NativeWrite);
		config.LoadRedirectorOption(m_Options, RedirectorOption::SaveOnProcessDetach, L"SaveOnProcessDetach", RedirectorOption::NativeWrite);
		config.LoadRedirectorOption(m_Options, RedirectorOption::SaveOnGameSave, L"SaveOnGameSave", RedirectorOption::NativeWrite);
		config.LoadRedirectorOption(m_Options, RedirectorOption::ProcessInlineComments, L"ProcessInlineComments");
		m_SaveOnWriteBuffer = config.GetGeneral().GetAttributeInt(L"SaveOnWriteBuffer", m_SaveOnWriteBuffer);
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

		if (!m_Options.Contains(RedirectorOption::SaveOnWrite) || std::clamp(m_SaveOnWriteBuffer, 2, 4096) != m_SaveOnWriteBuffer)
		{
			m_SaveOnWriteBuffer = 0;
		}
		if (m_Options.Contains(RedirectorOption::SaveOnThreadDetach))
		{
			app.Bind(DLLEvent::EvtThreadDetach, [this](DLLEvent& event)
			{
				SaveChangedFiles(L"On thread detach");
			});
		}
		if (m_Options.Contains(RedirectorOption::SaveOnProcessDetach))
		{
			app.Bind(DLLEvent::EvtProcessDetach, [this](DLLEvent& event)
			{
				SaveChangedFiles(L"On process detach");
			});
		}

		// ENB integration
		if (auto& enbInterafce = app.GetENBInterface(); true)
		{
			// Called when config is about to get saved and seems to get called twice
			#if 0
			auto uniqueID = kxf::UniversallyUniqueID::CreateSequential();
			enbInterafce.Bind(ENBEvent::EvtPreSave, [&, uniqueID](ENBEvent& event)
			{
				enbInterafce.QueueUniqueEvent(uniqueID, ENBEvent::EvtPreReset);
			});
			#endif

			// This event isn't getting called anywhere as far as I'm aware
			enbInterafce.Bind(ENBEvent::EvtPreReset, [&](ENBEvent& event)
			{
				kxf::Log::Info("ENB Event: PreReset");
				RefreshINI();
			});

			// This triggers after save config or load config is completed
			enbInterafce.Bind(ENBEvent::EvtPostLoad, [&](ENBEvent& event)
			{
				SaveChangedFiles(L"ENB Event: PostLoad");
			});

			// Gets called on normal exit
			enbInterafce.Bind(ENBEvent::EvtOnExit, [&](ENBEvent& event)
			{
				SaveChangedFiles(L"ENB Event: OnExit");
			});
		}

		KX_SCOPEDLOG.SetSuccess();
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

	Redirector::Redirector(DLLApplication& app, const AppConfigLoader& config)
	{
		KX_SCOPEDLOG_FUNC;

		// Load config
		LoadConfig(app, config);

		// Save function pointers
		InitFunctions();

		// Initialize detour
		FunctionRedirector::Initialize();
		OverrideFunctions();

		KX_SCOPEDLOG.SetSuccess();
	}
	Redirector::~Redirector()
	{
		KX_SCOPEDLOG_FUNC;

		RestoreFunctions();
		FunctionRedirector::Uninitialize();

		KX_SCOPEDLOG.SetSuccess();
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
