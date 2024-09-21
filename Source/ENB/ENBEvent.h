#pragma once
#include "Common.h"
#include <kxf/EventSystem/Event.h>

namespace PPR
{
	class ENBEvent: public kxf::BasicEvent
	{
		public:
			KxEVENT_MEMBER(ENBEvent, OnInit);
			KxEVENT_MEMBER(ENBEvent, OnExit);

			KxEVENT_MEMBER(ENBEvent, BeginFrame);
			KxEVENT_MEMBER(ENBEvent, EndFrame);

			KxEVENT_MEMBER(ENBEvent, PreSave);
			KxEVENT_MEMBER(ENBEvent, PostLoad);

			KxEVENT_MEMBER(ENBEvent, PreReset);
			KxEVENT_MEMBER(ENBEvent, PostReset);

		public:
			ENBEvent() = default;
	};
}
