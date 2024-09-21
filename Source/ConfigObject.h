#pragma once
#include "Common.h"
#include <kxf/Threading/ReadWriteLock.h>
#include <kxf/Threading/LockGuard.h>
#include "INIWrapper.h"

namespace PPR
{
	class ConfigObject
	{
		friend class Redirector;

		public:
			using Options = INIWrapper::Options;
			using Encoding = INIWrapper::Encoding;

		private:
			INIWrapper m_INI;
			kxf::FSPath m_Path;
			size_t m_ChangesCount = 0;
			bool m_ExistOnDisk = false;

			kxf::ReadWriteLock m_Lock;

		private:
			bool LoadFile();
			bool SaveFile();

		public:
			ConfigObject(kxf::FSPath filePath)
				:m_Path(std::move(filePath))
			{
			}

		public:
			const INIWrapper& GetINI() const noexcept
			{
				return m_INI;
			}
			INIWrapper& GetINI() noexcept
			{
				return m_INI;
			}
			kxf::FSPath GetFilePath() const
			{
				return m_Path;
			}

			bool IsExistOnDisk() const
			{
				return m_ExistOnDisk;
			}
			bool HasChanges() const
			{
				return m_ChangesCount != 0;
			}
			bool IsEmpty() const
			{
				return m_INI.IsEmpty();
			}
			void OnWrite();

			kxf::ReadWriteLock& GetLock() noexcept
			{
				return m_Lock;
			}
			auto LockShared() noexcept
			{
				return kxf::ReadLockGuard(m_Lock);
			}
			auto LockExclusive()
			{
				return kxf::WriteLockGuard(m_Lock);
			}
	};
}
