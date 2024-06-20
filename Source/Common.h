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
	constexpr char ProjectName[] = "PrivateProfileRedirector";
	constexpr char ProjectAuthor[] = "Karandra";

	constexpr int VersionMajor = 0;
	constexpr int VersionMinor = 6;
	constexpr int VersionPatch = 1;

	constexpr int MakeFullVersion(int major, int minor, int patch) noexcept
	{
		// 1.2.3 -> 1 * 100 + 2 * 10 + 3 * 1 = 123
		// 0.1 -> (0 * 100) + (1 * 10) + (0 * 1) = 10
		return (major * 100) + (minor * 10) + (patch * 1);
	}
	constexpr int VersionFull = MakeFullVersion(VersionMajor, VersionMinor, VersionPatch);

	using kxf::operator|;
}
