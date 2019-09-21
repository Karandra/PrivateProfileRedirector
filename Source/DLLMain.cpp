#include "stdafx.h"
#include "PrivateProfileRedirector.h"

BOOL APIENTRY DllMain(HMODULE module, DWORD event, LPVOID lpReserved)
{
	return PPR::Redirector::DllMain(module, event);
}
void DummyFunction()
{
}
