#pragma once
#include "stdafx.h"

namespace PPR::Utility
{
	class LibraryHandle final
	{
		private:
			HMODULE m_Handle = nullptr;

		public:
			LibraryHandle() = default;
			LibraryHandle(const wchar_t* path)
				:m_Handle(::LoadLibraryW(path))
			{
			}
			~LibraryHandle()
			{
				if (m_Handle)
				{
					::FreeLibrary(m_Handle);
				}
			}

		public:
			template<class T = void> T GetFunction(const char* name) const
			{
				if (m_Handle)
				{
					return reinterpret_cast<T>(::GetProcAddress(m_Handle, name));
				}
				return nullptr;
			}
			template<class T> bool GetFunction(T& ref, const char* name) const
			{
				ref = GetFunction<std::remove_reference_t<T>>(name);
				return ref != nullptr;
			}

		public:
			explicit operator bool() const
			{
				return m_Handle != nullptr;
			}
			bool operator!() const
			{
				return m_Handle == nullptr;
			}

			operator HMODULE() const
			{
				return m_Handle;
			}
	};
}
