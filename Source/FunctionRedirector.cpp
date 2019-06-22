#include "stdafx.h"
#include "FunctionRedirector.h"
#include "PrivateProfileRedirector.h"
#include <detours.h>
#include <detver.h>

namespace PPR::FunctionRedirector::Internal
{
	void LogOperation(uint32_t status, const wchar_t* operation, const FunctionInfo& info)
	{
		Redirector& redirector = Redirector::GetInstance();
		switch (status)
		{
			case NO_ERROR:
			{
				redirector.Log(L"[%s]: %s -> NO_ERROR", operation, info.Name);
				break;
			}
			case ERROR_INVALID_BLOCK:
			{
				redirector.Log(L"[%s]: %s -> ERROR_INVALID_BLOCK", operation, info.Name);
				break;
			}
			case ERROR_INVALID_HANDLE:
			{
				redirector.Log(L"[%s]: %s -> ERROR_INVALID_HANDLE", operation, info.Name);
				break;
			}
			case ERROR_INVALID_OPERATION:
			{
				redirector.Log(L"[%s]: %s -> ERROR_INVALID_OPERATION", operation, info.Name);
				break;
			}
			case ERROR_NOT_ENOUGH_MEMORY:
			{
				redirector.Log(L"[%s]: %s -> ERROR_NOT_ENOUGH_MEMORY", operation, info.Name);
				break;
			}
			default:
			{
				redirector.Log(L"[%s]: %s -> <Unknown>", operation, info.Name);
				break;
			}
		};
	}
	void LogOperation(uint32_t status, const wchar_t* operation)
	{
		Redirector& redirector = Redirector::GetInstance();
		switch (status)
		{
			case NO_ERROR:
			{
				redirector.Log(L"[%s]: NO_ERROR", operation);
				break;
			}
			case ERROR_INVALID_BLOCK:
			{
				redirector.Log(L"[%s]: ERROR_INVALID_BLOCK", operation);
				break;
			}
			case ERROR_INVALID_HANDLE:
			{
				redirector.Log(L"[%s]: ERROR_INVALID_HANDLE", operation);
				break;
			}
			case ERROR_INVALID_OPERATION:
			{
				redirector.Log(L"[%s]: ERROR_INVALID_OPERATION", operation);
				break;
			}
			case ERROR_NOT_ENOUGH_MEMORY:
			{
				redirector.Log(L"[%s]: ERROR_NOT_ENOUGH_MEMORY", operation);
				break;
			}
			default:
			{
				redirector.Log(L"[%s]: Unknown error", operation);
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
