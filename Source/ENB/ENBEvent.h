#pragma once
#include "Common.h"
#include <kxf/EventSystem/Event.h>

namespace PPR
{
	class ENBEvent: public kxf::BasicEvent
	{
		public:
			kxf_EVENT_MEMBER(ENBEvent, OnInit);
			kxf_EVENT_MEMBER(ENBEvent, OnExit);

			kxf_EVENT_MEMBER(ENBEvent, BeginFrame);
			kxf_EVENT_MEMBER(ENBEvent, EndFrame);

			kxf_EVENT_MEMBER(ENBEvent, PreSave);
			kxf_EVENT_MEMBER(ENBEvent, PostLoad);

			kxf_EVENT_MEMBER(ENBEvent, PreReset);
			kxf_EVENT_MEMBER(ENBEvent, PostReset);

		public:
			ENBEvent() = default;
	};
}
