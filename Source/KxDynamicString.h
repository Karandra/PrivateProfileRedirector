#pragma once
#include "stdafx.h"

template<class CharType, size_t StaticStorageSize = MAX_PATH>
class KxBasicDynamicString
{
	public:
		static const constexpr size_t npos = std::wstring::npos;
		using CharT = CharType;
		using StringT = std::basic_string<CharT, std::char_traits<CharT>, std::allocator<CharT>>;
		using BufferT = std::array<CharT, StaticStorageSize>;
		using ViewT = std::basic_string_view<CharT>;

	protected:
		StringT m_HeapString;
		BufferT m_StaticString;
		size_t m_StaticLength = 0;
		bool m_IsUsingHeap = false;

		#if defined _DEBUG
		const CharT* md_StaticStringView = m_StaticString.data();
		#endif

	protected:
		void set_static_length(size_t nLength) noexcept
		{
			m_StaticLength = nLength;
		}
		bool can_use_static(const size_t nNewLength) const noexcept
		{
			return nNewLength < max_size_static();
		}
		bool can_use_static(const ViewT& sv) const noexcept
		{
			return can_use_static(sv.length());
		}
		
		void do_assign(const ViewT& sv)
		{
			if (can_use_static(sv))
			{
				m_IsUsingHeap = false;
				m_StaticLength = sv.length();

				if constexpr(sizeof(CharType) == sizeof(wchar_t))
				{
					::wcsncpy_s(m_StaticString.data(), m_StaticString.size(), sv.data(), sv.length());
				}
				else if constexpr(sizeof(CharType) == sizeof(char))
				{
					::strncpy_s(m_StaticString.data(), m_StaticString.size(), sv.data(), sv.length());
				}
				else
				{
					static_assert(false, "CharType size unsupported");
				}
			}
			else
			{
				m_IsUsingHeap = true;
				m_HeapString.assign(sv.data(), sv.length());
			}
		}
		void do_assign(const StringT& s)
		{
			do_assign(ViewT(s.data(), s.length()));
		}
		void do_move_to_heap()
		{
			m_HeapString.assign(m_StaticString.data(), m_StaticLength);
			m_StaticLength = 0;
			m_IsUsingHeap = true;

			#if defined _DEBUG
			memset(m_StaticString.data(), 0, m_StaticString.size());
			md_StaticStringView = NULL;
			#endif
		}
		void do_push_back(CharT c)
		{
			if (using_static())
			{
				if (can_use_static(length() + 1))
				{
					m_StaticString[m_StaticLength] = c;
					m_StaticString[m_StaticLength + 1] = '\000';
					m_StaticLength++;
				}
				else
				{
					do_move_to_heap();
					m_HeapString.push_back(c);
				}
			}
			else
			{
				m_HeapString.push_back(c);
			}
		}
		void do_pop_back()
		{
			if (using_static())
			{
				if (!empty())
				{
					m_StaticString[m_StaticLength - 1] = '\000';
					m_StaticLength--;
				}
			}
			else
			{
				m_HeapString.pop_back();
			}
		}
		void do_append(const ViewT& sv)
		{
			if (using_static())
			{
				if (m_StaticLength + sv.length() < max_size_static())
				{
					if constexpr(sizeof(CharType) == sizeof(wchar_t))
					{
						::wcsncat_s(m_StaticString.data(), m_StaticString.size(), sv.data(), sv.length());
					}
					else if constexpr(sizeof(CharType) == sizeof(char))
					{
						::strncat_s(m_StaticString.data(), m_StaticString.size(), sv.data(), sv.length());
					}
					else
					{
						static_assert(false, "CharType size unsupported");
					}
					m_StaticLength += sv.length();
				}
				else
				{
					m_HeapString.reserve(m_StaticLength + sv.length());
					do_move_to_heap();
					m_HeapString.append(sv.data(), sv.length());
				}
			}
			else
			{
				m_HeapString.append(sv.data(), sv.length());
			}
		}
		void do_append(size_t nCount, CharT c)
		{
			for (size_t i = 0; i < nCount; i++)
			{
				do_push_back(c);
			}
		}
		void do_set_eos_static()
		{
			m_StaticString[m_StaticLength] = CharT();
		}

