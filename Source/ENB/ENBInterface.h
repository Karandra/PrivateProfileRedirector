#pragma once
#include "Common.h"
#include "ENBAPI.h"
#include "ENBEvent.h"
#include <kxf/EventSystem/EvtHandler.h>

namespace PPR
{
	class AppConfigLoader;
	class DLLApplication;
}

namespace PPR
{
	class ENBInterface final: public kxf::EvtHandler
	{
		private:
			ENBAPI::ENBLink m_ENBLink;
			
		protected:
			// IEvtHandler
			bool OnDynamicBind(EventItem& eventItem) override;

		public:
			ENBInterface(DLLApplication& app, const AppConfigLoader& config);
			~ENBInterface();

		public:
			const ENBAPI::ENBLink& GetLink() const
			{
				return m_ENBLink;
			}
			ENBAPI::ENBLink& GetLink()
			{
				return m_ENBLink;
			}
	};
}
