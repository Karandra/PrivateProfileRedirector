#pragma once
#include "stdafx.h"

namespace PPR::FunctionRedirector
{
	enum class Status: uint32_t
	{
		OK = 0,
	};
}

namespace PPR::FunctionRedirector
{
	struct FunctionInfo
	{
		const wchar_t* Name = nullptr;
		void** Original = nullptr;
		void* Override = nullptr;

		FunctionInfo(void** original, void* override, const wchar_t* name)
			:Original(original), Override(override), Name(name)
		{
		}
		FunctionInfo(const wchar_t* name = L"")
			:Name(name)
		{
		}
	};
}

namespace PPR::FunctionRedirector::Internal
{
	void LogOperation(uint32_t status, const wchar_t* operation, const FunctionInfo& info);
	void LogOperation(uint32_t status, const wchar_t* operation);

	uint32_t AttachFunction(void** originalFunc, void* overrideFunc);
	uint32_t DetachFunction(void** originalFunc, void* overrideFunc);

	uint32_t BeginTransaction();
	uint32_t AbortTransaction();
	uint32_t CommitTransaction();
}

namespace PPR::FunctionRedirector
{
	template<class T> uint32_t AttachFunction(T* originalFunc, T overrideFunc, const FunctionInfo& info)
	{
		const uint32_t status = Internal::AttachFunction(reinterpret_cast<void**>(originalFunc), reinterpret_cast<void*>(overrideFunc));
		LogOperation(status, L"AttachFunction", info);
		return status;
	}
	template<class T> uint32_t DetachFunction(T* originalFunc, T overrideFunc, const FunctionInfo& info)
	{
		const uint32_t status = Internal::DetachFunction(reinterpret_cast<void**>(originalFunc), reinterpret_cast<void*>(overrideFunc));
		LogOperation(status, L"DetachFunction", info);
		return status;
	}

	template<class TFunc> uint32_t PerformTransaction(TFunc&& func)
	{
		if (Internal::BeginTransaction() == (uint32_t)Status::OK)
		{
			using TRet = std::invoke_result_t<TFunc>;
			if constexpr(std::is_same_v<TRet, bool>)
			{
				if (!std::invoke(func))
				{
					return Internal::AbortTransaction();
				}
			}
			else
			{
				std::invoke(func);
			}
			return Internal::CommitTransaction();
		}
	}
}
