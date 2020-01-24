#pragma once
#include "Event.h"
class QxEvtHandler;

namespace Qx::EventSystem
{
	class EventBuilder
	{
		private:
			QxEvtHandler* m_EvtHandler = nullptr;
			QxEvent* m_Event = nullptr;
			bool m_Async = false;
			bool m_Sent = false;
			bool m_IsSkipped = false;
			bool m_IsAllowed = true;

		private:
			EventBuilder() = default;

		public:
			EventBuilder(QxEvtHandler& evtHandler, std::unique_ptr<QxEvent> event)
				:m_EvtHandler(&evtHandler), m_Event(event.release()), m_Async(true)
			{
			}
			EventBuilder(QxEvtHandler& evtHandler, QxEvent& event)
				:m_EvtHandler(&evtHandler), m_Event(&event), m_Async(false)
			{
			}
			EventBuilder(EventBuilder&& other)
			{
				*this = std::move(other);
			}
			EventBuilder(const EventBuilder&) = delete;
			virtual ~EventBuilder();

		public:
			bool Do();

			bool IsAsync() const
			{
				return m_Async;
			}
			bool IsSkipped() const
			{
				return m_IsSkipped;
			}
			bool IsAllowed() const
			{
				return m_IsAllowed;
			}

		public:
			EventBuilder& operator=(EventBuilder&& other);
			EventBuilder& operator=(const EventBuilder&) = delete;
	};
}

template<class TEvent, class... Args>
class QxEventBuilder: public Qx::EventSystem::EventBuilder
{
	private:
		TEvent m_Event;

	public:
		QxEventBuilder(QxEventTag<TEvent> eventTag, QxEvtHandler& evtHandler, Args&&... arg)
			:EventBuilder(evtHandler, m_Event), m_Event(std::forward<Args>(arg)...)
		{
			m_Event.SetEventID(eventTag);
			m_Event.AssignSender(evtHandler);
		}
		QxEventBuilder(QxEventBuilder&& other) = default;

	public:
		template<class TFunc>
		QxEventBuilder& On(TFunc&& func)
		{
			std::invoke(func, m_Event);
			return *this;
		}

		template<class TFunc>
		QxEventBuilder& Then(TFunc&& func)
		{
			m_Event.SetCallback([func = std::move(func)](QxEvtHandler& evtHandler, QxEvent& event)
			{
				std::invoke(func, static_cast<TEvent&>(event));
			});
			return *this;
		}

	public:
		QxEventBuilder& operator=(QxEventBuilder&& other) = default;
};
