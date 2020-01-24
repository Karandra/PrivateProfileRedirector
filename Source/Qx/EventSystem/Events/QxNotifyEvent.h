#pragma once
#include "Qx/EventSystem/Event.h"

class QxNotifyEvent: public QxEvent
{
	public:
		QxNotifyEvent(QxEventID eventID = EvtNull)
			:QxEvent(eventID)
		{
		}
};
