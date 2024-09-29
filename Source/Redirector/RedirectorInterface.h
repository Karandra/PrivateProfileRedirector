#pragma once
#include "Common.h"
#include "CommonWinAPI.h"
#include "AppModule.h"
#include "INIWrapper.h"
#include "AppConfigLoader.h"
#include "FunctionRedirector.h"
#include "FunctionTable.h"
#include "ConfigObject.h"
#include <kxf/Core/IEncodingConverter.h>
#include <kxf/FileSystem/NativeFileSystem.h>
#include <kxf/Threading/ReadWriteLock.h>
#include <kxf/Utility/String.h>

namespace PPR
{
	class DLLApplication;
}

namespace PPR
{
	class RedirectorInterface final: public AppModule
	{
		public:
			static RedirectorInterface& GetInstance();

		private:
			kxf::FlagSet<RedirectorOption> m_Options;
			std::unique_ptr<kxf::IEncodingConverter> m_EncodingConverter;
			FunctionTable m_Functions;
			int m_SaveOnWriteBuffer = 0;

			mutable kxf::ReadWriteLock m_INIMapLock;
			kxf::Utility::UnorderedMapNoCase<kxf::String, std::unique_ptr<ConfigObject>> m_INIMap;
			std::atomic<size_t> m_TotalWriteCount = 0;

		private:
			void LoadConfig(DLLApplication& app, const AppConfigLoader& config);

			void SaveFunctionPointers();
			void OverrideFunctions();
			void RestoreFunctions();
			void SetupIntegrations(DLLApplication& app);

		public:
			RedirectorInterface(DLLApplication& app, const AppConfigLoader& config);
			~RedirectorInterface();

		public:
			// AppModule
			void OnInit(DLLApplication& app) override;
			void OnExit(DLLApplication& app) override;
			
			// RedirectorInterface
			const FunctionTable& GetFunctionTable() const noexcept
			{
				return m_Functions;
			}
			kxf::IEncodingConverter& GetEncodingConverter() const noexcept
			{
				return *m_EncodingConverter;
			}

			std::optional<size_t> GetSaveOnWriteBuffer() const noexcept
			{
				if (m_SaveOnWriteBuffer > 0)
				{
					return m_SaveOnWriteBuffer;
				}
				return {};
			}
			bool IsOptionEnabled(RedirectorOption option) const noexcept
			{
				return m_Options.Contains(option);
			}

			ConfigObject& GetOrLoadFile(const kxf::String& filePath);
			size_t SaveChangedFiles(const wchar_t* message);
			size_t OnFileWrite(ConfigObject& configObject) noexcept;
			size_t RefreshINI();
	};
}
