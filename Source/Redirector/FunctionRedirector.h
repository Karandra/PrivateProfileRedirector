#pragma once
#include "Common.h"

namespace PPR::FunctionRedirector
{
	struct FunctionInfo final
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

namespace PPR::FunctionRedirector::Private
{
	void LogOperation(uint32_t status, const wchar_t* operation, const FunctionInfo& info);
	void LogOperation(uint32_t status, const wchar_t* operation);

	bool Initialize();
	void Uninitialize();

	uint32_t AttachFunction(void** originalFunc, void* overrideFunc);
	uint32_t DetachFunction(void** originalFunc, void* overrideFunc);

	uint32_t BeginTransaction();
	uint32_t AbortTransaction();
	uint32_t CommitTransaction();
}

namespace PPR::FunctionRedirector
{
	inline bool Initialize()
	{
		KX_SCOPEDLOG_FUNC;

		if (Private::Initialize())
		{
			KX_SCOPEDLOG.SetSuccess();
			return true;
		}
		return false;
	}
	inline void Uninitialize()
	{
		KX_SCOPEDLOG_FUNC;

		Private::Uninitialize();

		KX_SCOPEDLOG.SetSuccess();
	}
	
	template<class T>
	uint32_t AttachFunction(T* originalFunc, T overrideFunc, const FunctionInfo& info)
	{
		const uint32_t status = Private::AttachFunction(reinterpret_cast<void**>(originalFunc), reinterpret_cast<void*>(overrideFunc));
		Private::LogOperation(status, L"AttachFunction", info);
		return status;
	}
	
	template<class T>
	uint32_t DetachFunction(T* originalFunc, T overrideFunc, const FunctionInfo& info)
	{
		const uint32_t status = Private::DetachFunction(reinterpret_cast<void**>(originalFunc), reinterpret_cast<void*>(overrideFunc));
		Private::LogOperation(status, L"DetachFunction", info);
		return status;
	}

	template<class TFunc>
	uint32_t PerformTransaction(TFunc&& func)
	{
		const uint32_t status = Private::BeginTransaction();
		if (status == NO_ERROR)
		{
			using TRet = std::invoke_result_t<TFunc>;
			if constexpr(std::is_same_v<TRet, bool>)
			{
				if (!std::invoke(func))
				{
					return Private::AbortTransaction();
				}
			}
			else
			{
				std::invoke(func);
			}
			return Private::CommitTransaction();
		}
		return status;
	}
}
