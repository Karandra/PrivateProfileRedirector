#include "stdafx.h"
#include "ENBInterface.h"
#include "ENBEvent.h"
#include "PrivateProfileRedirector.h"

namespace PPR
{
	ENBInterface& ENBInterface::GetInstance() noexcept
	{
		static ENBInterface instance;
		return instance;
	}

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

	Redirector& ENBInterface::GetRedirector() const
	{
		return Redirector::GetInstance();
	}
}
