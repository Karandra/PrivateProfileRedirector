#pragma once
#include "stdafx.h"
#include <utility>

namespace PPR
{
	class SRWLock final
	{
		private:
			SRWLOCK m_Lock = SRWLOCK_INIT;

		public:
			SRWLock() = default;
			SRWLock(const SRWLock&) = delete;
			SRWLock(SRWLock&&) = delete;
			~SRWLock() = default;

		public:
			void AcquireShared() noexcept
			{
				::AcquireSRWLockShared(&m_Lock);
			}
			void AcquireExclusive() noexcept
			{
				::AcquireSRWLockExclusive(&m_Lock);
			}

			bool TryAcquireShared() noexcept
			{
				return ::TryAcquireSRWLockShared(&m_Lock);
			}
			bool TryAcquireExclusive() noexcept
			{
				return ::TryAcquireSRWLockExclusive(&m_Lock);
			}

			void ReleaseShared() noexcept
			{
				::ReleaseSRWLockShared(&m_Lock);
			}
			void ReleaseExclusive() noexcept
			{
				::ReleaseSRWLockExclusive(&m_Lock);
			}
	};
}

namespace PPR::Utility
{
	enum class SRWLockerType
	{
		Shared,
		Exclusive,
	};

	template<SRWLockerType t_LockerType, bool t_IsMoveable>
	class BasicSRWLocker final
	{
		public:
			constexpr static bool IsShared() noexcept
			{
				return t_LockerType == SRWLockerType::Shared;
			}
			constexpr static bool IsExclusive() noexcept
			{
				return t_LockerType == SRWLockerType::Exclusive;
			}
			constexpr static bool IsMoveable() noexcept
			{
				return t_IsMoveable;
			}

		private:
			SRWLock* m_Lock = nullptr;

		public:
			template<class T = void*>
			BasicSRWLocker(SRWLock& lock) noexcept
				:m_Lock(&lock)
			{
				if constexpr(t_LockerType == SRWLockerType::Shared)
				{
					m_Lock->AcquireShared();
				}
				else if constexpr(t_LockerType == SRWLockerType::Exclusive)
				{
					m_Lock->AcquireExclusive();
				}
				else
				{
					static_assert(sizeof(T*) == 0, "invalid locker type");
				}
			}
			BasicSRWLocker(BasicSRWLocker&& other) noexcept
			{
				*this = std::move(other);
			}
			BasicSRWLocker(const BasicSRWLocker&) = delete;
			~BasicSRWLocker() noexcept
			{
				if constexpr(t_IsMoveable)
				{
					if (m_Lock == nullptr)
					{
						return;
					}
				}

				if constexpr(t_LockerType == SRWLockerType::Shared)
				{
					m_Lock->ReleaseShared();
				}
				else if constexpr(t_LockerType == SRWLockerType::Exclusive)
				{
					m_Lock->ReleaseExclusive();
				}
			}
			
		public:
			BasicSRWLocker& operator=(const BasicSRWLocker&) = delete;
			BasicSRWLocker& operator=(BasicSRWLocker&& other) noexcept
			{
				if constexpr(t_IsMoveable)
				{
					m_Lock = other.m_Lock;
					other.m_Lock = nullptr;
				}
				else
				{
					static_assert(!t_IsMoveable, "this locker type is not movable");
				}
				return *this;
			}
	};
}

namespace PPR
{
	using SharedSRWLocker = Utility::BasicSRWLocker<Utility::SRWLockerType::Shared, false>;
	using ExclusiveSRWLocker = Utility::BasicSRWLocker<Utility::SRWLockerType::Exclusive, false>;

	using MoveableSharedSRWLocker = Utility::BasicSRWLocker<Utility::SRWLockerType::Shared, true>;
	using MoveableExclusiveSRWLocker = Utility::BasicSRWLocker<Utility::SRWLockerType::Exclusive, true>;
}