	public:
		// Properties
		bool using_heap() const noexcept
		{
			return m_IsUsingHeap;
		}
		bool using_static() const noexcept
		{
			return !using_heap();
		}
		
		size_t max_size_heap() const noexcept
		{
			return m_HeapString.max_size();
		}
		constexpr size_t max_size_static() const noexcept
		{
			return StaticStorageSize;
		}
		size_t max_size() const noexcept
		{
			return max_size_heap();
		}
		size_t capacity() const noexcept
		{
			return using_heap() ? m_HeapString.capacity() : StaticStorageSize;
		}
		size_t length() const noexcept
		{
			return using_static() ? m_StaticLength : m_HeapString.length();
		}
		size_t size() const noexcept
		{
			return length();
		}
		bool empty() const noexcept
		{
			return length() == 0;
		}

		// Memory
		void clear() noexcept
		{
			m_IsUsingHeap = false;
			m_StaticLength = 0;
			m_StaticString[0] = CharT(0);
		}
		void clear_heap() noexcept
		{
			m_HeapString.clear();
		}
		void shrink_to_fit()
		{
			m_HeapString.shrink_to_fit();
		}
		void reserve(size_t nSize = 0)
		{
			if (!can_use_static(nSize))
			{
				m_HeapString.reserve(nSize);
				do_move_to_heap();
			}
		}
		void resize(size_t nSize, CharT c = CharT())
		{
			if (using_static())
			{
				if (can_use_static(nSize))
				{
					nSize = std::min(nSize, max_size_static());
					if (length() < nSize)
					{
						do_append(nSize - length(), c);
					}
					else
					{
						set_static_length(nSize);
						do_set_eos_static();
					}
					return;
				}
				else
				{
					do_move_to_heap();
				}
			}
			m_HeapString.resize(nSize, c);
		}
		void swap(KxBasicDynamicString& other)
		{
			std::swap(this->m_HeapString, other.m_HeapString);
			std::swap(this->m_StaticString, other.m_StaticString);
			std::swap(this->m_StaticLength, other.m_StaticLength);
			std::swap(this->m_IsUsingHeap, other.m_IsUsingHeap);

			#if defined _DEBUG
			std::swap(this->md_StaticStringView, other.md_StaticStringView);
			#endif
		}

		void erase(size_t nOffset, size_t nCount)
		{
			if (nOffset < length() && nCount != 0)
			{
				auto sv = view(nOffset + std::min(nCount, capacity()));
				if (using_static())
				{
					set_static_length(nOffset);
					do_set_eos_static();
					do_append(sv);
				}
				else
				{
					m_HeapString.assign(sv, nOffset);
				}
			}
		}

		// Getters
		ViewT view() const noexcept
		{
			return using_static() ? ViewT(m_StaticString.data(), m_StaticLength) : ViewT(m_HeapString.data(), m_HeapString.length());
		}
		ViewT view(size_t nOffset, size_t nCount = npos) const
		{
			return view().substr(nOffset, nCount);
		}
		
		CharT* data() noexcept
		{
			return using_static() ? m_StaticString.data() : m_HeapString.data();
		}
		const CharT* data() const noexcept
		{
			return using_static() ? m_StaticString.data() : m_HeapString.data();
		}
		const CharT* c_str() const noexcept
		{
			return data();
		}
		operator const CharT*() const noexcept
		{
			return data();
		}
		operator ViewT() const noexcept
		{
			return view();
		}

		CharT& front()
		{
			return data()[0];
		}
		const CharT& front() const
		{
			return view().front();
		}
		CharT& back()
		{
			return data()[length() - 1];
		}
		const CharT& back() const
		{
			return view().back();
		}

