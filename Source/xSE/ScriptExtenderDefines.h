#pragma once
#include "stdafx.h"
#include "ScriptExtenderDefinesBase.h"

//////////////////////////////////////////////////////////////////////////
// xSE_Interface
//////////////////////////////////////////////////////////////////////////
#if xSE_PLATFORM_SKSE || xSE_PLATFORM_SKSE64 || xSE_PLATFORM_SKSE64AE || xSE_PLATFORM_SKSEVR

using xSE_Interface = struct SKSEInterface;
#define xSE_INTERFACE_VERSION(xSE)	(xSE)->skseVersion

#elif xSE_PLATFORM_F4SE || xSE_PLATFORM_F4SEVR
using xSE_Interface = struct F4SEInterface;
#define xSE_INTERFACE_VERSION(xSE)	(xSE)->f4seVersion

#elif xSE_PLATFORM_NVSE

using xSE_Interface = struct NVSEInterface;
#define xSE_INTERFACE_VERSION(xSE)	(xSE)->nvseVersion
#define xSE_INTERFACE_NOSE 1

#else
using xSE_Interface = void;
#endif

//////////////////////////////////////////////////////////////////////////
// xSE_ScaleformInterface
//////////////////////////////////////////////////////////////////////////
#if xSE_PLATFORM_SKSE || xSE_PLATFORM_SKSE64 || xSE_PLATFORM_SKSE64AE || xSE_PLATFORM_SKSEVR

using xSE_ScaleformInterface = struct SKSEScaleformInterface;
#define xSE_HAS_SCALEFORM_INTERFACE 1

#elif xSE_PLATFORM_F4SE || xSE_PLATFORM_F4SEVR

using xSE_ScaleformInterface = struct F4SEScaleformInterface;
#define xSE_HAS_SCALEFORM_INTERFACE 1

#else
using xSE_ScaleformInterface = void;
#endif

//////////////////////////////////////////////////////////////////////////
// xSE_MessagingInterface
//////////////////////////////////////////////////////////////////////////
#if xSE_PLATFORM_SKSE || xSE_PLATFORM_SKSE64 || xSE_PLATFORM_SKSE64AE || xSE_PLATFORM_SKSEVR

using xSE_MessagingInterface = struct SKSEMessagingInterface;
#define xSE_HAS_MESSAGING_INTERFACE 1

#elif xSE_PLATFORM_F4SE || xSE_PLATFORM_F4SEVR

using xSE_MessagingInterface = struct F4SEMessagingInterface;
#define xSE_HAS_MESSAGING_INTERFACE 1

#else
using xSE_MessagingInterface = void;
#endif

//////////////////////////////////////////////////////////////////////////
// Console command struct
//////////////////////////////////////////////////////////////////////////
#if xSE_PLATFORM_SKSE64 || xSE_PLATFORM_SKSE64AE || xSE_PLATFORM_SKSEVR || xSE_PLATFORM_F4SE || xSE_PLATFORM_F4SEVR
#define xSE_HAS_CONSOLE_COMMAND_INFO 1
#endif

//////////////////////////////////////////////////////////////////////////
// Logging
//////////////////////////////////////////////////////////////////////////
#if xSE_PLATFORM_NVSE

#define xSE_HAS_SE_LOG 0
#define xSE_LOG(format, ...)	Redirector::GetInstance().Log(L##format, __VA_ARGS__);

#else

#define xSE_HAS_SE_LOG 1

#define xSE_LOG(format, ...) 	\
PPR::Redirector::GetInstance().Log(format, __VA_ARGS__);	\
if (PPR::SEInterface::GetInstance().CanUseSEFunctions())	\
{	\
	_MESSAGE("[" xSE_NAME_A "] " format, __VA_ARGS__);	\
}	\

#endif

//////////////////////////////////////////////////////////////////////////
// Other forward declarations
//////////////////////////////////////////////////////////////////////////
struct PluginInfo;
class TESObjectREFR;
class GFxMovieView;
class GFxValue;
