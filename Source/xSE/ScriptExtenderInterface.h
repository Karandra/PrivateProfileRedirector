#pragma once
#include "stdafx.h"
#include "ScriptExtenderDefines.h"
#include "IConsoleCommandOverrider.h"
#include "GameEvent.h"
#include "ConsoleEvent.h"
#include <kxf/EventSystem/EvtHandler.h>

xSE_API(bool) xSE_PRELOADFUNCTION(const xSE_Interface* xSE);
xSE_API(bool) xSE_QUERYFUNCTION(const xSE_Interface* xSE, PluginInfo* pluginInfo);
xSE_API(bool) xSE_LOADFUNCTION(const xSE_Interface* xSE);

namespace PPR
{
	class Redirector;
}

namespace PPR
{
	class SEInterface final: public kxf::EvtHandler
	{
		using PluginHandle = uint32_t;

		friend bool ::xSE_PRELOADFUNCTION(const xSE_Interface*);
		friend bool ::xSE_QUERYFUNCTION(const xSE_Interface*, PluginInfo*);
		friend bool ::xSE_LOADFUNCTION(const xSE_Interface*);

		public:
			static SEInterface& GetInstance() noexcept;

		private:
			const xSE_Interface* m_XSE = nullptr;
			xSE_ScaleformInterface* m_Scaleform = nullptr;
			xSE_MessagingInterface* m_Messaging = nullptr;
			PluginHandle m_PluginHandle;
			bool m_CanUseSEFunctions = false;

			std::unique_ptr<IConsoleCommandOverrider> m_ConsoleCommandOverrider;
			bool m_GameEventListenerRegistered = false;

		private:
			bool OnCheckVersion(uint32_t interfaceVersion, uint32_t compiledVersion);
			bool OnQuery(PluginHandle pluginHandle, const xSE_Interface* xSE, xSE_ScaleformInterface* scaleform, xSE_MessagingInterface* messaging);
			bool OnLoad();

			bool DoPrintConsole(const char* string) const;
			void InitConsoleCommandOverrider();
			void InitGameMessageDispatcher();

			void OnConsoleCommand(ConsoleEvent& event);
			void OnGameSave(GameEvent& event);

		protected:
			// IEvtHandler
			bool OnDynamicBind(EventItem& eventItem) override;

		private:
			SEInterface() noexcept;

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
			requires(std::is_base_of_v<IConsoleCommandOverrider, T>)
			T* GetConsoleCommandOverrider() const noexcept
			{
				return static_cast<T*>(m_ConsoleCommandOverrider.get());
			}

			template<class TFormat, class... Args>
			void PrintConsole(const TFormat& format, Args&&... arg) const
			{
				auto formatted = kxf::Format(format, std::forward<Args>(arg)...);
				DoPrintConsole(formatted.utf8_str());
			}
	};
}
