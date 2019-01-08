#pragma once
#include "stdafx.h"
#include "KxBasicDynamicString.h"

using KxDynamicStringA = KxBasicDynamicString<char, MAX_PATH>;
using KxDynamicStringRefA = KxDynamicStringA::TStringView;

using KxDynamicStringW = KxBasicDynamicString<wchar_t, MAX_PATH>;
using KxDynamicStringRefW = KxDynamicStringW::TStringView;

extern const KxDynamicStringA KxNullDynamicStringA;
extern const KxDynamicStringRefA KxNullDynamicStringRefA;

extern const KxDynamicStringW KxNullDynamicStringW;
extern const KxDynamicStringRefW KxNullDynamicStringRefW;
