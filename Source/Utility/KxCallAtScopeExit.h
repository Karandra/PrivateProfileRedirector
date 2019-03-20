#pragma once
#include <utility>
#include <functional>
#include <optional>

template<class T> class KxCallAtScopeExit
{
	private:
		std::optional<T> m_Functor;

	public:
		KxCallAtScopeExit(T&& functor)
			:m_Functor(std::move(functor))
		{
		}
		KxCallAtScopeExit(const T& functor)
			:m_Functor(functor)
		{
		}
		
		KxCallAtScopeExit(KxCallAtScopeExit&& other)
			:m_Functor(std::move(other.m_Functor))
		{
			other.m_Functor.reset();
		}
		KxCallAtScopeExit(const KxCallAtScopeExit& other)
			:m_Functor(other.m_Functor)
		{
		}
		
		~KxCallAtScopeExit()
		{
			if (m_Functor)
			{
				(void)std::invoke(*m_Functor);
			}
		}
	
	public:
		KxCallAtScopeExit& operator=(KxCallAtScopeExit&& other)
		{
			m_Functor = std::move(other.m_Functor);
			other.m_Functor.reset();

			return *this;
		}
		KxCallAtScopeExit& operator=(const KxCallAtScopeExit& other)
		{
			m_Functor = other.m_Functor;
			return *this;
		}
};
