#include "Common.h"
#include "CommonWinAPI.h"
#include "PrivateProfileRedirector.h"
#include "xSE/ScriptExtenderInterface.h"
#include "ENB/ENBInterface.h"
#include <kxf/Application/AttachedApplication.h>
#include <kxf/Localization/WindowsLocalizationPackage.h>

namespace PPR
{
	class AppConfigLoader;
}

namespace PPR
{
	class DLLApplication final: public kxf::AttachedApplication
	{
		public:
			static DLLApplication& GetInstance() noexcept;
			static void OnProcessAttach();

		private:
			std::optional<Redirector> m_Redirector;
			std::optional<XSEInterface> m_XSEInterface;
			std::optional<ENBInterface> m_ENBInterface;

			kxf::NativeFileSystem m_ConfigFS;
			kxf::NativeFileSystem m_PluginFS;
			kxf::WindowsLocalizationPackage m_LocatizationPackage;

		private:
			AppConfigLoader LoadConfig();
			bool OpenLog(kxf::LogLevel logLevel);

			void LogInformation();
			void SetupInfrastructure();

		protected:
			// Application::IPendingEvents
			bool OnPendingEventHandlerProcess(IEvtHandler& evtHandler) override;

		public:
			DLLApplication();

		public:
			// ICoreApplication
			bool OnInit() override;
			void OnExit() override;
			const kxf::ILocalizationPackage& GetLocalizationPackage() const override
			{
				return m_LocatizationPackage;
			}

			// DLLApplication
			Redirector& GetRedirector() noexcept
			{
				return *m_Redirector;
			}
			XSEInterface& GetXSEInterface() noexcept
			{
				return *m_XSEInterface;
			}
			ENBInterface& GetENBInterface() noexcept
			{
				return *m_ENBInterface;
			}

			void OnDLLEntry(void* handle , uint32_t eventType);
	};
}
