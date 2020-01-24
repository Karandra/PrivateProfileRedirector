#include "EvtHandler.h"
#include "EventItem.h"
#include "CallWrapper.h"

namespace Qx::EventSystem
{
	void EventItem::Clear()
	{
		if (m_ShouldDelete)
		{
			delete m_Handler;
			m_Handler = nullptr;
			m_ShouldDelete = false;
		}
	}

	bool EventItem::operator==(const EventItem& other) const
	{
		if (this != &other)
		{
			const bool sameEventID = m_EventID == other.m_EventID;
			return sameEventID && (m_Handler == other.m_Handler || (m_Handler && other.m_Handler && m_Handler->IsSameAs(*other.m_Handler)));
		}
		return true;
	}
	EventItem& EventItem::operator=(EventItem&& other)
	{
		m_Handler = other.m_Handler;
		other.m_Handler = nullptr;

		m_ShouldDelete = other.m_ShouldDelete;
		other.m_ShouldDelete = false;

		m_EventID = other.m_EventID;
		other.m_EventID = QxEvent::EvtNull;

		return *this;
	}
}
