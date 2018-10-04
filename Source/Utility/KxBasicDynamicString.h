#pragma once
#include <string>
#include <string_view>
#include <array>
#include <memory>
#include <type_traits>
#include <variant>
#include <cstdio>
#include <cwchar>

template<class t_CharType, size_t t_StaticStorageLength>
class KxBasicDynamicStringStore
{
	using StdStringViewT = std::basic_string_view<t_CharType, std::char_traits<t_CharType>>;

	private:
		std::array<t_CharType, t_StaticStorageLength> m_Buffer;
		size_t m_Size = 0;

	public:
		const t_CharType* data() const noexcept
		{
			return m_Buffer.data();
		}
		t_CharType* data() noexcept
		{
			return m_Buffer.data();
		}

		constexpr size_t max_size() const noexcept
		{
			return t_StaticStorageLength;
		}
		size_t size() const noexcept
		{
			return m_Size;
		}

		void set_size(size_t size)
		{
			m_Size = std::min(max_size() - 1, size);
		}
		void set_eos()
		{
			m_Buffer[m_Size] = t_CharType();
		}
		void set_size_eos(size_t size)
		{
			set_size(size);
			set_eos();
		}

		void assign(const t_CharType* data, size_t size, size_t startAt = 0)
		{
			const size_t maxToCopy = sizeof(t_CharType) * std::min(size, m_Buffer.size());
			std::memmove(m_Buffer.data() + startAt, data, maxToCopy);
			set_size_eos(startAt + size);
		}
		void assign(const StdStringViewT& view, size_t startAt = 0)
		{
			assign(view.data(), view.size(), startAt);
		}
		void assign(size_t count, t_CharType c, size_t startAt = 0)
		{
			for (size_t i = startAt; i < m_Size; i++)
			{
				m_Buffer[i] = c;
			}
			set_size_eos(startAt + count);
		}

		void append(const t_CharType* data, size_t size)
		{
			assign(data, size, m_Size);
		}
		void append(const StdStringViewT& view)
		{
			append(view.data(), view.size());
		}
		void append(size_t count, t_CharType c)
		{
			assign(count, c, m_Size);
		}

		bool empty() const noexcept
		{
			return m_Size == 0;
		}
		void clear()
		{
			set_size_eos(0);
			m_Buffer.fill(t_CharType());
		}

		const t_CharType& front() const
		{
			return m_Buffer.front();
		}
		t_CharType& front()
		{
			return m_Buffer.front();
		}

		const t_CharType& back() const
		{
			return m_Buffer.back();
		}
		t_CharType& back()
		{
			return m_Buffer.back();
		}

		const t_CharType& operator[](size_t i) const
		{
			return m_Buffer[i];
		}
		t_CharType& operator[](size_t i)
		{
			return m_Buffer[i];
		}

	public:
		auto begin()
		{
			return m_Buffer.begin();
		}
		auto end()
		{
			return m_Buffer.end();
		}

		const auto begin() const
		{
			return m_Buffer.begin();
		}
		const auto end() const
		{
			return m_Buffer.end();
		}
};

//////////////////////////////////////////////////////////////////////////
template<class t_CharType, size_t t_StaticStorageLength, class t_Traits = std::char_traits<t_CharType>, class t_Allocator = std::allocator<t_CharType>>
class KxBasicDynamicString
{
	public:
		// Std types
		using traits_type = t_Traits;
		using value_type = t_CharType;
		using allocator_type = t_Allocator;
		using size_type = typename std::allocator_traits<t_Allocator>::size_type;
		using difference_type = typename std::allocator_traits<t_Allocator>::difference_type;
		
		using reference = value_type&;
		using const_reference = const value_type&;
		
		using pointer = typename std::allocator_traits<allocator_type>::pointer;
		using const_pointer = typename std::allocator_traits<allocator_type>::const_pointer;

		// Iterators
		using iterator = value_type*;
		using const_iterator = const value_type*;

		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = const std::reverse_iterator<iterator>;

