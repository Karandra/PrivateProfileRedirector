#pragma once
#include "stdafx.h"
#include "INIWrapper.h"
#include "RedirectorConfig.h"
#include "FunctionRedirector.h"
#include "FunctionTable.h"
#include "ConfigObject.h"
#include <kxf/Core/IEncodingConverter.h>
#include <kxf/FileSystem/NativeFileSystem.h>
#include <kxf/Threading/ReadWriteLock.h>
#include <kxf/Utility/String.h>

namespace PPR
{
	class SEInterface;
}

namespace PPR
{
	class Redirector final
	{
		friend class RedirectorConfigLoader;

		public:
			static bool HasInstance();
			static Redirector& GetInstance();
			
			static kxf::String GetLibraryName();
			static kxf::String GetLibraryAuthor();
			static kxf::Version GetLibraryVersion();

			static bool DllMain(HMODULE module, DWORD event);

		private:
			FunctionTable m_Functions;

			kxf::NativeFileSystem m_PluginFS;
			kxf::NativeFileSystem m_ConfigFS;

			kxf::FlagSet<RedirectorOption> m_Options;
			std::unique_ptr<kxf::IEncodingConverter> m_EncodingConverter;
			int m_SaveOnWriteBuffer = 0;

			mutable kxf::ReadWriteLock m_INIMapLock;
			kxf::Utility::UnorderedMapNoCase<kxf::String, std::unique_ptr<ConfigObject>> m_INIMap;
			std::atomic<size_t> m_TotalWriteCount = 0;

		private:
			void InitConfig();
			bool OpenLog(kxf::LogLevel logLevel);

			void InitFunctions();
			void OverrideFunctions();
			void RestoreFunctions();

		public:
			Redirector();
			~Redirector();

		public:
			const FunctionTable& GetFunctionTable() const noexcept
			{
				return m_Functions;
			}
			SEInterface& GetSEInterface() const noexcept;
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
