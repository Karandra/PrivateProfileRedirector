#pragma once
#include "stdafx.h"
#include "String.h"
#include "PrivateProfileRedirector.h"
#include <sstream>
#include <iomanip>

namespace PPR::Utility::String::Internal
{
	KxDynamicStringW ToCurrentCodePage(KxDynamicStringRefA value)
	{
		return Redirector::GetInstance().ConvertFromACP(value.data(), static_cast<int>(value.length()));
	}
}

namespace PPR::Utility::String
{
	KxDynamicStringRefW TrimCharsL(KxDynamicStringRefW value, KxDynamicStringW::TChar c1, KxDynamicStringW::TChar c2)
	{
		if (!value.empty())
		{
			size_t count = 0;
			for (size_t i = 0; i < value.size(); i++)
			{
				if (value[i] == c1 || value[i] == c2)
				{
					count++;
				}
				else
				{
					break;
				}
			}
			value.remove_prefix(count);
		}
		return value;
	}
	KxDynamicStringRefW TrimCharsR(KxDynamicStringRefW value, KxDynamicStringW::TChar c1, KxDynamicStringW::TChar c2)
	{
		if (!value.empty())
		{
			size_t count = 0;
			for (size_t i = value.size() - 1; i != 0; i--)
			{
				if (value[i] == c1 || value[i] == c2)
				{
					count++;
				}
				else
				{
					break;
				}
			}
			value.remove_suffix(count);
		}
		return value;
	}
}

namespace
{
	std::optional<int64_t> DoToInteger(const KxDynamicStringW& stringValue, int base)
	{
		errno = 0;
		wchar_t* endPtr = nullptr;

		int64_t value = std::wcstoll(stringValue.data(), &endPtr, base);
		if (endPtr && *endPtr == L'\0' && errno == 0)
		{
			return value;
		}
		return std::nullopt;
	}
}
namespace PPR::Utility::String
{
	std::optional<int64_t> ToInteger(KxDynamicStringRefW stringValue, int base)
	{
		auto MakeBuffer = [&stringValue](size_t offset = 0) -> KxDynamicStringW
		{
			return stringValue.substr(offset, std::numeric_limits<int64_t>::digits10 + 1);
		};

		switch (base)
		{
			case 16:
			case 10:
			case 8:
			case 2:
			{
				KxDynamicStringW buffer = MakeBuffer();
				return DoToInteger(buffer, base);
			}
			case 0:
			{
				KxDynamicStringRefW prefix = stringValue.substr(0, 2);
				if (prefix == L"0x")
				{
					KxDynamicStringW buffer = MakeBuffer(2);
					return DoToInteger(buffer, 16);
				}
				else if (prefix == L"0o")
				{
					KxDynamicStringW buffer = MakeBuffer(2);
					return DoToInteger(buffer, 8);
				}
				else if (prefix == L"0b")
				{
					KxDynamicStringW buffer = MakeBuffer(2);
					return DoToInteger(buffer, 2);
				}
				else
				{
					KxDynamicStringW buffer = MakeBuffer();
					return DoToInteger(buffer, 10);
				}
			}
		};
		return std::nullopt;
	}
}