		// KxBasicDynamicString types
		using CharType = t_CharType;
		using StdStringType = std::basic_string<value_type, traits_type, allocator_type>;
		using StdStringViewType = std::basic_string_view<value_type, traits_type>;
		using StaticStorageType = KxBasicDynamicStringStore<value_type, t_StaticStorageLength>;

		// Constants
		static const constexpr size_t npos = StdStringType::npos;

	private:
		std::variant<StaticStorageType, StdStringType> m_Storage;

	private:
		// Store checks
		bool can_use_static_store_for(size_t length) const noexcept
		{
			return length < t_StaticStorageLength;
		}
		bool can_use_static_store_to_append(size_t length) const
		{
			return get_static_store().size() + length < t_StaticStorageLength;
		}

		// Store getters
		const StaticStorageType& get_static_store() const
		{
			return std::get<StaticStorageType>(m_Storage);
		}
		StaticStorageType& get_static_store()
		{
			return std::get<StaticStorageType>(m_Storage);
		}

		const StdStringType& get_dynamic_store() const
		{
			return std::get<StdStringType>(m_Storage);
		}
		StdStringType& get_dynamic_store()
		{
			return std::get<StdStringType>(m_Storage);
		}

		// Assign
		StaticStorageType& assgin_static_store()
		{
			m_Storage = StaticStorageType();
			return get_static_store();
		}
		StdStringType& assgin_dynamic_store()
		{
			m_Storage = StdStringType();
			return get_dynamic_store();
		}

		void move_to_dynamic_store(size_t reserve = 0)
		{
			// Make copy
			StaticStorageType staticStore = get_static_store();
			StdStringType& dynamicStore = assgin_dynamic_store();

			dynamicStore.reserve(std::max(reserve, staticStore.size()));
			dynamicStore.assign(staticStore.data(), staticStore.size());
		}
		void move_to_static_store()
		{
			// Move
			StdStringType store = std::move(std::get<StdStringType>(m_Storage));
			assgin_static_store().assign(store.data(), store.size());
		}

		void do_assing(const StdStringViewType& view)
		{
			if (can_use_static_store_for(view.size()))
			{
				assgin_static_store().assign(view);
			}
			else
			{
				assgin_dynamic_store().assign(view);
			}
		}
		void do_assing(size_t count, value_type c)
		{
			if (can_use_static_store_for(count))
			{
				assgin_static_store().assign(count, c);
			}
			else
			{
				assgin_dynamic_store().assign(count, c);
			}
		}

		void do_append(const StdStringViewType& view)
		{
			if (using_static_store())
			{
				if (!can_use_static_store_to_append(view.size()))
				{
					move_to_dynamic_store();
				}
				else
				{
					get_static_store().append(view);
					return;
				}
			}

			get_dynamic_store().append(view);
		}
		void do_append(size_t count, value_type c)
		{
			if (using_static_store())
			{
				if (!can_use_static_store_to_append(count))
				{
					move_to_dynamic_store();
				}
				else
				{
					get_static_store().append(count, c);
					return;
				}
			}

			get_dynamic_store().append(count, c);
		}

	public:
		KxBasicDynamicString() {}
		KxBasicDynamicString(const value_type* data, size_t length = npos)
		{
			if (length != npos)
			{
				do_assing(StdStringViewType(data, length));
			}
			else
			{
				do_assing(StdStringViewType(data));
			}
		}
		KxBasicDynamicString(const StdStringViewType& view)
		{
			do_assing(view);
		}
		KxBasicDynamicString(const StdStringType& s)
		{
			do_assing(s);
		}
		KxBasicDynamicString(const KxBasicDynamicString& other)
		{
			do_assing(other.get_view());
		}

	public:
		// Properties
		bool using_static_store() const noexcept
		{
			return m_Storage.index() == 0;
		}
		bool using_dynamic_store() const noexcept
		{
			return !using_static_store();
		}

