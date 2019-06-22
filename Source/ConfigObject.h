#pragma once
#include "stdafx.h"
#include "Utility\KxDynamicString.h"
#include "Utility\SRWLock.h"
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
			KxDynamicStringW m_Path;
			SRWLock m_Lock;
			bool m_HasChanges = false;
			bool m_ExistOnDisk = false;

		private:
			bool LoadFile();
			bool SaveFile();

		public:
			ConfigObject(KxDynamicStringRefW filePath)
				:m_Path(filePath)
			{
			}

		public:
			const INIWrapper& GetINI() const
			{
				return m_INI;
			}
			INIWrapper& GetINI()
			{
				return m_INI;
			}
			KxDynamicStringRefW GetFilePath() const
			{
				return m_Path;
			}

			void OnWrite();
			bool IsExistOnDisk() const
			{
				return m_ExistOnDisk;
			}
			bool HasChanges() const
			{
				return m_HasChanges;
			}
			bool IsEmpty() const
			{
				return m_INI.IsEmpty();
			}

			SRWLock& GetLock()
			{
				return m_Lock;
			}
			SharedSRWLocker LockShared()
			{
				return m_Lock;
			}
			ExclusiveSRWLocker LockExclusive()
			{
				return m_Lock;
			}
	};
}
