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
			m_HasChanges = false;
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
			m_HasChanges = false;
			return false;
		}

		if (m_INI.Save(m_Path))
		{
			m_HasChanges = false;
			m_ExistOnDisk = true;

			return true;
		}
		return false;
	}

	void ConfigObject::OnWrite()
	{
		m_HasChanges = true;

		Redirector& instance = Redirector::GetInstance();
		if (instance.IsOptionEnabled(RedirectorOption::SaveOnWrite))
		{
			Redirector::GetInstance().Log(L"Saving file on write: '%s', Is empty: '%d'", m_Path.data(), (int)m_INI.IsEmpty());
			SaveFile();
		}
	}
}
