#pragma once
#define KXF_STATIC_LIBRARY 1
#define _CRT_SECURE_NO_WARNINGS 1
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS 1

#include <kxf/pch.hpp>
#include <kxf/Core/FlagSet.h>
#include <kxf/Core/String.h>
#include <kxf/Core/Version.h>
#include <kxf/Log/ScopedLogger.h>
#include <kxf/Utility/Common.h>
#include <kxf/Utility/ScopeGuard.h>

namespace PPR
{
	using kxf::operator|;
}
