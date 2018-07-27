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

KxDynamicString KxDynamicString::to_utf16(const char* text, int length, int codePage)
{
	KxDynamicString converted;
	int lengthRequired = MultiByteToWideChar(codePage, 0, text, length, NULL, 0);
	if (lengthRequired != 0)
	{
		converted.resize(static_cast<size_t>(lengthRequired + 1));
		MultiByteToWideChar(codePage, 0, text, length, converted.data(), lengthRequired);
		converted.resize(static_cast<size_t>(lengthRequired - 1));
	}
	return converted;
}
std::string KxDynamicString::to_codepage(const WCHAR* text, int length, int codePage)
{
	std::string converted;
	int lengthRequired = WideCharToMultiByte(codePage, 0, text, length, NULL, 0, NULL, NULL);
	if (lengthRequired != 0)
	{
		converted.resize(static_cast<size_t>(lengthRequired + 1));
		WideCharToMultiByte(codePage, 0, text, length, converted.data(), lengthRequired, NULL, NULL);
		converted.resize(static_cast<size_t>(lengthRequired - 1));
	}
	return converted;
}
