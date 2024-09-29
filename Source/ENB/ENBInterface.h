#pragma once
#include "Common.h"
#include "AppModule.h"
#include "ENBAPI.h"
#include "ENBEvent.h"

namespace PPR
{
	class ENBInterface final: public AppModule
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
			// AppModule
			void OnInit(DLLApplication& app) override;

			// ENBInterface
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
