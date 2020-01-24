#pragma once
#include "Event.h"
class QxEventCallWrapper;

namespace Qx::EventSystem
{
	class EventItem final
	{
		private:
			QxEventCallWrapper* m_Handler = nullptr;
			QxEventID m_EventID = QxEvent::EvtNull;
			bool m_ShouldDelete = false;

		public:
			EventItem(QxEventID eventID, std::unique_ptr<QxEventCallWrapper> handler)
				:m_Handler(handler.release()), m_EventID(eventID), m_ShouldDelete(true)
			{
			}
			EventItem(QxEventID eventID, QxEventCallWrapper& handler)
				:m_Handler(&handler), m_EventID(eventID), m_ShouldDelete(false)
			{
			}
			EventItem(EventItem&& other)
			{
				*this = std::move(other);
			}
			EventItem(const EventItem&) = delete;
			~EventItem()
			{
				Clear();
			}

		public:
			const QxEventCallWrapper* GetHandler() const
			{
				return m_Handler;
			}
			QxEventCallWrapper* GetHandler()
			{
				return m_Handler;
			}
			QxEventID GetEventID() const
			{
				return m_EventID;
			}

			bool IsEmpty() const
			{
				return m_Handler == nullptr;
			}
			void Clear();

		public:
			EventItem& operator=(EventItem&& other);
			EventItem& operator=(const EventItem&) = delete;

			bool operator==(const EventItem& other) const;
			bool operator!=(const EventItem& other) const
			{
				return !(*this == other);
			}
	};
}
