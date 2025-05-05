#include "stdafx.h"
#include "ENBInterface.h"
#include "ENBEvent.h"

namespace PPR
{
	// IEvtHandler
	bool ENBInterface::OnDynamicBind(EventItem& eventItem)
	{
		auto id = eventItem.GetEventID();
		if (id.IsOfEventClass<ENBEvent>())
		{
			m_ENBLink.BindCallback(*this);
		}
		return EvtHandler::OnDynamicBind(eventItem);
	}
	
	// ENBInterface
	ENBInterface::ENBInterface(DLLApplication& app, const AppConfigLoader& config)
	{
		KXF_SCOPEDLOG_FUNC;

		bool loaded = m_ENBLink.Load();

		KXF_SCOPEDLOG.SetSuccess(loaded);
	}
	ENBInterface::~ENBInterface()
	{
		KXF_SCOPEDLOG_FUNC;
		KXF_SCOPEDLOG.SetSuccess();
	}

	// AppModule
	void ENBInterface::OnInit(DLLApplication& app)
	{
		KXF_SCOPEDLOG_FUNC;
		KXF_SCOPEDLOG.SetSuccess();
	}
}