		CharT& at(size_t nIndex)
		{
			if (nIndex < length())
			{
				return data()[nIndex];
			}
			ThrowInvalidIndex();
		}
		const CharT& at(size_t nIndex) const
		{
			return view().at(nIndex);
		}
		template<class T> CharT& operator[](T nIndex)
		{
			static_assert(std::is_integral<T>::value, "T must be integral");
			return data()[(size_t)nIndex];
		}
		template<class T>const CharT& operator[](T nIndex) const
		{
			static_assert(std::is_integral<T>::value, "T must be integral");
			return data()[(size_t)nIndex];
		}

		// Comparison
		int compare(const ViewT& sv) const
		{
			return view().compare(sv);
		}
		int compare(const CharT* s) const
		{
			return view().compare(s);
		}
		int compare(const KxBasicDynamicString& other) const
		{
			return view().compare(other.view());
		}

		// Search
		size_t find(const ViewT& sPattern, size_t nPos = 0) const
		{
			return view().find(sPattern, nPos);
		}
		size_t find(CharT c, size_t nPos = 0) const
		{
			return view().find(c, nPos);
		}
		
		size_t rfind(const ViewT& sPattern, size_t nPos = npos) const
		{
			return view().rfind(sPattern, nPos);
		}
		size_t rfind(CharT c, size_t nPos = npos) const
		{
			return view().rfind(c, nPos);
		}

		size_t find_first_of(const ViewT& sPattern, size_t nPos = 0) const
		{
			return view().find_first_of(sPattern, nPos);
		}
		size_t find_first_of(CharT c, size_t nPos = 0) const
		{
			return view().find_first_of(c, nPos);
		}

		size_t find_last_of(const ViewT& sPattern, size_t nPos = 0) const
		{
			return view().find_last_of(sPattern, nPos);
		}
		size_t find_last_of(CharT c, size_t nPos = 0) const
		{
			return view().find_last_of(c, nPos);
		}

	public:
		KxBasicDynamicString()
		{
			clear();
		}
		virtual ~KxBasicDynamicString() = default;

	public:
		bool operator==(const CharT* s) const
		{
			return view() == ViewT(s);
		}
		bool operator==(const KxBasicDynamicString& other) const
		{
			return view() == other.view();
		}

		bool operator!=(const CharT* s) const
		{
			return !(*this == s);
		}
		bool operator!=(const KxBasicDynamicString& other) const
		{
			return !(*this == other);
		}

		bool operator<(const CharT* s) const
		{
			return view() < ViewT(s);
		}
		bool operator<(const KxBasicDynamicString& other) const
		{
			return view() < other.view();
		}

		bool operator>(const CharT* s) const
		{
			return view() > ViewT(s);
		}
		bool operator>(const KxBasicDynamicString& other) const
		{
			return view() > other.view();
		}

		bool operator<=(const CharT* s) const
		{
			return view() <= ViewT(s);
		}
		bool operator<=(const KxBasicDynamicString& other) const
		{
			return view() <= other.view();
		}

		bool operator>=(const CharT* s) const
		{
			return view() >= ViewT(s);
		}
		bool operator>=(const KxBasicDynamicString& other) const
		{
			return view() >= other.view();
		}
};

//////////////////////////////////////////////////////////////////////////
class KxDynamicString: public KxBasicDynamicString<wchar_t>
{
	public:
		static KxDynamicString Format(const CharT* sFormatString, ...);
		
		static KxDynamicString to_utf16(const char* text, int length = -1, int codePage = CP_ACP);
		static std::string to_codepage(const WCHAR* text, int length = -1, int codePage = CP_ACP);
		static std::string to_utf8(const WCHAR* text, int length = -1)
		{
			return to_codepage(text, length, CP_UTF8);
		}

