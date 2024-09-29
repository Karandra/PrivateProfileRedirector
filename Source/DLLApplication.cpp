#include "DLLApplication.h"
#include "AppConfigLoader.h"
#include "DLLEvent.h"
#include "xSE/ScriptExtenderInterfaceIncludes.h"
#include <kxf/Log/ScopedLoggerContext.h>
#include <kxf/System/ShellOperations.h>
#include <kxf/System/DynamicLibrary.h>
#include <kxf/System/SystemProcess.h>
#include <kxf/System/NativeAPI.h>

namespace
{
	std::unique_ptr<PPR::DLLApplication> g_Instance;
}

BOOL APIENTRY DllMain(HMODULE module, DWORD event, void* lpReserved)
{
	switch (event)
	{
		case DLL_THREAD_ATTACH:
		{
			kxf::Log::Info("DllMain({}, DLL_THREAD_ATTACH, {})", static_cast<void*>(module), lpReserved);
			break;
		}
		case DLL_THREAD_DETACH:
		{
			kxf::Log::Info("DllMain({}, DLL_THREAD_DETACH, {})", static_cast<void*>(module), lpReserved);
			break;
		}
		case DLL_PROCESS_ATTACH:
		{
			kxf::Log::Info("DllMain({}, DLL_PROCESS_ATTACH, {})", static_cast<void*>(module), lpReserved);
			break;
		}
		case DLL_PROCESS_DETACH:
		{
			kxf::Log::Info("DllMain({}, DLL_PROCESS_DETACH, {})", static_cast<void*>(module), lpReserved);
			break;
		}
		default:
		{
			kxf::Log::Info("DllMain({}, Unk={}, {})", static_cast<void*>(module), event, lpReserved);
			return FALSE;
		}
	};

	if (event == DLL_PROCESS_ATTACH)
	{
		if (!g_Instance)
		{
			g_Instance = std::make_unique<PPR::DLLApplication>();
			kxf::Log::Info("Created 'PPR::DLLApplication' instance");

			if (!g_Instance->OnInit())
			{
				return FALSE;
			}
		}
		else
		{
			kxf::Log::Warning("An instance of 'PPR::DLLApplication' already exists");
			return TRUE;
		}
	}
	else if (event == DLL_PROCESS_DETACH)
	{
		kxf::Log::Info("Destroying 'PPR::DLLApplication' instance");
		if (g_Instance)
		{
			g_Instance->OnDLLEntry(module, event);
			g_Instance->OnExit();
			g_Instance = {};

			return TRUE;
		}
	}

	if (g_Instance)
	{
		g_Instance->OnDLLEntry(module, event);
	}
	return TRUE;
}
void DummyFunction()
{
}

namespace PPR
{
	// DLLApplication
	DLLApplication& DLLApplication::GetInstance() noexcept
	{
		return *g_Instance;
	}
	void DLLApplication::OnProcessAttach()
	{
		auto currentModule = kxf::DynamicLibrary::GetCurrentModule();
		DllMain(static_cast<HMODULE>(currentModule.GetHandle()), DLL_PROCESS_ATTACH, nullptr);
	}

	AppConfigLoader DLLApplication::LoadConfig()
	{
		m_PluginFS.SetLookupDirectory(kxf::NativeFileSystem::GetCurrentModuleRootDirectory());
		m_ConfigFS.SetLookupDirectory(kxf::Shell::GetKnownDirectory(kxf::KnownDirectoryID::Documents) / "My Games" / xSE_CONFIG_FOLDER_NAME_W / xSE_FOLDER_NAME_W);

		// Open log
		AppConfigLoader config(m_PluginFS.OpenToRead("PrivateProfileRedirector.ini"));
		if (auto enableLog = config.GetGeneralSection().QueryAttributeBool(L"EnableLog"); enableLog == true)
		{
			// Open log with max level, v0.5.x compatibility
			OpenLog(kxf::LogLevel::Unknown);
		}
		else if (auto logLevel = config.GetGeneralSection().QueryAttributeInt<kxf::LogLevel>(L"LogLevel"))
		{
			OpenLog(*logLevel);
		}

		if (!config.IsNull())
		{
			kxf::Log::Info("LogLevel: {} -> {}", kxf::ScopedLoggerGlobalContext::GetInstance().GetLogLevel(), kxf::Log::IsEnabled());
		}
		else
		{
			kxf::Log::Warning("Failed to load config file, using default options");
		}
		return config;
	}
	bool DLLApplication::OpenLog(kxf::LogLevel logLevel)
	{
		if (kxf::ToInt(logLevel) > 0)
		{
			// Try 'Documents\My Games\<Game>\<xSE>' folder first
			auto stream = m_ConfigFS.OpenToWrite("PrivateProfileRedirector.log");
			if (!stream)
			{
				// Then '<Game>\Data\<xSE>\Plugins' folder first
				stream = m_PluginFS.OpenToWrite("PrivateProfileRedirector.log");
			}

			if (stream)
			{
				kxf::ScopedLoggerGlobalContext::Initialize(std::make_shared<kxf::ScopedLoggerSingleFileContext>(std::move(stream)), logLevel);
				return true;
			}
		}
		return false;
	}

