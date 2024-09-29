#pragma once
#include "Common.h"
#include <kxf/EventSystem/EvtHandler.h>

namespace PPR
{
	class DLLApplication;
	class AppConfigLoader;
}

namespace PPR
{
	class AppModule: public kxf::EvtHandler
	{
		public:
			AppModule() noexcept = default;

		public:
			virtual void OnInit(DLLApplication& app) = 0;
			virtual void OnExit(DLLApplication& app)
			{
			}
	};
}
