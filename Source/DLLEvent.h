#pragma once
#include "Common.h"
#include <kxf/EventSystem/Event.h>

namespace PPR
{
	class DLLEvent: public kxf::BasicEvent
	{
		public:
			kxf_EVENT_MEMBER(DLLEvent, ProcessAttach);
			kxf_EVENT_MEMBER(DLLEvent, ProcessDetach);

			kxf_EVENT_MEMBER(DLLEvent, ThreadAttach);
			kxf_EVENT_MEMBER(DLLEvent, ThreadDetach);

		public:
			DLLEvent() = default;
	};
}
