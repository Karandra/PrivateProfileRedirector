#pragma once
#include "Common.h"
#include "AppModule.h"
#include "AppConfigLoader.h"
#include "ScriptExtenderDefines.h"
#include "IConsoleCommandOverrider.h"
#include "GameEvent.h"
#include "ConsoleEvent.h"

xSE_API(bool) xSE_PRELOADFUNCTION(const xSE_Interface* xSE);
xSE_API(bool) xSE_QUERYFUNCTION(const xSE_Interface* xSE, PluginInfo* pluginInfo);
xSE_API(bool) xSE_LOADFUNCTION(const xSE_Interface* xSE);

namespace PPR
{
	class XSEInterface final: public AppModule
	{
		using PluginHandle = uint32_t;
		friend bool ::xSE_PRELOADFUNCTION(const xSE_Interface*);
		friend bool ::xSE_QUERYFUNCTION(const xSE_Interface*, PluginInfo*);
		friend bool ::xSE_LOADFUNCTION(const xSE_Interface*);

		public:
			static XSEInterface& GetInstance() noexcept;

		private:
			kxf::FlagSet<XSEOption> m_Options;

			const xSE_Interface* m_XSE = nullptr;
			xSE_ScaleformInterface* m_Scaleform = nullptr;
			xSE_MessagingInterface* m_Messaging = nullptr;
			PluginHandle m_PluginHandle;
			bool m_CanUseSEFunctions = false;

			std::unique_ptr<IConsoleCommandOverrider> m_ConsoleCommandOverrider;
			bool m_GameEventListenerRegistered = false;

		private:
			void LoadConfig(DLLApplication& app, const AppConfigLoader& config);

			bool OnCheckVersion(uint32_t interfaceVersion, uint32_t compiledVersion);
			bool OnQuery(PluginHandle pluginHandle, const xSE_Interface* xSE, xSE_ScaleformInterface* scaleform, xSE_MessagingInterface* messaging);
			bool OnLoad();

			bool DoPrintConsole(const char* string) const;
			void InitConsoleCommandOverrider();
			void InitGameMessageDispatcher();

		protected:
			// IEvtHandler
			bool OnDynamicBind(EventItem& eventItem) override;

		public:
			XSEInterface(DLLApplication& app, const AppConfigLoader& config);
			~XSEInterface();

		public:
			// AppModule
			void OnInit(DLLApplication& app) override;

			// XSEInterface
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
	
			template<class T> requires(std::is_base_of_v<IConsoleCommandOverrider, T>)
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
