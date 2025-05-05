#pragma once
#include "Common.h"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#undef GetPrivateProfileSectionNames
#undef GetPrivateProfileSection
#undef GetPrivateProfileString
#undef GetPrivateProfileInt
#undef WritePrivateProfileString

#include <kxf/pch.hpp>
#include <kxf/Win32/LinkLibs-GUI.h>
#include <kxf/Win32/UndefMacros.h>
