#pragma once
#include <unordered_set>
#include <unordered_map>
#include "KxDynamicString.h"

namespace PPR::Utility::String
{
	inline char CharToLower(char c)
	{
		#pragma warning(suppress: 4312) // 'operation' : conversion from 'type1' to 'type2' of greater size
		#pragma warning(suppress: 4302) // 'conversion' : truncation from 'type 1' to 'type 2'
		return reinterpret_cast<char>(::CharLowerA(reinterpret_cast<LPSTR>(c)));
	}
	inline char CharToUpper(char c)
	{
		#pragma warning(suppress: 4312) // 'operation' : conversion from 'type1' to 'type2' of greater size
		#pragma warning(suppress: 4302) // 'conversion' : truncation from 'type 1' to 'type 2'
		return reinterpret_cast<char>(::CharUpperA(reinterpret_cast<LPSTR>(c)));
	}
	inline wchar_t CharToLower(wchar_t c)
	{
		#pragma warning(suppress: 4312) // 'operation' : conversion from 'type1' to 'type2' of greater size
		#pragma warning(suppress: 4302) // 'conversion' : truncation from 'type 1' to 'type 2'
		return reinterpret_cast<wchar_t>(::CharLowerW(reinterpret_cast<LPWSTR>(c)));
	}
	inline wchar_t CharToUpper(wchar_t c)
	{
		#pragma warning(suppress: 4312) // 'operation' : conversion from 'type1' to 'type2' of greater size
		#pragma warning(suppress: 4302) // 'conversion' : truncation from 'type 1' to 'type 2'
		return reinterpret_cast<wchar_t>(::CharUpperW(reinterpret_cast<LPWSTR>(c)));
	}
}

namespace PPR::Utility::String::Internal
{
	enum class CompareResult: int
	{
		LessThan = CSTR_LESS_THAN,
		Equal = CSTR_EQUAL,
		GreaterThan = CSTR_GREATER_THAN,
	};

	inline CompareResult CompareStrings(KxDynamicStringRefW v1, KxDynamicStringRefW v2, bool ignoreCase)
	{
		return static_cast<CompareResult>(::CompareStringOrdinal(v1.data(), static_cast<int>(v1.length()), v2.data(), static_cast<int>(v2.length()), ignoreCase));
	}
	inline CompareResult CompareStrings(KxDynamicStringRefW v1, KxDynamicStringRefW v2)
	{
		return CompareStrings(v1, v2, false);
	}
	inline CompareResult CompareStringsNoCase(KxDynamicStringRefW v1, KxDynamicStringRefW v2)
	{
		return CompareStrings(v1, v2, true);
	}

	KxDynamicStringW ToCurrentCodePage(KxDynamicStringRefA value);
}

namespace PPR::Utility::String
{
	// ==
	template<class T>
	bool IsEqual(const T& v1, const T& v2)
	{
		return v1 == v2;
	}
	
	inline bool IsEqual(KxDynamicStringRefW v1, KxDynamicStringRefW v2)
	{
		return Internal::CompareStrings(v1, v2) == Internal::CompareResult::Equal;
	}
	inline bool IsEqualNoCase(KxDynamicStringRefW v1, KxDynamicStringRefW v2)
	{
		return Internal::CompareStringsNoCase(v1, v2) == Internal::CompareResult::Equal;
	}

	inline bool IsEqual(KxDynamicStringRefA v1, KxDynamicStringRefA v2)
	{
		KxDynamicStringW w1 = Internal::ToCurrentCodePage(v1);
		KxDynamicStringW w2 = Internal::ToCurrentCodePage(v2);
		return Internal::CompareStrings(w1, w2) == Internal::CompareResult::Equal;
	}
	inline bool IsEqualNoCase(KxDynamicStringRefA v1, KxDynamicStringRefA v2)
	{
		KxDynamicStringW w1 = Internal::ToCurrentCodePage(v1);
		KxDynamicStringW w2 = Internal::ToCurrentCodePage(v2);
		return Internal::CompareStringsNoCase(w1, w2) == Internal::CompareResult::Equal;
	}
}

