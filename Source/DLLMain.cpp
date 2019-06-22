#include "stdafx.h"
#include "PrivateProfileRedirector.h"

BOOL APIENTRY DllMain(HMODULE module, DWORD event, LPVOID lpReserved)
{
	using namespace PPR;

    switch (event)
    {
		case DLL_PROCESS_ATTACH:
		{
			Redirector::CreateInstance();
			break;
		}
		case DLL_THREAD_DETACH:
		{
			if (Redirector* instance = Redirector::GetInstancePtr())
			{
				if (instance->IsOptionEnabled(RedirectorOption::SaveOnThreadDetach))
				{
					instance->SaveChnagedFiles(L"On thread detach");
				}
			}
			break;
		}
		case DLL_PROCESS_DETACH:
		{
			Redirector::DestroyInstance();
			break;
		}
    }
    return TRUE;
}
void DummyFunction()
{
}