		constexpr size_t max_size_static() const noexcept
		{
			return t_StaticStorageLength;
		}
		constexpr size_t max_size_dynamic() const noexcept
		{
			return std::numeric_limits<size_type>::max();
		}
		constexpr size_t max_size() const noexcept
		{
			return max_size_dynamic();
		}
		size_t capacity() const noexcept
		{
			return using_static_store() ? max_size_static() : get_dynamic_store().capacity();
		}

		size_t size() const noexcept
		{
			return using_static_store() ? get_static_store().size() : get_dynamic_store().size();
		}
		size_t length() const noexcept
		{
			return size();
		}
		bool empty() const noexcept
		{
			return size() == 0;
		}

		// Getters
		value_type* data() noexcept
		{
			return using_static_store() ? get_static_store().data() : get_dynamic_store().data();
		}
		const value_type* data() const noexcept
		{
			return using_static_store() ? get_static_store().data() : get_dynamic_store().data();
		}
		const value_type* c_str() const noexcept
		{
			return data();
		}
		
		StdStringViewType get_view() const noexcept
		{
			if (using_static_store())
			{
				const StaticStorageType& store = get_static_store();
				return StdStringViewType(store.data(), store.size());
			}
			else
			{
				return get_dynamic_store();
			}
		}
		StdStringViewType get_view(size_t offset, size_t count = npos) const
		{
			StdStringViewType view = get_view();
			if (offset < view.size())
			{
				return view.substr(offset, count);
			}
			return StdStringViewType();
		}

		operator const value_type*() const noexcept
		{
			return data();
		}
		operator StdStringViewType() const noexcept
		{
			return get_view();
		}

		value_type& front()
		{
			return data()[0];
		}
		const value_type& front() const
		{
			return get_view().front();
		}
		
		value_type& back()
		{
			return data()[size() - 1];
		}
		const value_type& back() const
		{
			return get_view().back();
		}

		value_type& at(size_t index)
		{
			if (index < size())
			{
				return data()[index];
			}
			throw std::runtime_error("invalid index");
		}
		const value_type& at(size_t index) const
		{
			return get_view().at(index);
		}

		value_type& operator[](size_type index)
		{
			return data()[index];
		}
		const value_type& operator[](size_type index) const
		{
			return data()[index];
		}

		// Modifiers
		void clear() noexcept
		{
			if (using_static_store())
			{
				get_static_store().clear();
			}
			else
			{
				get_dynamic_store().clear();
				move_to_static_store();
			}
		}
		void shrink_to_fit()
		{
			if (using_dynamic_store())
			{
				StdStringType& store = get_dynamic_store();
				if (can_use_static_store_for(store.size()))
				{
					move_to_static_store();
				}
				else
				{
					store.shrink_to_fit();
				}
			}
		}
		void reserve(size_t newSize = 0)
		{
			if (using_static_store() && !can_use_static_store_for(newSize))
			{
				move_to_dynamic_store(newSize);
			}
			else if (using_dynamic_store())
			{
				get_dynamic_store().reserve(newSize);
			}
		}
		void resize(size_t newSize, value_type c = value_type())
		{
			if (using_static_store())
			{
				if (can_use_static_store_for(newSize))
				{
					if (size() < newSize)
					{
						do_append(newSize - size(), c);
					}
					else
					{
						get_static_store().set_size_eos(newSize);
					}
					return;
				}
				else
				{
					move_to_dynamic_store(newSize);
				}
			}
			get_dynamic_store().resize(newSize, c);
		}
		void swap(KxBasicDynamicString& other)
		{
			std::swap(this->m_Storage, other.m_Storage);
		}
		
		void erase(size_t offset)
		{
			if (using_static_store())
			{
				StaticStorageType& store = get_static_store();
				if (offset > store.size())
				{
					throw std::out_of_range("erase");
				}
				else
				{
					store.set_size_eos(offset);
				}
			}
			else
			{
				get_dynamic_store().erase(offset);
			}
		}
		void erase(size_t offset, size_t count)
		{
			if (using_static_store())
			{
				StaticStorageType& store = get_static_store();
				const size_t stringSize = size();

				if (offset > stringSize)
				{
					throw std::out_of_range("erase");
				}
				else if (offset < stringSize && count != 0)
				{
					StdStringViewType view = get_view(offset + std::min(count, stringSize));
					store.assign(view, offset);
					store.set_size_eos(stringSize - count);
				}
			}
			else
			{
				get_dynamic_store().erase(offset, count);
			}
		}
		
