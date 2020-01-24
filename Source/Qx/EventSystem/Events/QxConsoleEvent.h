#pragma once
#include "Qx/EventSystem/Event.h"
#include "Utility/KxDynamicString.h"

class QxConsoleEvent: public QxEvent
{
	public:
		QxEVENT_MEMBER_DEFINE(QxConsoleEvent, Command);

	private:
		KxDynamicStringA m_CommandName;
		KxDynamicStringA m_CommandAlias;
		KxDynamicStringA m_CommandHelp;

		double m_Result = 0;

	public:
		QxConsoleEvent() = default;

	public:
		KxDynamicStringA GetCommandName() const
		{
			return m_CommandName;
		}
		void SetCommandName(KxDynamicStringRefA name)
		{
			m_CommandName = name;
		}

		KxDynamicStringA GetCommandAlias() const
		{
			return m_CommandAlias;
		}
		void SetCommandAlias(KxDynamicStringRefA alias)
		{
			m_CommandAlias = alias;
		}
		
		KxDynamicStringA GetCommandHelp() const
		{
			return m_CommandName;
		}
		void SetCommandHelp(KxDynamicStringRefA help)
		{
			m_CommandHelp = help;
		}

		double GetResult() const
		{
			return m_Result;
		}
		double SetResult(double result)
		{
			m_Result = result;
		}
};
