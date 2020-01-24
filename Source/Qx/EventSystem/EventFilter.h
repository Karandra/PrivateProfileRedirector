#pragma once
class QxEvent;
class QxEvtHandler;

class QxEventFilter
{
	friend class QxEvtHandler;

	public:
		enum class Result
		{
			Skip = -1,
			Ignore = 0,
			Processed = 1
		};

	private:
		// Objects of this class are made to be stored in a linked list in
		// QxEvtHandler so put the next node pointer directly in the class itself.
		QxEventFilter* m_Next = nullptr;

	public:
		QxEventFilter() = default;
		QxEventFilter(const QxEventFilter&) = delete;
		virtual ~QxEventFilter() = default;

	public:
		virtual Result FilterEvent(QxEvent& event) = 0;

	public:
		QxEventFilter& operator=(const QxEventFilter&) = delete;
};
