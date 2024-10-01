#pragma once
#include "Common.h"
#include <kxf/EventSystem/Event.h>

namespace PPR
{
	class DLLEvent: public kxf::BasicEvent
	{
		public:
			KxEVENT_MEMBER(DLLEvent, ProcessAttach);
			KxEVENT_MEMBER(DLLEvent, ProcessDetach);

			KxEVENT_MEMBER(DLLEvent, ThreadAttach);
			KxEVENT_MEMBER(DLLEvent, ThreadDetach);

		public:
			DLLEvent() = default;
	};
}
