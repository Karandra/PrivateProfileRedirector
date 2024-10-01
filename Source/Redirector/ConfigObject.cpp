#include "stdafx.h"
#include "ConfigObject.h"
#include "RedirectorInterface.h"
#include <kxf/System/Win32Error.h>

namespace PPR
{
	bool ConfigObject::LoadFile()
	{
		RedirectorInterface& instance = RedirectorInterface::GetInstance();

		// Set loading options
		kxf::FlagSet<kxf::INIDocumentOption> options;
		options.Add(kxf::INIDocumentOption::Quotes);
		options.Add(kxf::INIDocumentOption::IgnoreCase);
		options.Mod(kxf::INIDocumentOption::InlineComments, instance.IsOptionEnabled(RedirectorOption::ProcessInlineComments));

		if (m_INI.Load(m_Path, options))
		{
			m_ChangesCount = 0;
			m_ExistOnDisk = true;

			return true;
		}
		else
		{
			auto lastError = kxf::Win32Error::GetLastError();
			m_ExistOnDisk = kxf::NativeFileSystem().FileExist(m_Path);
			kxf::Log::Error("Failed to load file '{}', Exist on disk: {}, {}", m_Path.GetFullPath(), m_ExistOnDisk, lastError);

			return false;
		}
	}
	bool ConfigObject::SaveFile()
	{
		RedirectorInterface& instance = RedirectorInterface::GetInstance();
		if (instance.IsOptionEnabled(RedirectorOption::WriteProtected))
		{
			kxf::Log::TraceCategory("WriteProtected", "Attempt to write data to '{}'", m_Path.GetFullPath());

			m_ChangesCount = 0;
			return false;
		}
		else if (instance.IsOptionEnabled(RedirectorOption::NativeWrite))
		{
			kxf::Log::TraceCategory("NativeWrite", "NativeWrite enabled, ignoring this write operation for '{}'", m_Path.GetFullPath());

			m_ChangesCount = 0;
			return false;
		}

		if (m_INI.Save(m_Path))
		{
			m_ChangesCount = 0;
			m_ExistOnDisk = true;

			return true;
		}
		else
		{
			auto lastError = kxf::Win32Error::GetLastError();
			m_ExistOnDisk = kxf::NativeFileSystem().FileExist(m_Path);
			kxf::Log::Error("Failed to save file '{}', Exist on disk: {}, {}", m_Path.GetFullPath(), m_ExistOnDisk, lastError);

			return false;
		}
	}

	void ConfigObject::OnWrite()
	{
		m_ChangesCount++;

		RedirectorInterface& instance = RedirectorInterface::GetInstance();
		kxf::Utility::ScopeGuard atExit = [&]()
		{
			instance.OnFileWrite(*this);
		};
		
		if (instance.IsOptionEnabled(RedirectorOption::NativeWrite))
		{
			kxf::Log::TraceCategory("NativeWrite", "NativeWrite enabled, ignoring this write operation for '{}'", m_Path.GetFullPath());
			return;
		}

		if (instance.IsOptionEnabled(RedirectorOption::SaveOnWrite))
		{
			if (auto bufferSize = instance.GetSaveOnWriteBuffer())
			{
				if (m_ChangesCount >= *bufferSize)
				{
					kxf::Log::InfoCategory("SaveOnWrite", "Changes count for '{}' reached buffer capacity ({}), flushing changes. Is empty file: {}", m_Path.GetFullPath(), *bufferSize, m_INI.IsEmpty());
				}
				else
				{
					kxf::Log::TraceCategory("SaveOnWrite", "Accumulated {} changes (out of {}) for '{}'", m_ChangesCount, *bufferSize, m_Path.GetFullPath());
					return;
				}
			}
			else
			{
				kxf::Log::InfoCategory("SaveOnWrite", "Saving file on write: '{}', is empty file: {}", m_Path.GetFullPath(), m_INI.IsEmpty());
			}
			SaveFile();
		}
	}
}
