#include "stdafx.h"
#include "AppConfigLoader.h"

namespace PPR
{
	AppConfigLoader::AppConfigLoader(std::unique_ptr<kxf::IInputStream> stream)
	{
		if (stream && m_Config.Load(*stream))
		{
			m_General = m_Config.QueryElement("General");
			m_Redirector = m_Config.QueryElement("Redirector");
			m_XSEInterface = m_Config.QueryElement("XSE");
			m_ENBInterface = m_Config.QueryElement("ENB");
		}
	}
}
