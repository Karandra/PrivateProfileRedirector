#include "stdafx.h"
#include "RedirectorInterface.h"
#include "RedirectedFunctions.h"
#include "DLLApplication.h"
#include "AppConfigLoader.h"
#include "ENB/ENBInterface.h"
#include "xSE/ScriptExtenderInterface.h"
#include "DLLEvent.h"
#include <kxf/Core/EncodingConverter/NativeEncodingConverter.h>
#include <detours/detours.h>
#include <detours/detver.h>

namespace PPR
{
	// RedirectorInterface
	RedirectorInterface& RedirectorInterface::GetInstance()
	{
		return DLLApplication::GetInstance().GetRedirector();
	}

	void RedirectorInterface::LoadConfig(DLLApplication& app, const AppConfigLoader& config)
	{
		KXF_SCOPEDLOG_FUNC;

		// Load options
		config.LoadRedirectorOption(m_Options, RedirectorOption::WriteProtected, L"WriteProtected");
		config.LoadRedirectorOption(m_Options, RedirectorOption::NativeWrite, L"NativeWrite", RedirectorOption::WriteProtected);
		config.LoadRedirectorOption(m_Options, RedirectorOption::SaveOnWrite, L"SaveOnWrite", RedirectorOption::WriteProtected);
		config.LoadRedirectorOption(m_Options, RedirectorOption::SaveOnThreadDetach, L"SaveOnThreadDetach", RedirectorOption::NativeWrite);
		config.LoadRedirectorOption(m_Options, RedirectorOption::SaveOnProcessDetach, L"SaveOnProcessDetach", RedirectorOption::NativeWrite);
		config.LoadRedirectorOption(m_Options, RedirectorOption::SaveOnGameSave, L"SaveOnGameSave", RedirectorOption::NativeWrite);
		config.LoadRedirectorOption(m_Options, RedirectorOption::ProcessInlineComments, L"ProcessInlineComments");
		
		m_SaveOnWriteBuffer = config.GetRedirectorSection().GetAttributeInt(L"SaveOnWriteBuffer", m_SaveOnWriteBuffer);
		if (!m_Options.Contains(RedirectorOption::SaveOnWrite) || std::clamp(m_SaveOnWriteBuffer, 2, 4096) != m_SaveOnWriteBuffer)
		{
			m_SaveOnWriteBuffer = 0;
		}

		auto encodingConverter = std::make_unique<kxf::NativeEncodingConverter>(config.GetRedirectorSection().GetAttributeInt(L"CodePage", CP_ACP));

		// Print options
		KXF_SCOPEDLOG.Info().Format("WriteProtected: {}", m_Options.Contains(RedirectorOption::WriteProtected));
		KXF_SCOPEDLOG.Info().Format("NativeWrite: {}", m_Options.Contains(RedirectorOption::NativeWrite));
		KXF_SCOPEDLOG.Info().Format("SaveOnWrite: {}", m_Options.Contains(RedirectorOption::SaveOnWrite));
		KXF_SCOPEDLOG.Info().Format("SaveOnThreadDetach: {}", m_Options.Contains(RedirectorOption::SaveOnThreadDetach));
		KXF_SCOPEDLOG.Info().Format("SaveOnProcessDetach: {}", m_Options.Contains(RedirectorOption::SaveOnProcessDetach));
		KXF_SCOPEDLOG.Info().Format("SaveOnGameSave: {}", m_Options.Contains(RedirectorOption::SaveOnGameSave));
		KXF_SCOPEDLOG.Info().Format("ProcessInlineComments: {}", m_Options.Contains(RedirectorOption::ProcessInlineComments));
		KXF_SCOPEDLOG.Info().Format("SaveOnWriteBuffer: {}", m_SaveOnWriteBuffer);
		KXF_SCOPEDLOG.Info().Format("CodePage: '{}'/{}", encodingConverter->GetEncodingName(), encodingConverter->GetCodePage());
		m_EncodingConverter = std::move(encodingConverter);

		KXF_SCOPEDLOG.SetSuccess();
	}

