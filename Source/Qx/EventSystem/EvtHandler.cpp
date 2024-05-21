#include "EvtHandler.h"
#include "CallWrapper.h"
#include "EventFilter.h"
#include "Qx/Utility.h"
#include "Events/QxIdleEvent.h"
#include <algorithm>

QxEventFilter* QxEvtHandler::ms_EventFilters = nullptr;

void QxEvtHandler::AddEventFilter(QxEventFilter& eventFilter)
{
	eventFilter.m_Next = ms_EventFilters;
	ms_EventFilters = &eventFilter;
}
bool QxEvtHandler::RemoveEventFilter(QxEventFilter& eventFilter)
{
	QxEventFilter* prev = nullptr;
	for (QxEventFilter* currentFilter = ms_EventFilters; currentFilter; currentFilter = currentFilter->m_Next)
	{
		if (currentFilter == &eventFilter)
		{
			// Set the previous list element or the list head to the next element.
			if (prev)
			{
				prev->m_Next = currentFilter->m_Next;
			}
			else
			{
				ms_EventFilters = currentFilter->m_Next;
			}

			// Also reset the next pointer in the filter itself just to avoid having
			// possibly dangling pointers, even though it's not strictly necessary.
			currentFilter->m_Next = nullptr;
			return true;
		}
		prev = currentFilter;
	}
	return false;
}

bool QxEvtHandler::DoBind(QxEventID eventID, std::unique_ptr<QxEventCallWrapper> eventHandler)
{
	if (eventHandler && eventID != QxEvent::EvtNull)
	{
		EventItem eventItem(eventID, std::move(eventHandler));
		if (OnDynamicBind(eventItem))
		{
			m_EventTable.emplace_back(std::move(eventItem));
		}
		return true;
	}
	return false;
}
bool QxEvtHandler::DoUnbind(QxEventID eventID, QxEventCallWrapper& eventHandler)
{
	EventItem eventItem(eventID, eventHandler);
	if (eventID != QxEvent::EvtNull)
	{
		size_t deletedCount = 0;
		for (auto it = m_EventTable.rbegin(); it != m_EventTable.rend(); ++it)
		{
			EventItem& item = *it;

			// Compare handlers and clear the handler if EvtHandler allows that
			if (item == eventItem && OnDynamicUnbind(item))
			{
				item.Clear();
				deletedCount++;
			}
		}
		return deletedCount != 0;
	}
	return false;
}

bool QxEvtHandler::SearchEventTable(QxEvent& event)
{
	bool clearEmpty = false;
	for (auto it = m_EventTable.rbegin(); it != m_EventTable.rend(); ++it)
	{
		EventItem& item = *it;
		if (item.IsEmpty())
		{
			// This entry must have been unbound at some time in the past, so
			// skip it now and really remove it from the event table, once we
			// finish iterating.
			clearEmpty = true;
			continue;
		}

		if (item.GetEventID() == event.GetEventID())
		{
			QxEvtHandler* evtHandler = item.GetHandler()->GetEvtHandler();
			if (!evtHandler)
			{
				evtHandler = this;
			}

			if (ExecuteEventHandler(event, item, *evtHandler))
			{
				// It's important to skip clearing of the unbound event entries
				// below because this object itself could have been deleted by
				// the event handler making m_EventTable a dangling pointer
				// which can't be accessed any longer in the code below.
				//
				// In practice, it hopefully shouldn't be a problem to wait
				// until we get an event that we don't handle before clearing
				// because this should happen soon enough and even if it
				// doesn't the worst possible outcome is slightly increased
				// memory consumption while not skipping clearing can result in
				// hard to reproduce crashes (because it requires the disconnection
				// and deletion happen at the same time which is not always the
				// case).
				return true;
			}
		}
	}
	
	if (clearEmpty)
	{
		auto it = std::remove_if(m_EventTable.begin(), m_EventTable.end(), [](const EventItem& item)
		{
			return item.IsEmpty();
		});
		m_EventTable.erase(it, m_EventTable.end());
		m_EventTable.shrink_to_fit();
	}
	return false;
}
bool QxEvtHandler::ExecuteEventHandler(QxEvent& event, EventItem& eventItem, QxEvtHandler& evtHandler)
{
	// Reset skip instruction
	event.Skip(false);

	// Call the handler
	CallEventHandler(*event.GetSender(), event, *eventItem.GetHandler());

	// Return true if we processed this event and event handler itself didn't skipped it
	return !event.IsSkipped();
}
void QxEvtHandler::CallEventHandler(QxEvtHandler& evtHandler, QxEvent& event, QxEventCallWrapper& callWrapper)
{
	callWrapper.Execute(evtHandler, event);

	// If event wasn't skipped, call callback for this event
	// and restore any possible changes in skip state
	if (const bool isSkipped = event.IsSkipped(); !isSkipped)
	{
		event.ExecuteCallback(evtHandler);
		event.Skip(isSkipped);
	}
}

