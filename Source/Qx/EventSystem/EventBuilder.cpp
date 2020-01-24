#include "EventBuilder.h"
#include "EvtHandler.h"
#include "Qx/Utility.h"
#include "Events/QxIdleEvent.h"

namespace Qx::EventSystem
{
	EventBuilder::~EventBuilder()
	{
		// If the event wasn't sent using 'EventBuilder::Do', send it here
		if (!m_Sent)
		{
			Do();
		}

		// Async events are allocated on the heap so we need to delete
		// the pointer if it's still here for some reason.
		if (m_Async)
		{
			delete m_Event;
		}
	}

	bool EventBuilder::Do()
	{
		if (m_Async)
		{
			delete m_Event;
			//QxEvent* event = Utility::ExchangeResetAndReturn(m_Event, nullptr);
			//m_EvtHandler->QueueEvent(std::unique_ptr<QxEvent>(event));

			m_Sent = true;
			return false;
		}
		else
		{
			const bool processed = m_EvtHandler->ProcessEvent(*m_Event);
			m_IsSkipped = m_Event->IsSkipped();
			m_IsAllowed = m_Event->IsAllowed();

			m_Sent = true;
			return processed;
		}
	}

	EventBuilder& EventBuilder::operator=(EventBuilder&& other)
	{
		const EventBuilder null;

		using Utility::ExchangeAndReset;
		ExchangeAndReset(m_EvtHandler, other.m_EvtHandler, null.m_EvtHandler);
		ExchangeAndReset(m_Event, other.m_Event, null.m_Event);
		ExchangeAndReset(m_Async, other.m_Async, null.m_Async);
		ExchangeAndReset(m_Sent, other.m_Sent, null.m_Sent);
		ExchangeAndReset(m_IsSkipped, other.m_IsSkipped, null.m_IsSkipped);
		ExchangeAndReset(m_IsAllowed, other.m_IsAllowed, null.m_IsAllowed);

		return *this;
	}
}
