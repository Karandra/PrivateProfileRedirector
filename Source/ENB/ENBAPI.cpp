#include "stdafx.h"
#include "ENBAPI.h"
#include "ENBEvent.h"
#include <kxf/System/Win32Error.h>

namespace ENBAPI
{
	bool ENBLink::Load(const kxf::String& filePath)
	{
		KX_SCOPEDLOG_ARGS(filePath, m_IsLoaded);

		if (m_IsLoaded)
		{
			KX_SCOPEDLOG.Warning().Log("Already loaded");
			return false;
		}

		if (filePath.IsEmpty())
		{
			m_ENBLib.Load("d3d11.dll");
		}
		else
		{
			m_ENBLib.Load(filePath);
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

			return false;
		}
		else
		{
			m_IsLoaded = true;
			KX_SCOPEDLOG.Info().Format("ENB API loaded successfully. Version: {}, SDK version: {}", m_ENBGetVersion(), m_ENBGetSDKVersion());
			KX_SCOPEDLOG.SetSuccess();

			return true;
		}
	}
	
	void ENBLink::BindCallback(kxf::IEvtHandler& evtHandler)
	{
		if (m_IsLoaded && !m_CallbackStore)
		{
			m_CallbackStore.SetCallable([&](CallbackType type)
			{
				switch (type)
				{
					case CallbackType::OnInit:
					{
						evtHandler.ProcessEvent(PPR::ENBEvent::EvtOnInit);
						break;
					}
					case CallbackType::OnExit:
					{
						evtHandler.ProcessEvent(PPR::ENBEvent::EvtOnExit);
						break;
					}

					case CallbackType::BeginFrame:
					{
						evtHandler.ProcessEvent(PPR::ENBEvent::EvtBeginFrame);
						break;
					}
					case CallbackType::EndFrame:
					{
						evtHandler.ProcessEvent(PPR::ENBEvent::EvtEndFrame);
						break;
					}

					case CallbackType::PreSave:
					{
						evtHandler.ProcessEvent(PPR::ENBEvent::EvtPreSave);
						break;
					}
					case CallbackType::PostLoad:
					{
						evtHandler.ProcessEvent(PPR::ENBEvent::EvtPostLoad);
						break;
					}

					case CallbackType::PreReset:
					{
						evtHandler.ProcessEvent(PPR::ENBEvent::EvtPreReset);
						break;
					}
					case CallbackType::PostReset:
					{
						evtHandler.ProcessEvent(PPR::ENBEvent::EvtPostReset);
						break;
					}
					default:
					{
						kxf::Log::WarningCategory("ENB API", "Unknown callback type: {}", type);
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