		// Extraction
		KxBasicDynamicString substr(size_t offset = 0, size_t count = npos) const
		{
			return get_view(offset, count);
		}
		KxBasicDynamicString before_last(value_type ch, KxBasicDynamicString* rest = NULL) const
		{
			KxBasicDynamicString out;
			size_t charPos = rfind(ch);
			if (charPos != npos)
			{
				if (charPos != 0)
				{
					out.assign(get_view(0, charPos));
				}

				if (rest)
				{
					rest->assign(get_view(charPos + 1, npos));
				}
			}
			else
			{
				if (rest)
				{
					*rest = *this;
				}
			}
			return out;
		}

		// Assign
		KxBasicDynamicString& assign(const KxBasicDynamicString& other)
		{
			if (this != &other)
			{
				do_assing(other.get_view());
			}
			return *this;
		}
		KxBasicDynamicString& assign(const StdStringViewType& view)
		{
			do_assing(view);
			return *this;
		}
		KxBasicDynamicString& assign(const value_type* data, size_t length = npos)
		{
			if (length != npos)
			{
				do_assing(StdStringViewType(data, length));
			}
			else
			{
				do_assing(StdStringViewType(data));
			}
			return *this;
		}
		KxBasicDynamicString& assign(size_t count, value_type c)
		{
			do_assing(count, c);
			return *this;
		}

		// Append
		KxBasicDynamicString& append(const KxBasicDynamicString& other)
		{
			// If this is the same object and it can't hold new string in its static store
			// it will relocate the static store to dynamic invalidating it, so copy static store first.
			if (this == &other && using_static_store() && !can_use_static_store_to_append(other.size()))
			{
				StaticStorageType store = other.get_static_store();
				do_append(StdStringViewType(store.data(), store.size()));
			}
			else
			{
				do_append(other.get_view());
			}
			return *this;
		}
		KxBasicDynamicString& append(const StdStringViewType& view)
		{
			do_append(view);
			return *this;
		}
		KxBasicDynamicString& append(const StdStringType& s)
		{
			do_append(s);
			return *this;
		}
		KxBasicDynamicString& append(const value_type* data, size_t length = npos)
		{
			if (length != npos)
			{
				do_append(StdStringViewType(data, length));
			}
			else
			{
				do_append(StdStringViewType(data));
			}
			return *this;
		}
		KxBasicDynamicString& append(size_t count, value_type c)
		{
			do_append(count, c);
			return *this;
		}
		
		KxBasicDynamicString& operator+=(const KxBasicDynamicString& other)
		{
			return append(other);
		}
		KxBasicDynamicString& operator+=(const StdStringViewType& view)
		{
			return append(view);
		}
		KxBasicDynamicString& operator+=(const value_type* data)
		{
			return append(data);
		}
		KxBasicDynamicString& operator+=(value_type c)
		{
			return push_back(c);
		}

		// Push/pop
		KxBasicDynamicString& push_back(value_type c)
		{
			do_append(1, c);
			return *this;
		}
		KxBasicDynamicString& pop_back(value_type c)
		{
			if (using_dynamic_store())
			{
				get_dynamic_store().pop_back();
			}
			else
			{
				StaticStorageType& store = get_static_store();
				if (!store.empty())
				{
					store.set_size_eos(store.size() - 1);
				}
			}
			return *this;
		}

		// Search
		size_t find(const StdStringViewType& pattern, size_t pos = 0) const
		{
			return get_view().find(pattern, pos);
		}
		size_t find(value_type c, size_t pos = 0) const
		{
			return get_view().find(c, pos);
		}

		size_t rfind(const StdStringViewType& pattern, size_t pos = npos) const
		{
			return get_view().rfind(pattern, pos);
		}
		size_t rfind(value_type c, size_t pos = npos) const
		{
			return get_view().rfind(c, pos);
		}

