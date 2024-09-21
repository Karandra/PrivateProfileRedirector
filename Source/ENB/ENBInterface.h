#pragma once
#include "Common.h"
#include "ENBAPI.h"
#include "ENBEvent.h"
#include <kxf/EventSystem/EvtHandler.h>

namespace PPR
{
	class Redirector;
}

namespace PPR
{
	class ENBInterface final: public kxf::EvtHandler
	{
		public:
			static ENBInterface& GetInstance() noexcept;

		private:
			ENBAPI::ENBLink m_ENBLink;
			
		protected:
			// IEvtHandler
			bool OnDynamicBind(EventItem& eventItem) override;

		private:
			ENBInterface()
			{
				m_ENBLink.Load();
			}
			
		public:
			Redirector& GetRedirector() const;
	};
}