namespace PPR::Utility::String
{
	KxDynamicStringRefW TrimCharsL(KxDynamicStringRefW value, KxDynamicStringW::TChar c1, KxDynamicStringW::TChar c2);
	KxDynamicStringRefW TrimCharsR(KxDynamicStringRefW value, KxDynamicStringW::TChar c1, KxDynamicStringW::TChar c2);
	
	inline KxDynamicStringRefW TrimCharsLR(KxDynamicStringRefW value, KxDynamicStringW::TChar c1, KxDynamicStringW::TChar c2)
	{
		value = TrimCharsL(value, c1, c2);
		value = TrimCharsR(value, c1, c2);
		return value;
	}
	inline KxDynamicStringRefW TrimSpaceCharsLR(KxDynamicStringRefW value)
	{
		return TrimCharsLR(value, L' ', L'\t');
	}
	inline KxDynamicStringRefW TrimQuoteCharsLR(KxDynamicStringRefW value)
	{
		return TrimCharsLR(value, L'\"', L'\'');
	}

	std::optional<int64_t> ToInteger(KxDynamicStringRefW value, int base = 0);
}

namespace PPR::Utility::String
{
	struct StringEqualTo
	{
		bool operator()(KxDynamicStringRefW v1, KxDynamicStringRefW v2) const
		{
			return IsEqual(v1, v2);
		}
		bool operator()(KxDynamicStringRefA v1, KxDynamicStringRefA v2) const
		{
			return IsEqual(v1, v2);
		}
	};
	struct StringEqualToNoCase
	{
		bool operator()(KxDynamicStringRefW v1, KxDynamicStringRefW v2) const
		{
			return IsEqualNoCase(v1, v2);
		}
		bool operator()(KxDynamicStringRefA v1, KxDynamicStringRefA v2) const
		{
			return IsEqualNoCase(v1, v2);
		}
	};

	struct StringHash
	{
		size_t operator()(KxDynamicStringRefW value) const
		{
			return std::hash<KxDynamicStringRefW>()(value);
		}
		size_t operator()(KxDynamicStringRefA value) const
		{
			return std::hash<KxDynamicStringRefA>()(value);
		}
	};
	struct StringHashNoCase
	{
		// From Boost
		template<class T>
		static void hash_combine(size_t& seed, const T& v)
		{
			std::hash<T> hasher;
			seed ^= hasher(v) + size_t(0x9e3779b9u) + (seed << 6) + (seed >> 2);
		}

		size_t operator()(KxDynamicStringRefW value) const
		{
			size_t hashValue = 0;
			for (wchar_t c: value)
			{
				hash_combine(hashValue, String::CharToLower(c));
			}
			return hashValue;
		}
		size_t operator()(KxDynamicStringRefA value) const
		{
			size_t hashValue = 0;
			for (char c: value)
			{
				hash_combine(hashValue, String::CharToLower(c));
			}
			return hashValue;
		}
	};
}

namespace PPR::Utility::String
{
	template<class TValue> using UnorderedMapW = std::unordered_map<KxDynamicStringW, TValue, StringHash, StringEqualTo>;
	template<class TValue> using UnorderedMapWNoCase = std::unordered_map<KxDynamicStringW, TValue, StringHashNoCase, StringEqualToNoCase>;

	template<class TValue> using UnorderedMapA = std::unordered_map<KxDynamicStringA, TValue, StringHash, StringEqualTo>;
	template<class TValue> using UnorderedMapANoCase = std::unordered_map<KxDynamicStringA, TValue, StringHashNoCase, StringEqualToNoCase>;

	using UnorderedSetW = std::unordered_set<KxDynamicStringW, StringHash, StringEqualTo>;
	using UnorderedSetWNoCase = std::unordered_set<KxDynamicStringW, StringHashNoCase, StringEqualToNoCase>;

	using UnorderedSetA = std::unordered_set<KxDynamicStringA, StringHash, StringEqualTo>;
	using UnorderedSetANoCase = std::unordered_set<KxDynamicStringA, StringHashNoCase, StringEqualToNoCase>;
}
