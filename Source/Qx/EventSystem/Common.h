#pragma once
#include <cstdint>
#include <memory>
using QxEventID = intptr_t;
using QxWidgetID = intptr_t;

enum class QxStandardID: QxWidgetID
{
	None = -2,
	Any = -1,
};

template<class T>
class QxEventTag final
{
	friend class QxEvent;

	public:
		using TEvent = T;

	private:
		QxEventID m_EventID = 0;

	private:
		constexpr QxEventTag()
			:m_EventID(0)
		{
		}
	
	public:
		constexpr QxEventTag(QxEventID id)
			:m_EventID(id)
		{
		}

	public:
		constexpr QxEventID GetID() const
		{
			return m_EventID;
		}
		constexpr operator QxEventID() const
		{
			return m_EventID;
		}
};
