#pragma once
#include "stdafx.h"
#include "KxBasicDynamicString.h"

using KxDynamicStringA = KxBasicDynamicString<char, MAX_PATH>;
using KxDynamicStringW = KxBasicDynamicString<wchar_t, MAX_PATH>;

extern const KxDynamicStringA KxNullDynamicStringA;
extern const KxDynamicStringA::StdStringViewType KxNullDynamicStringRefA;

extern const KxDynamicStringW KxNullDynamicStringW;
extern const KxDynamicStringW::StdStringViewType KxNullDynamicStringRefW;