bool QxEvtHandler::DoTryApp(QxEvent& event)
{
	return false;
}
bool QxEvtHandler::DoTryChain(QxEvent& event)
{
	for (QxEvtHandler* evtHandler = m_NextHandler; evtHandler; evtHandler = evtHandler->m_NextHandler)
	{
		// We need to process this event at the level of this handler only
		// right now, the pre-/post-processing was either already done by
		// ProcessEvent() from which we were called or will be done by it when
		// we return.
		//
		// However we must call ProcessEvent() and not TryHereOnly() because the
		// existing code (including some in wxWidgets itself) expects the
		// overridden ProcessEvent() in its custom event handlers pushed on a
		// window to be called.
		//
		// So we must call ProcessEvent() but it must not do what it usually
		// does. To resolve this paradox we set up a special flag inside the
		// object itself to let ProcessEvent() know that it shouldn't do any
		// pre/post-processing for this event if it gets it. Note that this
		// only applies to this handler, if the event is passed to another one
		// by explicitly calling its ProcessEvent(), pre/post-processing should
		// be done as usual.

		class TargetHandlerChnager final
		{
			private:
				QxEvent& m_Event;
				QxEvtHandler* m_OldEvtHandler = nullptr;

			public:
				TargetHandlerChnager(QxEvent& event, QxEvtHandler& evtHandler)
					:m_Event(event), m_OldEvtHandler(event.m_TargetHandler)
				{
					m_Event.m_TargetHandler = &evtHandler;
				}
				~TargetHandlerChnager()
				{
					m_Event.m_TargetHandler = m_OldEvtHandler;
				}
		};

		TargetHandlerChnager targetChanger(event, *evtHandler);
		if (evtHandler->DoProcessEvent(event))
		{
			// Make sure "skipped" flag is not set as the event was really
			// processed in this case. Normally it shouldn't be set anyhow but
			// make sure just in case the user code does something strange.

			event.Skip(false);
			return true;
		}

		if (!event.ShouldProcessOnlyIn(*evtHandler))
		{
			// Still return true to indicate that no further processing should
			// be undertaken but ensure that "skipped" flag is set so that the
			// caller knows that the event was not really processed.

			event.Skip();
			return true;
		}
	}
	return false;
}
bool QxEvtHandler::TryHereOnly(QxEvent& event)
{
	// If the event handler is disabled it doesn't process any events
	if (IsEvtHandlerEnabled())
	{
		// Handle dynamic event table
		if (!m_EventTable.empty() && SearchEventTable(event))
		{
			return true;
		}
	}

	// We don't have a handler for this event.
	return false;
}
bool QxEvtHandler::TryBeforeAndHere(QxEvent& event)
{
	// Another helper which simply calls pre-processing hook and then tries to
	// handle the event at this handler level.
	return TryBefore(event) || TryHereOnly(event);
}
bool QxEvtHandler::ProcessEventLocally(QxEvent& event)
{
	// Try the hooks which should be called before our own handlers and this
	// handler itself first. Notice that we should not call ProcessEvent() on
	// this one as we're already called from it, which explains why we do it
	// here and not in DoTryChain()
	return TryBeforeAndHere(event) || DoTryChain(event);
}

