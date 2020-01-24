#pragma once
#include <type_traits>

namespace Qx::Utility
{
	template<class TLeft, class TRight>
	void ExchangeAndReset(TLeft& left, TLeft& right, TRight nullValue)
	{
		static_assert(std::is_trivially_move_assignable_v<TLeft> && std::is_trivially_move_assignable_v<TRight>,
					  "can only use ExchangeAndReset for trivially move assignable types");

		left = right;
		right = std::move(nullValue);
	}
	
	template<class TLeft, class TRight>
	TLeft ExchangeResetAndReturn(TLeft& right, TRight nullValue)
	{
		static_assert(std::is_default_constructible_v<TLeft>, "left type must be default constructible");

		TLeft left{};
		ExchangeAndReset(left, right, std::move(nullValue));
		return left;
	}
}

namespace Qx::Utility
{
	template<class> struct MethodTraits;

	template <class Return, class Object, class... Args>
	struct MethodTraits<Return(Object::*)(Args...)>
	{
		using TReturn = Return;
		using TInstance = Object;

		inline static constexpr size_t ArgumentCount = sizeof...(Args);
	};
}

namespace Qx::Utility
{
	template<class TCallable, class... Args>
	struct CallableTraits
	{
		inline static constexpr bool IsInvokable = std::is_invocable_v<TCallable, Args...>;
		inline static constexpr bool IsFreeFunction = std::is_function_v<std::remove_pointer_t<TCallable>>;
		inline static constexpr bool IsMemberFunction = std::is_member_function_pointer_v<TCallable>;
		inline static constexpr bool IsFunctor = IsInvokable && (!IsFreeFunction && !IsMemberFunction);
	};
}