	public:
		KxDynamicString() {}
		KxDynamicString(const ViewT& sv)
		{
			assign(sv);
		}
		KxDynamicString(const StringT& s)
		{
			assign(s);
		}
		KxDynamicString(const CharT* s, size_t nCount = npos)
		{
			assign(s, nCount);
		}
		KxDynamicString(const KxDynamicString& other)
		{
			assign(other);
		}

	public:
		KxDynamicString substr(size_t nPos = 0, size_t nCount = npos) const
		{
			return view().substr(nPos, nCount);
		}
		KxDynamicString before_last(CharT ch, KxDynamicString* pRest = NULL) const
		{
			KxDynamicString sOut;
			size_t nCharPos = rfind(ch);
			if (nCharPos != npos)
			{
				if (nCharPos != 0)
				{
					sOut.assign(view(0, nCharPos));
				}

				if (pRest)
				{
					pRest->assign(view(nCharPos + 1, npos));
				}
			}
			else
			{
				if (pRest)
				{
					*pRest = *this;
				}
			}

			return sOut;
		}

		KxDynamicString& assign(const ViewT& sv)
		{
			do_assign(sv);
			return *this;
		}
		KxDynamicString& assign(const StringT& s)
		{
			do_assign(s);
			return *this;
		}
		KxDynamicString& assign(const CharT* s, size_t nCount = npos)
		{
			do_assign(nCount == npos ? ViewT(s) : ViewT(s, nCount));
			return *this;
		}
		KxDynamicString& assign(const KxDynamicString& other)
		{
			do_assign(other.view());
			return *this;
		}

		KxDynamicString& push_back(CharT c)
		{
			do_push_back(c);
			return *this;
		}
		KxDynamicString& pop_back()
		{
			do_pop_back();
			return *this;
		}

		KxDynamicString& append(const CharT& c)
		{
			return push_back(c);
		}
		KxDynamicString& append(size_t n, const CharT& c)
		{
			for (size_t i = 0; i < n; ++i)
			{
				push_back(c);
			}
			return *this;
		}
		KxDynamicString& append(const ViewT& sv)
		{
			do_append(sv);
			return *this;
		}
		KxDynamicString& append(const StringT& s)
		{
			do_append(s);
			return *this;
		}
		KxDynamicString& append(const CharT* s, size_t nCount = npos)
		{
			do_append(nCount == npos ? ViewT(s) : ViewT(s, nCount));
			return *this;
		}
		KxDynamicString& append(const KxDynamicString& other)
		{
			do_append(other.view());
			return *this;
		}
		KxDynamicString& operator+=(const KxDynamicString& other)
		{
			return append(other);
		}
		KxDynamicString& operator+=(const ViewT& sv)
		{
			return append(sv);
		}
		KxDynamicString& operator+=(const CharT* s)
		{
			return append(s);
		}
		KxDynamicString& operator+=(CharT c)
		{
			return append(c);
		}

		KxDynamicString& make_lower()
		{
			::CharLowerBuffW(data(), (DWORD)length());
			return *this;
		}
		KxDynamicString& make_upper()
		{
			::CharUpperBuffW(data(), (DWORD)length());
			return *this;
		}
		KxDynamicString to_lower() const
		{
			KxDynamicString sOut(*this);
			sOut.make_lower();
			return sOut;
		}
		KxDynamicString to_upper() const
		{
			KxDynamicString sOut(*this);
			sOut.make_upper();
			return sOut;
		}
};

typedef KxDynamicString::ViewT KxDynamicStringRef;
extern const KxDynamicString KxNullDynamicStrig;
extern const KxDynamicStringRef KxNullDynamicStrigRef;

//////////////////////////////////////////////////////////////////////////
template<> struct std::hash<KxDynamicString>
{
	public:
		using argument_type = KxDynamicString;
		using result_type = size_t;

	public:
		size_t operator()(KxDynamicString const& s) const noexcept
		{
			return std::hash<KxDynamicString::ViewT>()(s.view());
		}
};
