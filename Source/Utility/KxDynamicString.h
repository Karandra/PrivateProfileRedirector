#pragma once
#include "stdafx.h"
#include "KxBasicDynamicString.h"

using KxDynamicStringA = KxBasicDynamicString<char, MAX_PATH>;
using KxDynamicStringRefA = typename KxDynamicStringA::TStringView;

using KxDynamicStringW = KxBasicDynamicString<wchar_t, MAX_PATH>;
using KxDynamicStringRefW = typename KxDynamicStringW::TStringView;

extern const KxDynamicStringA KxNullDynamicStringA;
extern const KxDynamicStringRefA KxNullDynamicStringRefA;

extern const KxDynamicStringW KxNullDynamicStringW;
extern const KxDynamicStringRefW KxNullDynamicStringRefW;
