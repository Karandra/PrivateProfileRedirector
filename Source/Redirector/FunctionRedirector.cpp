#include "stdafx.h"
#include "FunctionRedirector.h"
#include "RedirectorInterface.h"
#include <kxf/Log/Categories.h>
#include <detours/detours.h>
#include <detours/detver.h>

namespace PPR::LogCategory
{
	KX_DefineLogCategory(Detours);
}

namespace PPR::FunctionRedirector::Private
{
	void LogOperation(uint32_t status, const wchar_t* operation, const FunctionInfo& info)
	{
		RedirectorInterface& redirector = RedirectorInterface::GetInstance();
		switch (status)
		{
			case NO_ERROR:
			{
				kxf::Log::InfoCategory(LogCategory::Detours, L"[{}]: {} -> NO_ERROR", operation, info.Name);
				break;
			}
			case ERROR_INVALID_BLOCK:
			{
				kxf::Log::InfoCategory(LogCategory::Detours, L"[{}]: {} -> ERROR_INVALID_BLOCK", operation, info.Name);
				break;
			}
			case ERROR_INVALID_HANDLE:
			{
				kxf::Log::InfoCategory(LogCategory::Detours, L"[{}]: {} -> ERROR_INVALID_HANDLE", operation, info.Name);
				break;
			}
			case ERROR_INVALID_OPERATION:
			{
				kxf::Log::InfoCategory(LogCategory::Detours, L"[{}]: {} -> ERROR_INVALID_OPERATION", operation, info.Name);
				break;
			}
			case ERROR_NOT_ENOUGH_MEMORY:
			{
				kxf::Log::InfoCategory(LogCategory::Detours, L"[{}]: {} -> ERROR_NOT_ENOUGH_MEMORY", operation, info.Name);
				break;
			}
			default:
			{
				kxf::Log::InfoCategory(LogCategory::Detours, L"[{}]: {} -> <Unknown-{}>", operation, info.Name, status);
				break;
			}
		};
	}
	void LogOperation(uint32_t status, const wchar_t* operation)
	{
		RedirectorInterface& redirector = RedirectorInterface::GetInstance();
		switch (status)
		{
			case NO_ERROR:
			{
				kxf::Log::InfoCategory(LogCategory::Detours, L"[{}]: NO_ERROR", operation);
				break;
			}
			case ERROR_INVALID_BLOCK:
			{
				kxf::Log::InfoCategory(LogCategory::Detours, L"[{}]: ERROR_INVALID_BLOCK", operation);
				break;
			}
			case ERROR_INVALID_HANDLE:
			{
				kxf::Log::InfoCategory(LogCategory::Detours, L"[{}]: ERROR_INVALID_HANDLE", operation);
				break;
			}
			case ERROR_INVALID_OPERATION:
			{
				kxf::Log::InfoCategory(LogCategory::Detours, L"[{}]: ERROR_INVALID_OPERATION", operation);
				break;
			}
			case ERROR_NOT_ENOUGH_MEMORY:
			{
				kxf::Log::InfoCategory(LogCategory::Detours, L"[{}]: ERROR_NOT_ENOUGH_MEMORY", operation);
				break;
			}
			default:
			{
				kxf::Log::InfoCategory(LogCategory::Detours, L"[{}]: Unknown error '{}'", operation, status);
				break;
			}
		};
	}
	
	bool Initialize()
	{
		DetourRestoreAfterWith();
		return true;
	}
	void Uninitialize()
	{
	}

	uint32_t AttachFunction(void** originalFunc, void* overrideFunc)
	{
		return ::DetourAttach(originalFunc, overrideFunc);
	}
	uint32_t DetachFunction(void** originalFunc, void* overrideFunc)
	{
		return ::DetourDetach(originalFunc, overrideFunc);
	}

	uint32_t BeginTransaction()
	{
		LONG status = ::DetourTransactionBegin();
		LogOperation(status, L"BeginTransaction");

		if (status == NO_ERROR)
		{
			status = ::DetourUpdateThread(::GetCurrentThread());
			LogOperation(status, L"DetourUpdateThread");
		}
		return status;
	}
	uint32_t AbortTransaction()
	{
		const LONG status = ::DetourTransactionAbort();
		LogOperation(status, L"AbortTransaction");

		return status;
	}
	uint32_t CommitTransaction()
	{
		const LONG status = ::DetourTransactionCommit();
		LogOperation(status, L"CommitTransaction");

		return status;
	}
}