void QxEvtHandler::DoQueueEvent(std::unique_ptr<QxEvent> event)
{
}
bool QxEvtHandler::DoProcessEvent(QxEvent& event)
{
	// The very first thing we do is to allow any registered filters to hook
	// into event processing in order to globally pre-process all events.
	//
	// Note that we should only do it if we're the first event handler called
	// to avoid calling FilterEvent() multiple times as the event goes through
	// the event handler chain and possibly upwards the window hierarchy.
	if (!event.WasProcessed())
	{
		for (QxEventFilter* eventFilter = ms_EventFilters; eventFilter; eventFilter = eventFilter->m_Next)
		{
			using Result = QxEventFilter::Result;

			const Result result = eventFilter->FilterEvent(event);
			if (result != Result::Skip)
			{
				return result != Result::Ignore;
			}
			// Proceed normally
		}
	}

	// Short circuit the event processing logic if we're requested to process
	// this event in this handler only, see DoTryChain() for more details.
	if (event.ShouldProcessOnlyIn(*this))
	{
		return TryBeforeAndHere(event);
	}

	// Try to process the event in this handler itself.
	if (ProcessEventLocally(event))
	{
		// It is possible that DoTryChain() called from ProcessEventLocally()
		// returned true but the event was not really processed: this happens
		// if a custom handler ignores the request to process the event in this
		// handler only and in this case we should skip the post processing
		// done in TryAfter() but still return the correct value ourselves to
		// indicate whether we did or did not find a handler for this event.
		return !event.IsSkipped();
	}

	// If we still didn't find a handler, propagate the event upwards the
	// window chain and/or to the application object.
	if (TryAfter(event))
	{
		return true;
	}

	// No handler found anywhere, bail out.
	return false;
}

bool QxEvtHandler::TryBefore(QxEvent& event)
{
	return false;
}
bool QxEvtHandler::TryAfter(QxEvent& event)
{
	// We only want to pass the window to the application object once even if
	// there are several chained handlers. Ensure that this is what happens by
	// only calling DoTryApp() if there is no next handler (which would do it).
	//
	// Notice that, unlike simply calling TryAfter() on the last handler in the
	// chain only from ProcessEvent(), this also works with wxWindow object in
	// the middle of the chain: its overridden TryAfter() will still be called
	// and propagate the event upwards the window hierarchy even if it's not
	// the last one in the chain (which, admittedly, shouldn't happen often).

	if (m_NextHandler)
	{
		return m_NextHandler->TryAfter(event);
	}

	// If this event is going to be processed in another handler next, don't pass it
	// to the application object now, it will be done from TryAfter() of this other handler.
	if (event.WillBeProcessedAgain())
	{
		return false;
	}
	return DoTryApp(event);
}

void QxEvtHandler::Unlink()
{
	// This event handler must take itself out of the chain of handlers:
	if (m_PrevHandler)
	{
		m_PrevHandler->SetNextHandler(m_NextHandler);
	}
	if (m_NextHandler)
	{
		m_NextHandler->SetPrevHandler(m_PrevHandler);
	}

	m_PrevHandler = nullptr;
	m_NextHandler = nullptr;
}
bool QxEvtHandler::IsUnlinked() const
{
	return m_PrevHandler == nullptr && m_NextHandler == nullptr;
}

QxEvtHandler& QxEvtHandler::operator=(QxEvtHandler&& other)
{
	using Qx::Utility::ExchangeAndReset;

	m_EventTable = std::move(other.m_EventTable);
	ExchangeAndReset(m_PrevHandler, other.m_PrevHandler, nullptr);
	ExchangeAndReset(m_NextHandler, other.m_NextHandler, nullptr);
	ExchangeAndReset(m_IsEnabled, other.m_IsEnabled, true);

	return *this;
}