	void RedirectorInterface::InitHooks()
	{
		KXF_SCOPEDLOG_FUNC;

		auto& controller = kxf::CFunctionHookController::GetInstance();
		if (auto transaction = controller.NewTransaction())
		{
			m_UntypedHooks.reserve(16);

			transaction.AttachFunction(m_UntypedHooks.emplace_back(), &::GetPrivateProfileStringA, &PrivateProfile::GetStringA, "GetPrivateProfileStringA");
			transaction.AttachFunction(m_UntypedHooks.emplace_back(), &::GetPrivateProfileStringW, &PrivateProfile::GetStringW, "GetPrivateProfileStringW");
			transaction.AttachFunction(m_UntypedHooks.emplace_back(), &::GetPrivateProfileIntA, &PrivateProfile::GetIntA, "GetPrivateProfileIntA");
			transaction.AttachFunction(m_UntypedHooks.emplace_back(), &::GetPrivateProfileIntW, &PrivateProfile::GetIntW, "GetPrivateProfileIntW");
			transaction.AttachFunction(m_UntypedHooks.emplace_back(), &::GetPrivateProfileSectionNamesA, &PrivateProfile::GetSectionNamesA, "GetPrivateProfileSectionNamesA");
			transaction.AttachFunction(m_UntypedHooks.emplace_back(), &::GetPrivateProfileSectionNamesW, &PrivateProfile::GetSectionNamesW, "GetPrivateProfileSectionNamesW");
			transaction.AttachFunction(m_UntypedHooks.emplace_back(), &::GetPrivateProfileSectionA, &PrivateProfile::GetSectionA, "GetPrivateProfileSectionA");
			transaction.AttachFunction(m_UntypedHooks.emplace_back(), &::GetPrivateProfileSectionW, &PrivateProfile::GetSectionW, "GetPrivateProfileSectionW");
			transaction.AttachFunction(m_WriteStringA, &::WritePrivateProfileStringA, &PrivateProfile::WriteStringA, "WritePrivateProfileStringA");
			transaction.AttachFunction(m_WriteStringW, &::WritePrivateProfileStringW, &PrivateProfile::WriteStringW, "WritePrivateProfileStringW");

			KXF_SCOPEDLOG.SetSuccess(transaction.Commit());
		}
	}
	void RedirectorInterface::RestoreHooks()
	{
		KXF_SCOPEDLOG_FUNC;

		auto& controller = kxf::CFunctionHookController::GetInstance();
		if (auto transaction = controller.NewTransaction())
		{
			for (auto& hook: m_UntypedHooks)
			{
				transaction.DetachHook(hook);
			}
			transaction.DetachHook(m_WriteStringA);
			transaction.DetachHook(m_WriteStringW);

			KXF_SCOPEDLOG.SetSuccess(transaction.Commit());
		}
	}
	void RedirectorInterface::SetupIntegrations(DLLApplication& app)
	{
		// Application
		if (m_Options.Contains(RedirectorOption::SaveOnThreadDetach))
		{
			app.Bind(DLLEvent::EvtThreadDetach, [this](DLLEvent& event)
			{
				SaveChangedFiles(L"DLLEvent: ThreadDetach");
			});
		}
		if (m_Options.Contains(RedirectorOption::SaveOnProcessDetach))
		{
			app.Bind(DLLEvent::EvtProcessDetach, [this](DLLEvent& event)
			{
				SaveChangedFiles(L"DLLEvent: ProcessDetach");
			});
		}

		// XSE integration
		if (auto& xseInterafce = app.GetXSEInterface(); true)
		{
			xseInterafce.Bind(ConsoleEvent::EvtCommand, [&](ConsoleEvent& event)
			{
				kxf::Log::Info("ConsoleEvent: Command");

				auto commandName = event.GetCommandName();
				if (commandName == "RefreshINI")
				{
					xseInterafce.PrintConsole("Executing '{}'", commandName);
					size_t count = RefreshINI();
					xseInterafce.PrintConsole("Executing '{}' done, {} files reloaded.", commandName, count);
				}
				else
				{
					xseInterafce.PrintConsole("Unknown command '{}'", commandName);
				}

				// Allow the original console command to be called after
				event.Skip();
			});

			if (m_Options.Contains(RedirectorOption::SaveOnGameSave))
			{
				xseInterafce.Bind(GameEvent::EvtGameSave, [&](GameEvent& event)
				{
					auto saveFile = event.GetSaveFile();
					kxf::Log::Info("Saving game: '{}'", saveFile);

					SaveChangedFiles(L"GameEvent: GameSave");
				});
			}
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
				kxf::Log::Info("ENBEvent: PreReset");
				RefreshINI();
			});

