#pragma once
#include "stdafx.h"

namespace PPR
{
	class CriticalSection final
	{
		private:
			CRITICAL_SECTION m_CritSec;

		public:
			CriticalSection() noexcept
			{
				::InitializeCriticalSection(&m_CritSec);
			}
			CriticalSection(uint32_t count)
			{
				::InitializeCriticalSectionAndSpinCount(&m_CritSec, count);
			}
			CriticalSection(const CriticalSection&) = delete;
			CriticalSection(CriticalSection&&) = delete;
			~CriticalSection() noexcept
			{
				::DeleteCriticalSection(&m_CritSec);
			}

		public:
			void Enter() noexcept
			{
				::EnterCriticalSection(&m_CritSec);
			}
			bool TryEnter() noexcept
			{
				return ::TryEnterCriticalSection(&m_CritSec);
			}
			void Leave() noexcept
			{
				::LeaveCriticalSection(&m_CritSec);
			}

		public:
			CriticalSection& operator=(const CriticalSection&) = delete;
			CriticalSection& operator=(CriticalSection&&) = delete;
	};
}

namespace PPR::Utility
{
	template<bool t_IsMoveable> class BasicCriticalSectionLocker final
	{
		public:
			constexpr static bool IsMoveable() noexcept
			{
				return t_IsMoveable;
			}

		private:
			CriticalSection* m_CritSec = nullptr;

		public:
			BasicCriticalSectionLocker(CriticalSection& critSec) noexcept
				:m_CritSec(&critSec)
			{
				m_CritSec->Enter();
			}
			BasicCriticalSectionLocker(BasicCriticalSectionLocker&& other) noexcept
			{
				*this = std::move(other);
			}
			BasicCriticalSectionLocker(const BasicCriticalSectionLocker&) = delete;
			~BasicCriticalSectionLocker() noexcept
			{
				if constexpr(t_IsMoveable)
				{
					if (m_CritSec == nullptr)
					{
						return;
					}
				}
				m_CritSec->Leave();
			}

		public:
			BasicCriticalSectionLocker& operator=(const BasicCriticalSectionLocker&) = delete;
			BasicCriticalSectionLocker& operator=(BasicCriticalSectionLocker&& other) noexcept
			{
				if constexpr(t_IsMoveable)
				{
					m_CritSec = other.m_CritSec;
					other.m_CritSec = nullptr;
				}
				else
				{
					static_assert(false, "this locker type is not movable");
				}
				return *this;
			}
	};
}

namespace PPR
{
	using CriticalSectionLocker = Utility::BasicCriticalSectionLocker<false>;
	using MoveableCriticalSectionLocker = Utility::BasicCriticalSectionLocker<true>;
}
