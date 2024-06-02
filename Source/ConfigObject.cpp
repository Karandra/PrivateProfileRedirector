#include "stdafx.h"
#include "ConfigObject.h"
#include "PrivateProfileRedirector.h"

namespace PPR
{
	bool ConfigObject::LoadFile()
	{
		Redirector& instance = Redirector::GetInstance();

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
			m_ExistOnDisk = false;
			return false;
		}
	}
	bool ConfigObject::SaveFile()
	{
		Redirector& instance = Redirector::GetInstance();
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
		return false;
	}

	void ConfigObject::OnWrite()
	{
		m_ChangesCount++;

		Redirector& instance = Redirector::GetInstance();
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
					kxf::Log::InfoCategory("SaveOnWrite", "Changes count for '{}' reached buffer capacity ({}), flushing changes. Empty file: '{}'", m_Path.GetFullPath(), *bufferSize, (int)m_INI.IsEmpty());
				}
				else
				{
					kxf::Log::TraceCategory("SaveOnWrite", "{} changes (out of {}) accumulated for '{}'", m_ChangesCount, *bufferSize, m_Path.GetFullPath());
					return;
				}
			}
			else
			{
				kxf::Log::InfoCategory("SaveOnWrite", "Saving file on write: '{}', empty file: '{}'", m_Path.GetFullPath(), m_INI.IsEmpty());
			}
			SaveFile();
		}
	}
}
