#include "Event.h"
#include <atomic>

QxEventID QxEvent::NewEventID()
{
	static std::atomic<QxEventID> g_EventCounter(0);
	return ++g_EventCounter;
}

QxEvent& QxEvent::operator=(const QxEvent& other)
{
	m_Callback = other.m_Callback;
	m_EventSender = other.m_EventSender;
	m_EventID = other.m_EventID;
	m_ClientData = other.m_ClientData;
	m_IsSkipped = other.m_IsSkipped;
	m_IsAllowed = other.m_IsAllowed;
	m_CanDisallow = other.m_CanDisallow;
	m_ExceptionThrown = other.m_ExceptionThrown;

	return *this;
}
QxEvent& QxEvent::operator=(QxEvent&& other)
{
	m_Callback = std::move(other.m_Callback);

	using Qx::Utility::ExchangeAndReset;
	ExchangeAndReset(m_EventSender, other.m_EventSender, nullptr);
	ExchangeAndReset(m_EventID, other.m_EventID, QxEvent::EvtNull);
	ExchangeAndReset(m_ClientData, other.m_ClientData, nullptr);
	ExchangeAndReset(m_IsSkipped, other.m_IsSkipped, false);
	ExchangeAndReset(m_IsAllowed, other.m_IsAllowed, true);
	ExchangeAndReset(m_CanDisallow, other.m_CanDisallow, false);
	ExchangeAndReset(m_ExceptionThrown, other.m_ExceptionThrown, false);

	return *this;
}
