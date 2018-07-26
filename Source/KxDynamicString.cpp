#include "stdafx.h"
#include "KxDynamicString.h"

const KxDynamicString KxNullDynamicStrig;
const KxDynamicStringRef KxNullDynamicStrigRef;

KxDynamicString KxDynamicString::Format(const CharT* sFormatString, ...)
{
	KxDynamicString sBuffer;

	va_list argptr;
	va_start(argptr, sFormatString);
	int nCount = _vscwprintf(sFormatString, argptr);
	if (nCount > 0)
	{
		sBuffer.resize((size_t)nCount + 1);
		::vswprintf(sBuffer.data(), sBuffer.size(), sFormatString, argptr);
	}
	va_end(argptr);
	return sBuffer;
}
KxDynamicString KxDynamicString::to_utf16(const char* sText)
{
	KxDynamicString sOut;
	int iReqLength = MultiByteToWideChar(CP_UTF8, 0, sText, -1, NULL, 0);
	if (iReqLength != 0)
	{
		sOut.resize((size_t)iReqLength);
		MultiByteToWideChar(CP_UTF8, 0, sText, -1, sOut.data(), iReqLength);
	}
	return sOut;
}
std::string KxDynamicString::to_utf8(const WCHAR* sText)
{
	std::string sOut;
	int iReqLength = WideCharToMultiByte(CP_UTF8, 0, sText, -1, NULL, 0, NULL, NULL);
	if (iReqLength != 0)
	{
		sOut.resize((size_t)iReqLength);
		WideCharToMultiByte(CP_UTF8, 0, sText, -1, sOut.data(), iReqLength, NULL, NULL);
	}
	return sOut;
}