		size_t find_first_of(const StdStringViewType& pattern, size_t pos = 0) const
		{
			return get_view().find_first_of(pattern, pos);
		}
		size_t find_first_of(value_type c, size_t pos = 0) const
		{
			return get_view().find_first_of(c, pos);
		}

		size_t find_first_not_of(const StdStringViewType& pattern, size_t pos = 0) const
		{
			return get_view().find_first_not_of(pattern, pos);
		}
		size_t find_first_not_of(value_type c, size_t pos = 0) const
		{
			return get_view().find_first_not_of(c, pos);
		}

		size_t find_last_of(const StdStringViewType& pattern, size_t pos = npos) const
		{
			return get_view().find_last_of(pattern, pos);
		}
		size_t find_last_of(value_type c, size_t pos = npos) const
		{
			return get_view().find_last_of(c, pos);
		}

		size_t find_last_not_of(const StdStringViewType& pattern, size_t pos = npos) const
		{
			return get_view().find_last_not_of(pattern, pos);
		}
		size_t find_last_not_of(value_type c, size_t pos = npos) const
		{
			return get_view().find_last_not_of(c, pos);
		}

		// Comparison
		int compare(const KxBasicDynamicString& other) const
		{
			return get_view().compare(other.get_view());
		}
		int compare(const StdStringViewType& view) const
		{
			return get_view().compare(view);
		}
		int compare(const value_type* data) const
		{
			return get_view().compare(data);
		}

		bool operator==(const value_type* s) const
		{
			return get_view() == s;
		}
		bool operator==(const KxBasicDynamicString& other) const
		{
			return this == &other || get_view() == other.get_view();
		}

		bool operator!=(const value_type* s) const
		{
			return !(*this == s);
		}
		bool operator!=(const KxBasicDynamicString& other) const
		{
			return !(*this == other);
		}

		bool operator<(const value_type* s) const
		{
			return get_view() < s;
		}
		bool operator<(const KxBasicDynamicString& other) const
		{
			return this != &other && get_view() < other.get_view();
		}

		bool operator>(const value_type* s) const
		{
			return get_view() > s;
		}
		bool operator>(const KxBasicDynamicString& other) const
		{
			return this != &other && get_view() > other.get_view();
		}

		bool operator<=(const value_type* s) const
		{
			return get_view() <= s;
		}
		bool operator<=(const KxBasicDynamicString& other) const
		{
			return this == &other || get_view() <= other.get_view();
		}

		bool operator>=(const value_type* s) const
		{
			return get_view() >= s;
		}
		bool operator>=(const KxBasicDynamicString& other) const
		{
			return this == &other || get_view() >= other.get_view();
		}

		// Iterators
		iterator begin()
		{
			return data();
		}
		iterator end()
		{
			return data() + size();
		}
		const_iterator begin() const
		{
			return data();
		}
		const_iterator end() const
		{
			return data() + size();
		}

		reverse_iterator rbegin()
		{
			return std::make_reverse_iterator(data() + size());
		}
		reverse_iterator rend()
		{
			return std::make_reverse_iterator(data());
		}
		const_reverse_iterator rbegin() const
		{
			return std::make_reverse_iterator(data() + size());
		}
		const_reverse_iterator rend() const
		{
			return std::make_reverse_iterator(data());
		}

		// Upper/lower
		KxBasicDynamicString& make_lower()
		{
			if constexpr(std::is_same_v<value_type, wchar_t>)
			{
				::CharLowerBuffW(data(), static_cast<DWORD>(size()));
			}
			else if constexpr(std::is_same_v<value_type, char>)
			{
				::CharLowerBuffA(data(), static_cast<DWORD>(size()));
			}
			else
			{
				static_assert(false, "function 'KxBasicDynamicString::make_lower' is unavailable for this char type");
			}
			return *this;
		}
		KxBasicDynamicString& make_upper()
		{
			if constexpr(std::is_same_v<value_type, wchar_t>)
			{
				::CharUpperBuffW(data(), static_cast<DWORD>(size()));
			}
			else if constexpr(std::is_same_v<value_type, char>)
			{
				::CharUpperBuffA(data(), static_cast<DWORD>(size()));
			}
			else
			{
				static_assert(false, "function 'KxBasicDynamicString::make_upper' is unavailable for this char type");
			}
			return *this;
		}

