#pragma once
#include "stdafx.h"

class KxCriticalSection
{
	private:
		CRITICAL_SECTION m_CritSec;

	public:
		KxCriticalSection()
		{
			::InitializeCriticalSection(&m_CritSec);
		}
		KxCriticalSection(DWORD spinCount)
		{
			::InitializeCriticalSectionAndSpinCount(&m_CritSec, spinCount);
		}
		~KxCriticalSection()
		{
			::DeleteCriticalSection(&m_CritSec);
		}

	public:
		void Enter()
		{
			::EnterCriticalSection(&m_CritSec);
		}
		bool TryEnter()
		{
			return ::TryEnterCriticalSection(&m_CritSec);
		}
		void Leave()
		{
			::LeaveCriticalSection(&m_CritSec);
		}
};

class KxCriticalSectionLocker
{
	private:
		KxCriticalSection& m_CritSec;
		bool m_ShouldLeave = true;

	public:
		KxCriticalSectionLocker(KxCriticalSection& critSec)
			:m_CritSec(critSec)
		{
			m_CritSec.Enter();
		}
		~KxCriticalSectionLocker()
		{
			if (m_ShouldLeave)
			{
				m_CritSec.Leave();
			}
		}

	public:
		const KxCriticalSection& GetCritSection() const
		{
			return m_CritSec;
		}
		KxCriticalSection& GetCritSection()
		{
			return m_CritSec;
		}

		void Leave()
		{
			if (m_ShouldLeave)
			{
				m_CritSec.Leave();
				m_ShouldLeave = false;
			}
		}
};
