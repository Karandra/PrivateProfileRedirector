#pragma once
#include "Event.h"
#include "EventItem.h"
#include "EventBuilder.h"
#include "EventFilter.h"
#include "CallWrapper.h"
#include <kxf/Utility/TypeTraits.h>
class QxEventFilter;
class QxEventCallWrapper;

class QxEvtHandler
{
	friend class QxCoreApplication;

	protected:
		using EventItem = Qx::EventSystem::EventItem;

	private:
		static QxEventFilter* ms_EventFilters;

	public:
		static void AddEventFilter(QxEventFilter& eventFilter);
		static bool RemoveEventFilter(QxEventFilter& eventFilter);

	private:
		// Dynamic events table
		std::vector<EventItem> m_EventTable;

		// Event handler chain
		QxEvtHandler* m_PrevHandler = nullptr;
		QxEvtHandler* m_NextHandler = nullptr;

		// Enabled/disabled switch
		bool m_IsEnabled = true;

	private:
		bool DoBind(QxEventID eventID, std::unique_ptr<QxEventCallWrapper> eventHandler);
		bool DoUnbind(QxEventID eventID, QxEventCallWrapper& eventHandler);

		bool SearchEventTable(QxEvent& event);
		bool ExecuteEventHandler(QxEvent& event, EventItem& eventItem, QxEvtHandler& evtHandler);
		void CallEventHandler(QxEvtHandler& evtHandler, QxEvent& event, QxEventCallWrapper& callWrapper);
		
		bool DoTryApp(QxEvent& event);
		bool DoTryChain(QxEvent& event);
		bool TryHereOnly(QxEvent& event);
		bool TryBeforeAndHere(QxEvent& event);
		bool ProcessEventLocally(QxEvent& event);

	protected:
		void DoQueueEvent(std::unique_ptr<QxEvent> event);
		bool DoProcessEvent(QxEvent& event);

		virtual bool OnDynamicBind(EventItem& eventItem)
		{
			return true;
		}
		virtual bool OnDynamicUnbind(const EventItem& eventItem)
		{
			return true;
		}

		virtual bool TryBefore(QxEvent& event);
		virtual bool TryAfter(QxEvent& event);

	public:
		QxEvtHandler() = default;
		QxEvtHandler(QxEvtHandler&& other)
		{
			*this = std::move(other);
		}
		QxEvtHandler(const QxEvtHandler&) = delete;
		virtual ~QxEvtHandler()
		{
			Unlink();
		}

	public:
		bool IsEvtHandlerEnabled() const
		{
			return m_IsEnabled;
		}
		void EnableEvtHandler(bool enabled = true)
		{
			m_IsEnabled = enabled;
		}

	public:
		// Event handler chain
		QxEvtHandler* GetNextHandler() const
		{
			return m_NextHandler;
		}
		QxEvtHandler* GetPreviousHandler() const
		{
			return m_PrevHandler;
		}
		void SetNextHandler(QxEvtHandler* evtHandler)
		{
			m_NextHandler = evtHandler;
		}
		void SetPrevHandler(QxEvtHandler* evtHandler)
		{
			m_PrevHandler = evtHandler;
		}

		void Unlink();
		bool IsUnlinked() const;

	public:
		// Bind free or static function
		template<class TEvent, class TEventArg>
		bool Bind(QxEventTag<TEvent> eventTag, void(*func)(TEventArg&))
		{
			return DoBind(eventTag, std::make_unique<Qx::EventSystem::FunctionWrapper<QxEventTag<TEvent>>>(func));
		}
		
		// Unbind free or static function
		template<class TEvent, class TEventArg>
		bool Unbind(QxEventTag<TEvent> eventTag, void(*func)(TEventArg&))
		{
			Qx::EventSystem::FunctionWrapper<TEvent> callWrapper(func);
			return DoUnbind(eventTag, callWrapper);
		}

		// Bind functor or a lambda function
		template<class TEvent, class TFunctor>
		bool Bind(QxEventTag<TEvent> eventTag, const TFunctor& func)
		{
			return DoBind(eventTag, std::make_unique<Qx::EventSystem::FunctorWrapper<TEvent, TFunctor>>(func));
		}

		// Unbind functor or lambda function
		template<class TEvent, class TFunctor>
		bool Unbind(QxEventTag<TEvent> eventTag, const TFunctor& func)
		{
			Qx::EventSystem::FunctorWrapper<TEvent, TFunctor> callWrapper(func);
			return DoUnbind(eventTag, callWrapper);
		}

		// Bind a member function
		template<class TEvent, class TClass, class TEventArg, class TEventHandler>
		bool Bind(QxEventTag<TEvent> eventTag, void(TClass::* method)(TEventArg&), TEventHandler* handler)
		{
			return DoBind(eventTag, std::make_unique<Qx::EventSystem::MethodWrapper<TEvent, TClass, TEventArg, TEventHandler>>(method, handler));
		}

		// Unbind a member function
		template<class TEvent, class TClass, class TEventArg, class TEventHandler>
		bool Unbind(QxEventTag<TEvent> eventTag, void(TClass::* method)(TEventArg&), TEventHandler* handler)
		{
			Qx::EventSystem::MethodWrapper<TEvent, TClass, TEventArg, TEventHandler> callWrapper(method, handler);
			return DoUnbind(eventTag, callWrapper);
		}

	public:
		// Regular event sending functions
		bool ProcessEvent(QxEvent& event)
		{
			event.AssignSender(*this);
			return DoProcessEvent(event);
		}
		bool ProcessEvent(QxEvent& event, QxEventID eventID)
		{
			event.SetEventID(eventID);
			event.AssignSender(*this);
			return DoProcessEvent(event);
		}

		template<class TEvent, class... Args>
		bool ProcessEvent(QxEventTag<TEvent> eventTag, Args&&... arg)
		{
			TEvent event(std::forward<Args>(arg)...);
			event.SetEventID(eventTag);
			event.AssignSender(*this);

			return DoProcessEvent(event);
		}

		// Construct and send the event using event builder
		template<class TEvent, class... Args>
		auto ProcessEventEx(QxEventTag<TEvent> eventTag, Args&&... arg)
		{
			return QxEventBuilder<TEvent, Args...>(eventTag, *this, std::forward<Args>(arg)...);
		}

	public:
		QxEvtHandler& operator=(QxEvtHandler&& other);
};