		KxBasicDynamicString to_lower() const
		{
			KxBasicDynamicString out(*this);
			out.make_lower();
			return out;
		}
		KxBasicDynamicString to_upper() const
		{
			KxBasicDynamicString out(*this);
			out.make_upper();
			return out;
		}

		// Encoding conversion
		template<class StringT = KxBasicDynamicString<wchar_t, t_StaticStorageLength, std::char_traits<wchar_t>, std::allocator<wchar_t>>>
		static StringT to_utf16(const char* text, int length, int codePage)
		{
			StringT converted;
			int lengthRequired = ::MultiByteToWideChar(codePage, 0, text, length, NULL, 0);
			if (lengthRequired != 0)
			{
				converted.resize(static_cast<size_t>(lengthRequired + 1));
				::MultiByteToWideChar(codePage, 0, text, length, converted.data(), lengthRequired);
				converted.resize(static_cast<size_t>(lengthRequired - 1));
			}
			return converted;
		}

		template<class StringT = KxBasicDynamicString<char, t_StaticStorageLength, std::char_traits<char>, std::allocator<char>>>
		static StringT to_codepage(const wchar_t* text, int length, int codePage)
		{
			StringT converted;
			int lengthRequired = ::WideCharToMultiByte(codePage, 0, text, length, NULL, 0, NULL, NULL);
			if (lengthRequired != 0)
			{
				converted.resize(static_cast<size_t>(lengthRequired + 1));
				::WideCharToMultiByte(codePage, 0, text, length, converted.data(), lengthRequired, NULL, NULL);
				converted.resize(static_cast<size_t>(lengthRequired - 1));
			}
			return converted;
		}

		template<class StringT = KxBasicDynamicString<char, t_StaticStorageLength, std::char_traits<char>, std::allocator<char>>>
		static StringT to_utf8(const wchar_t* text, int length, int codePage)
		{
			return to_codepage(text, length, CP_UTF8);
		}

		// Formatting
		template<class CharT = value_type, class StringT = KxBasicDynamicString<CharT, t_StaticStorageLength, std::char_traits<CharT>, std::allocator<CharT>>>
		static StringT Format(const CharT* formatString, ...)
		{
			StringT buffer;

			va_list argptr;
			va_start(argptr, formatString);
			int count = 0;

			if constexpr (std::is_same_v<CharT, wchar_t>)
			{
				count = _vscwprintf(formatString, argptr);
			}
			else if constexpr (std::is_same_v<CharT, char>)
			{
				count = _vscprintf(formatString, argptr);
			}
			else
			{
				static_assert(false, "function 'KxBasicDynamicString::Format' is unavailable for this char type");
			}

			if (count > 0)
			{
				buffer.resize((size_t)count + 1);

				if constexpr (std::is_same_v<CharT, wchar_t>)
				{
					count = vswprintf(buffer.data(), buffer.size(), formatString, argptr);
				}
				else if constexpr (std::is_same_v<CharT, char>)
				{
					count = vsprintf(buffer.data(), buffer.size(), formatString, argptr);
				}
			}
			va_end(argptr);
			return buffer;
		}
};

//////////////////////////////////////////////////////////////////////////
template<class t_CharType, size_t t_StaticStorageLength, class t_Traits, class t_Allocator>
struct std::hash<KxBasicDynamicString<t_CharType, t_StaticStorageLength, t_Traits, t_Allocator>>
{
	public:
		using argument_type = KxBasicDynamicString<t_CharType, t_StaticStorageLength, t_Traits, t_Allocator>;
		using result_type = size_t;

	public:
		size_t operator()(const argument_type& s) const noexcept
		{
			return std::hash<argument_type::StdStringViewType>()(s.get_view());
		}
};
