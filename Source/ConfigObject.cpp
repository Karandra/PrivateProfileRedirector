#include "stdafx.h"
#include "ConfigObject.h"
#include "PrivateProfileRedirector.h"

namespace PPR
{
	bool ConfigObject::LoadFile()
	{
		Redirector& instance = Redirector::GetInstance();

		// Set loading options
		Encoding encoding = Encoding::Auto;
		Options options = Options::None;
		if (instance.IsOptionEnabled(RedirectorOption::ProcessInlineComments))
		{
			options |= Options::RemoveInlineComments;
		}

		if (m_INI.Load(m_Path, options, encoding))
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
			Redirector::GetInstance().Log(L"[WriteProtected] Attempt to write data to '%s'", m_Path.data());
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
		if (instance.IsOptionEnabled(RedirectorOption::SaveOnWrite))
		{
			if (auto bufferSize = instance.GetSaveOnWriteBuffer())
			{
				if (m_ChangesCount >= *bufferSize)
				{
					Redirector::GetInstance().Log(L"SaveOnWrite: Changes count for '%s' reached buffer capacity (%zu), flushing changes. Empty file: '%d'", m_Path.data(), *bufferSize, (int)m_INI.IsEmpty());
				}
				else
				{
					Redirector::GetInstance().Log(L"SaveOnWrite: %zu changes (out of %zu) accumulated for '%s'", m_ChangesCount, *bufferSize, m_Path.data());
					return;
				}
			}
			else
			{
				Redirector::GetInstance().Log(L"Saving file on write: '%s', empty file: '%d'", m_Path.data(), (int)m_INI.IsEmpty());
			}
			SaveFile();
		}
	}
}