			// This triggers after save config or load config is completed
			enbInterafce.Bind(ENBEvent::EvtPostLoad, [&](ENBEvent& event)
			{
				SaveChangedFiles(L"ENBEvent: PostLoad");
			});

			// Gets called on normal exit
			enbInterafce.Bind(ENBEvent::EvtOnExit, [&](ENBEvent& event)
			{
				SaveChangedFiles(L"ENBEvent: OnExit");
			});
		}
	}

	RedirectorInterface::RedirectorInterface(DLLApplication& app, const AppConfigLoader& config)
	{
		KXF_SCOPEDLOG_FUNC;

		LoadConfig(app, config);

		KXF_SCOPEDLOG.SetSuccess();
	}
	RedirectorInterface::~RedirectorInterface()
	{
		KXF_SCOPEDLOG_FUNC;
		KXF_SCOPEDLOG.SetSuccess();
	}

	// AppModule
	void RedirectorInterface::OnInit(DLLApplication& app)
	{
		KXF_SCOPEDLOG_FUNC;

		InitHooks();
		SetupIntegrations(app);

		KXF_SCOPEDLOG.SetSuccess();
	}
	void RedirectorInterface::OnExit(DLLApplication& app)
	{
		KXF_SCOPEDLOG_FUNC;

		RestoreHooks();

		KXF_SCOPEDLOG.SetSuccess();
	}

	// RedirectorInterface
	ConfigObject& RedirectorInterface::GetOrLoadFile(const kxf::String& filePath)
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
		KXF_SCOPEDLOG_ARGS(filePath);
		kxf::WriteLockGuard lock(m_INIMapLock);

		if (filePath.IsEmpty())
		{
			KXF_SCOPEDLOG.Warning().Format("The requested file is an empty string, an empty config object will be used for it");
		}

		auto result = m_INIMap.insert_or_assign(filePath, std::make_unique<ConfigObject>(filePath));
		auto& config = result.first->second;
		config->LoadFile();

		KXF_SCOPEDLOG.Info().Format("Attempt to access file: '{}' -> file object {}. Exist on disk: {}", filePath, result.second ? "initialized" : "overwritten", config->IsExistOnDisk());
		
		KXF_SCOPEDLOG.SetSuccess();
		return *config;
	}
	size_t RedirectorInterface::SaveChangedFiles(const wchar_t* message)
	{
		KXF_SCOPEDLOG_ARGS(message, m_TotalWriteCount.load());

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
						KXF_SCOPEDLOG.Info().Format("File saved: '{}', is empty: {}", path, config->IsEmpty());
					}
				}
				else
				{
					KXF_SCOPEDLOG.Info().Format("No changes: '{}', is empty: {}", path, config->IsEmpty());
				}
			}
			KXF_SCOPEDLOG.Info().Format("All changed files saved. Total: {}, Changed: {}", m_INIMap.size(), changedCount);
		}
		m_TotalWriteCount = 0;

		KXF_SCOPEDLOG.LogReturn(changedCount);
		return changedCount;
	}
	size_t RedirectorInterface::OnFileWrite(ConfigObject& configObject) noexcept
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
	size_t RedirectorInterface::RefreshINI()
	{
		KXF_SCOPEDLOG_FUNC;

		size_t count = 0;
		if (kxf::WriteLockGuard lock(m_INIMapLock); !m_INIMap.empty())
		{
			for (const auto& [path, config]: m_INIMap)
			{
				KXF_SCOPEDLOG.Info().Format(L"Reloading '{}'", path);

				auto lock = config->LockExclusive();
				config->LoadFile();
				count++;
			}
			KXF_SCOPEDLOG.Info().Format(L"Executing 'RefreshINI' done, {} files reloaded", count);
		}

		KXF_SCOPEDLOG.LogReturn(count);
		return count;
	}
}
