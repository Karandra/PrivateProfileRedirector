#pragma once
#include "stdafx.h"

template<class T, T t_DefaultOptions = (T)0>
class KxWithOptions
{
	public:
		using TEnum = typename T;
		using TInt = typename std::underlying_type_t<TEnum>;

	private:
		TEnum m_Value = t_DefaultOptions;

	protected:
		constexpr static bool DoIsOptionEnabled(TEnum value, TEnum option) noexcept
		{
			return static_cast<TInt>(value) & static_cast<TInt>(option);
		}
		constexpr static TEnum DoSetOptionEnabled(TEnum value, TEnum option, bool enable = true) noexcept
		{
			if (enable)
			{
				return static_cast<TEnum>(static_cast<TInt>(value) | static_cast<TInt>(option));
			}
			else
			{
				return static_cast<TEnum>(static_cast<TInt>(value) & ~static_cast<TInt>(option));
			}
		}

	public:
		constexpr KxWithOptions() = default;
		constexpr KxWithOptions(KxWithOptions&& other) noexcept
			:m_Value(other.m_Value)
		{
			other.m_Value = t_DefaultOptions;
		}
		constexpr KxWithOptions(const KxWithOptions& other) noexcept
			:m_Value(other.m_Value)
		{
		}
		constexpr KxWithOptions(TEnum value) noexcept
			:m_Value(value)
		{
		}

	public:
		constexpr TEnum GetOptionsValue() const noexcept
		{
			return m_Value;
		}
		constexpr void SetOptionsValue(TEnum options) noexcept
		{
			m_Value = options;
		}
		constexpr void SetOptionsValue(TInt options) noexcept
		{
			m_Value = static_cast<TEnum>(options);
		}

		constexpr bool IsOptionEnabled(TEnum option) const noexcept
		{
			return DoIsOptionEnabled(m_Value, option);
		}
		constexpr void SetOptionEnabled(TEnum option, bool enable = true) noexcept
		{
			m_Value = DoSetOptionEnabled(m_Value, option, enable);
		}

	public:
		constexpr bool operator==(const KxWithOptions& other) const noexcept
		{
			return m_Value == other.m_Value;
		}
		constexpr bool operator!=(const KxWithOptions& other) const noexcept
		{
			return m_Value != other.m_Value;
		}

		constexpr KxWithOptions& operator=(KxWithOptions&& other) noexcept
		{
			m_Value = other.m_Value;
			other.m_Value = t_DefaultOptions;
			return *this;
		}
		constexpr KxWithOptions& operator=(const KxWithOptions& other) noexcept
		{
			m_Value = other.m_Value;
			return *this;
		}
};
