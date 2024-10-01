#include "stdafx.h"
#include "ENBAPI.h"
#include "ENBEvent.h"
#include <kxf/System/Win32Error.h>
#include <kxf/FileSystem/NativeFileSystem.h>

namespace ENBAPI
{
	bool ENBLink::Load(const kxf::FSPath& filePath)
	{
		KX_SCOPEDLOG_ARGS(filePath.GetFullPath(), m_IsLoaded);

		if (m_IsLoaded)
		{
			KX_SCOPEDLOG.Warning().Log("Already loaded");
			return false;
		}

		if (filePath.IsNull())
		{
			auto rootPath = kxf::NativeFileSystem::GetExecutingModuleRootDirectory();
			m_ENBLib.Load(rootPath / "d3d11.dll") || m_ENBLib.Load(rootPath / "d3d10.dll") || m_ENBLib.Load(rootPath / "d3d9.dll");
		}
		else
		{
			if (filePath.IsAbsolute())
			{
				m_ENBLib.Load(filePath);
			}
			else
			{
				m_ENBLib.Load(kxf::NativeFileSystem::GetExecutingModuleRootDirectory() / filePath);
			}
		}

		if (m_ENBLib)
		{
			m_ENBGetVersion = m_ENBLib.GetExportedFunction<decltype(m_ENBGetVersion)>("ENBGetVersion");
			m_ENBGetSDKVersion = m_ENBLib.GetExportedFunction<decltype(m_ENBGetSDKVersion)>("ENBGetSDKVersion");
			m_ENBSetCallbackFunction = m_ENBLib.GetExportedFunction<decltype(m_ENBSetCallbackFunction)>("ENBSetCallbackFunction");
			m_ENBGetParameter = m_ENBLib.GetExportedFunction<decltype(m_ENBGetParameter)>("ENBGetParameter");
			m_ENBSetParameter = m_ENBLib.GetExportedFunction<decltype(m_ENBSetParameter)>("ENBSetParameter");
		}
		else
		{
			KX_SCOPEDLOG.Error().Format("Couldn't load ENB library", kxf::Win32Error::GetLastError());
		}

		if (m_ENBLib.IsNull() || !m_ENBGetVersion || !m_ENBGetSDKVersion || !m_ENBSetCallbackFunction || !m_ENBGetParameter || !m_ENBSetParameter)
		{
			m_IsLoaded = false;
			KX_SCOPEDLOG.Warning().Format("Couldn't load ENB API functions", kxf::Win32Error::GetLastError());
			KX_SCOPEDLOG.Warning().Format("ENBLib=[{}/'{}'], ENBGetVersion=[{}], ENBGetSDKVersion=[{}], ENBSetCallbackFunction=[{}], ENBGetParameter=[{}], ENBSetParameter=[{}]",
										  m_ENBLib.GetHandle(),
										  m_ENBLib ? m_ENBLib.GetFilePath().GetFullPath() : "<null>",
										  static_cast<void*>(m_ENBGetVersion),
										  static_cast<void*>(m_ENBGetSDKVersion),
										  static_cast<void*>(m_ENBSetCallbackFunction),
										  static_cast<void*>(m_ENBGetParameter),
										  static_cast<void*>(m_ENBSetParameter)
			);

			return false;
		}
		else
		{
			m_IsLoaded = true;
			KX_SCOPEDLOG.Info().Format("ENB API loaded successfully from '{}'. Version: {}, SDK version: {}", m_ENBLib.GetFilePath().GetFullPath(), m_ENBGetVersion(), m_ENBGetSDKVersion());
			KX_SCOPEDLOG.SetSuccess();

			return true;
		}
	}
	
	void ENBLink::BindCallback(kxf::IEvtHandler& evtHandler)
	{
		using namespace PPR;

		if (m_IsLoaded && !m_CallbackStore)
		{
			m_CallbackStore.SetCallable([&](CallbackType type)
			{
				switch (type)
				{
					case CallbackType::OnInit:
					{
						kxf::Log::InfoCategory(LogCategory::ENBInterface, "Callback: OnInit", type);
						evtHandler.ProcessEvent(ENBEvent::EvtOnInit);

						break;
					}
					case CallbackType::OnExit:
					{
						kxf::Log::InfoCategory(LogCategory::ENBInterface, "Callback: OnExit", type);
						evtHandler.ProcessEvent(ENBEvent::EvtOnExit);

						break;
					}

					// Don't log these two events as they only clog the log file
					case CallbackType::BeginFrame:
					{
						evtHandler.ProcessEvent(ENBEvent::EvtBeginFrame);
						break;
					}
					case CallbackType::EndFrame:
					{
						evtHandler.ProcessEvent(ENBEvent::EvtEndFrame);
						break;
					}

					case CallbackType::PreSave:
					{
						kxf::Log::InfoCategory(LogCategory::ENBInterface, "Callback: PreSave", type);
						evtHandler.ProcessEvent(ENBEvent::EvtPreSave);

						break;
					}
					case CallbackType::PostLoad:
					{
						kxf::Log::InfoCategory(LogCategory::ENBInterface, "Callback: PostLoad", type);
						evtHandler.ProcessEvent(ENBEvent::EvtPostLoad);

						break;
					}

					case CallbackType::PreReset:
					{
						kxf::Log::InfoCategory(LogCategory::ENBInterface, "Callback: PreReset", type);
						evtHandler.ProcessEvent(ENBEvent::EvtPreReset);

						break;
					}
					case CallbackType::PostReset:
					{
						kxf::Log::InfoCategory(LogCategory::ENBInterface, "Callback: PostReset", type);
						evtHandler.ProcessEvent(ENBEvent::EvtPostReset);

						break;
					}
					default:
					{
						kxf::Log::WarningCategory(LogCategory::ENBInterface, "Unknown callback: {}", type);
					}
				};
			});
			m_ENBSetCallbackFunction(m_CallbackStore.GetFunctionPointer());
		}
	}
	
	std::optional<ParameterValue> ENBLink::RawGetParameter(const kxf::String& fileName, const kxf::String& category, const kxf::String& keyName) const
	{
		if (m_IsLoaded)
		{
			ParameterValue value;
			if (m_ENBGetParameter(fileName.nc_str(), category.nc_str(), keyName.nc_str(), &value))
			{
				return value;
			}
		}
		return {};
	}
	bool ENBLink::RawSetParameter(const kxf::String& fileName, const kxf::String& category, const kxf::String& keyName, const ParameterValue& value)
	{
		if (m_IsLoaded)
		{
			ParameterValue temp = value;
			if (m_ENBSetParameter(fileName.nc_str(), category.nc_str(), keyName.nc_str(), &temp))
			{
				return true;
			}
		}
		return false;
	}
}
