#pragma once
#include "Event.h"
#include <functional>
class QxEvtHandler;

class QxEventCallWrapper
{
	public:
		QxEventCallWrapper() = default;
		virtual ~QxEventCallWrapper() = default;

	public:
		virtual void Execute(QxEvtHandler& evtHandler, QxEvent& event) = 0;
		virtual bool IsSameAs(const QxEventCallWrapper& other) const = 0;
		virtual QxEvtHandler* GetEvtHandler()
		{
			return nullptr;
		}
};

namespace Qx::EventSystem
{
	// Wrapper for lambda or class which implements 'operator()'
	template<class aTEvent, class aTFunctor>
	class FunctorWrapper: public QxEventCallWrapper
	{
		public:
			using TEvent = aTEvent;
			using TFunctor = aTFunctor;

		protected:
			TFunctor m_Functor;
			const void* m_OriginalAddress = nullptr;

		public:
			FunctorWrapper(TFunctor&& func)
				:m_Functor(std::forward<TFunctor>(func)), m_OriginalAddress(std::addressof(func))
			{
			}

		public:
			void Execute(QxEvtHandler& evtHandler, QxEvent& event) override
			{
				std::invoke(m_Functor, static_cast<TEvent&>(event));
			}
			bool IsSameAs(const QxEventCallWrapper& other) const override
			{
				if (typeid(*this) == typeid(other))
				{
					return m_OriginalAddress == static_cast<const FunctorWrapper&>(other).m_OriginalAddress;
				}
				return false;
			}
	};

	// Wrapper for free and static functions
	template<class aTEvent>
	class FunctionWrapper: public QxEventCallWrapper
	{
		public:
			using TEvent = aTEvent;
			using TFunction = void(*)(TEvent&);

		protected:
			TFunction m_Function = nullptr;

		public:
			FunctionWrapper(TFunction func)
				:m_Function(func)
			{
			}

		public:
			void Execute(QxEvtHandler& evtHandler, QxEvent& event) override
			{
				std::invoke(m_Function, static_cast<TEvent&>(event));
			}
			bool IsSameAs(const QxEventCallWrapper& other) const override
			{
				if (typeid(*this) == typeid(other))
				{
					return m_Function == static_cast<const FunctionWrapper&>(other).m_Function;
				}
				return false;
			}
	};

	// Wrapper for class member function
	template<class aTEvent, class aTClass, class aTEventArg, class aTHandler>
	class MethodWrapper: public QxEventCallWrapper
	{
		public:
			using TEvent = aTEvent;

			using TClass = aTClass;
			using TEventArg = aTEventArg;
			using THandler = aTHandler;
			using TMethod = void(TClass::*)(TEventArg&);

		protected:
			THandler* m_EvtHandler = nullptr;
			TMethod m_Method = nullptr;

		public:
			MethodWrapper(TMethod func, THandler* evtHandler)
				:m_Method(func), m_EvtHandler(evtHandler)
			{
			}

		public:
			void Execute(QxEvtHandler& evtHandler, QxEvent& event) override
			{
				TClass* realEvtHandler = m_EvtHandler;
				if (realEvtHandler == nullptr)
				{
					realEvtHandler = static_cast<TClass*>(&evtHandler);
				}

				std::invoke(m_Method, realEvtHandler, static_cast<TEventArg&>(event));
			}
			bool IsSameAs(const QxEventCallWrapper& other) const override
			{
				if (typeid(*this) == typeid(other))
				{
					const MethodWrapper& otherRef = static_cast<const MethodWrapper&>(other);

					return (m_Method == otherRef.m_Method || otherRef.m_Method == nullptr) &&
						(m_EvtHandler == otherRef.m_EvtHandler || otherRef.m_EvtHandler == nullptr);
				}
				return false;
			}
			QxEvtHandler* GetEvtHandler() override
			{
				return m_EvtHandler;
			}
	};
}
