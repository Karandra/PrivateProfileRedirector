#pragma once
#include "stdafx.h"
#include "String.h"
#include "PrivateProfileRedirector.h"

namespace PPR::Utility::String::Internal
{
	KxDynamicStringW ToUTF16(KxDynamicStringRefA value)
	{
		PrivateProfileRedirector::GetInstance().ConvertToUTF16(value.data(), static_cast<int>(value.length()));
	}
}
