#pragma once
#include "stdafx.h"
#include "ScriptExtenderDefines.h"
#include "Utility/KxDynamicString.h"
#include "IConsoleCommandOverrider.h"
#include "Qx/EventSystem/EvtHandler.h"
#include "Qx/EventSystem/Events/QxGameEvent.h"
#include "Qx/EventSystem/Events/QxConsoleEvent.h"

xSE_API(bool) xSE_QUERYFUNCTION(const xSE_Interface* xSE, PluginInfo* pluginInfo);
xSE_API(bool) xSE_LOADFUNCTION(const xSE_Interface* xSE);

namespace PPR
{
	class Redirector;
}

namespace PPR
{
	class SEInterface final: public QxEvtHandler
	{
		using PluginHandle = uint32_t;

		friend bool ::xSE_QUERYFUNCTION(const xSE_Interface*, PluginInfo*);
		friend bool ::xSE_LOADFUNCTION(const xSE_Interface*);

		public:
			static SEInterface& GetInstance();

		private:
			const xSE_Interface* m_XSE = nullptr;
			xSE_ScaleformInterface* m_Scaleform = nullptr;
			xSE_MessagingInterface* m_Messaging = nullptr;
			PluginHandle m_PluginHandle;
			bool m_CanUseSEFunctions = false;

			std::unique_ptr<IConsoleCommandOverrider> m_ConsoleCommandOverrider;

		private:
			bool OnCheckVersion(uint32_t interfaceVersion, uint32_t compiledVersion);
			bool OnQuery(PluginHandle pluginHandle, const xSE_Interface* xSE, xSE_ScaleformInterface* scaleform, xSE_MessagingInterface* messaging);
			bool OnLoad();

			bool DoPrintConsole(const char* string) const;
			void InitConsoleCommandOverrider();
			void InitGameMessageDispatcher();

			void OnConsoleCommand(QxConsoleEvent& event);
			void OnGameSave(QxGameEvent& event);

		private:
			SEInterface();

		public:
			bool CanUseSEFunctions() const
			{
				return m_CanUseSEFunctions;
			}
			PluginHandle GetPluginHandle() const
			{
				return m_PluginHandle;
			}
			
			const xSE_Interface* GetSEInterface() const
			{
				return m_XSE;
			}
			xSE_ScaleformInterface* GetScaleform() const
			{
				return m_Scaleform;
			}
			xSE_MessagingInterface* GetMessaging() const
			{
				return m_Messaging;
			}
	
		public:
			Redirector& GetRedirector() const;

			template<class T>
			T* GetConsoleCommandOverrider() const
			{
				static_assert(std::is_base_of_v<IConsoleCommandOverrider, T>);

				return static_cast<T*>(m_ConsoleCommandOverrider.get());
			}

			template<class... Args>
			void PrintConsole(const char* format, Args&&... arg) const
			{
				if constexpr ((sizeof...(Args)) != 0)
				{
					KxDynamicStringA buffer = KxDynamicStringA::Format(format, std::forward<Args>(arg)...);
					DoPrintConsole(buffer.data());
				}
				else
				{
					DoPrintConsole(format);
				}
			}
	};
}
