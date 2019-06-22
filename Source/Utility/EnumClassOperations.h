#pragma once
#include <type_traits>

namespace PPR
{
	template<class T> struct IsEnumCastAllowed: std::false_type {};
	template<class T> inline constexpr bool IsEnumCastAllowedV = IsEnumCastAllowed<T>::value;

	template<class T> struct IsEnumBitwiseAllowed: std::false_type {};
	template<class T> inline constexpr bool IsEnumBitwiseAllowedV = IsEnumBitwiseAllowed<T>::value;
}

namespace PPR::EnumClass
{
	template<class T> inline constexpr bool IsIntCastAllowed = IsEnumCastAllowedV<T> || IsEnumBitwiseAllowedV<T>;

	template<class TEnum> class AndValueWrapper final
	{
		private:
			TEnum m_Value;

		private:
			constexpr auto AsInt() const noexcept
			{
				return static_cast<std::underlying_type_t<TEnum>>(m_Value);
			}

		public:
			constexpr AndValueWrapper() = default;
			constexpr AndValueWrapper(TEnum value) noexcept
				:m_Value(value)
			{
			}
	
		public:
			constexpr operator TEnum() const noexcept
			{
				return m_Value;
			}
			constexpr operator bool() const noexcept
			{
				return AsInt() != 0;
			}
			constexpr bool operator!() const noexcept
			{
				return AsInt() == 0;
			}
	};
}

namespace PPR::EnumClass
{
	template<class TEnum>
	constexpr std::enable_if_t<IsIntCastAllowed<TEnum>, bool> ToBool(TEnum value) noexcept
	{
		using Tint = std::underlying_type_t<TEnum>;
		return static_cast<Tint>(value) != 0;
	}

	template<class TInt, class TEnum>
	constexpr std::enable_if_t<IsIntCastAllowed<TEnum>, TInt> ToInt(TEnum value) noexcept
	{
		return static_cast<TInt>(value);
	}

	template<class TEnum, class TInt = std::underlying_type_t<TEnum>>
	constexpr std::enable_if_t<IsIntCastAllowed<TEnum>, TInt> ToInt(TEnum value) noexcept
	{
		return static_cast<TInt>(value);
	}

	template<class TEnum, class TInt>
	constexpr std::enable_if_t<IsIntCastAllowed<TEnum>, TEnum> FromInt(TInt value) noexcept
	{
		return static_cast<TEnum>(value);
	}
}

namespace PPR
{
	template<class TEnum, class TRet = typename std::enable_if_t<IsEnumBitwiseAllowedV<TEnum>, EnumClass::AndValueWrapper<TEnum>>>
	constexpr TRet operator&(TEnum left, TEnum right) noexcept
	{
		using Tint = std::underlying_type_t<TEnum>;
		return static_cast<TEnum>(static_cast<Tint>(left) & static_cast<Tint>(right));
	}

	template<class TEnum, class TRet = typename std::enable_if_t<IsEnumBitwiseAllowedV<TEnum>, TEnum>>
	constexpr TRet operator|(TEnum left, TEnum right) noexcept
	{
		using Tint = std::underlying_type_t<TEnum>;
		return static_cast<TEnum>(static_cast<Tint>(left) | static_cast<Tint>(right));
	}

	template<class TEnum, class TRet = typename std::enable_if_t<IsEnumBitwiseAllowedV<TEnum>, TEnum&>>
	constexpr TRet operator&=(TEnum& left, TEnum right) noexcept
	{
		left = left & right;
		return left;
	}

	template<class TEnum, class TRet = typename std::enable_if_t<IsEnumBitwiseAllowedV<TEnum>, TEnum&>>
	constexpr TRet operator|=(TEnum& left, TEnum right) noexcept
	{
		left = left | right;
		return left;
	}

	template<class TEnum, class TRet = typename std::enable_if_t<IsEnumBitwiseAllowedV<TEnum>, TEnum>>
	constexpr TRet operator~(TEnum value) noexcept
	{
		using Tint = std::underlying_type_t<TEnum>;
		return static_cast<TEnum>(~static_cast<Tint>(value));
	}
}

#define PPR_AllowEnumCastOp(T) template<> struct IsEnumCastAllowed<T>: std::true_type {}
#define PPR_AllowEnumBitwiseOp(T) template<> struct IsEnumBitwiseAllowed<T>: std::true_type {}