	void DLLApplication::LogInformation()
	{
		kxf::Log::Info("{} v{} by {} loaded", GetDisplayName(), GetVersion().ToString(), GetVendorDisplayName());
		kxf::Log::Info(L"Script Extender build: {} [0x{:08x}]", xSE_NAME_W, static_cast<uint32_t>(xSE_PACKED_VERSION));
		kxf::Log::Info("Plugin directory: '{}', config directory: '{}'", m_PluginFS.GetLookupDirectory().GetFullPath(), m_ConfigFS.GetLookupDirectory().GetFullPath());
		kxf::Log::Info("Host process command line: '{}'", kxf::RunningSystemProcess::GetCurrentProcess().GetCommandLine());
	}
	void DLLApplication::SetupFramework()
	{
		// Load required native APIs
		using kxf::NativeAPISet;
		auto& nativeAPI = kxf::NativeAPILoader::GetInstance();
		nativeAPI.LoadLibraries({NativeAPISet::NtDLL});
		nativeAPI.Initialize();
	}
	void DLLApplication::SetupInfrastructure()
	{
		// Use ENB's EndFrame to process queued events for ENBInterface event handler
		// This handler is also skipped during general pending events processing.
		m_ENBInterface->Bind(ENBEvent::EvtEndFrame, [this](ENBEvent& event)
		{
			m_ENBInterface->ProcessPendingEvents();
		});
	}

	// Application::IPendingEvents
	bool DLLApplication::OnPendingEventHandlerProcess(IEvtHandler& evtHandler)
	{
		if (m_ENBInterface && &evtHandler == &m_ENBInterface.value())
		{
			return false;
		}
		return true;
	}

	// DLLApplication
	DLLApplication::DLLApplication()
	{
		SetName(kxf::StringViewOf(ProjectName));
		SetVendorName(kxf::StringViewOf(ProjectAuthor));
		SetVersion({VersionMajor, VersionMinor, VersionPatch});
	}

	// ICoreApplication
	bool DLLApplication::OnInit()
	{
		// Load config and setup logging
		auto config = LoadConfig();
		KX_SCOPEDLOG_FUNC;

		SetupFramework();
		LogInformation();

		// Init interfaces
		m_XSEInterface.emplace(*this, config);
		m_ENBInterface.emplace(*this, config);
		m_Redirector.emplace(*this, config);

		CallModuleInit(m_XSEInterface);
		CallModuleInit(m_ENBInterface);
		CallModuleInit(m_Redirector);
		SetupInfrastructure();

		KX_SCOPEDLOG.SetSuccess();
		return true;
	}
	void DLLApplication::OnExit()
	{
		KX_SCOPEDLOG_FUNC;

		CallModuleExit(m_Redirector);
		CallModuleExit(m_ENBInterface);
		CallModuleExit(m_XSEInterface);
		CoreApplication::OnExit();

		KX_SCOPEDLOG.SetSuccess();
	}

	// DLLApplication
	void DLLApplication::OnDLLEntry(void* handle, uint32_t eventType)
	{
		switch (eventType)
		{
			case DLL_THREAD_ATTACH:
			{
				ProcessPendingEventHandlers();
				ProcessEvent(DLLEvent::EvtThreadAttach);

				break;
			}
			case DLL_THREAD_DETACH:
			{
				ProcessPendingEventHandlers();
				ProcessEvent(DLLEvent::EvtThreadDetach);

				break;
			}
			case DLL_PROCESS_ATTACH:
			{
				ProcessPendingEventHandlers();
				ProcessEvent(DLLEvent::EvtProcessAttach);

				break;
			}
			case DLL_PROCESS_DETACH:
			{
				ProcessPendingEventHandlers();
				ProcessEvent(DLLEvent::EvtProcessDetach);

				break;
			}
		};
	}
}
