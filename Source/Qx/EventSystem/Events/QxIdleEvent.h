#pragma once
#include "Qx/EventSystem/Event.h"

class QxIdleEvent: public QxEvent
{
	public:
		QxEVENT_MEMBER_DEFINE(QxIdleEvent, Idle);

	public:
		QxIdleEvent() = default;
};
