#pragma once
#include "Common.h"
#include "ScriptExtenderDefinesBase.h"

bool xSE_CAN_USE_SCRIPTEXTENDER() noexcept;

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

#else

#define xSE_HAS_SE_LOG 1

#endif

template<class TFormat, class... Args>
void xSE_LOG_LEVEL(kxf::LogLevel logLevel, const TFormat& format, Args&&... arg)
{
	kxf::ScopedLoggerAutoScope scope;

	kxf::ScopedMessageLogger message(scope, logLevel);
	message.Format(format, std::forward<Args>(arg)...);

	#if xSE_HAS_SE_LOG
	if (xSE_CAN_USE_SCRIPTEXTENDER())
	{
		auto utf8 = kxf::String::ToUTF8(message.ToString());
		switch (logLevel)
		{
			case kxf::LogLevel::Critical:
			{
				::_FATALERROR("[%s] %s", xSE_NAME_A, utf8.c_str());
				break;
			}
			case kxf::LogLevel::Error:
			{
				::_ERROR("[%s] %s", xSE_NAME_A, utf8.c_str());
				break;
			}
			case kxf::LogLevel::Warning:
			{
				::_WARNING("[%s] %s", xSE_NAME_A, utf8.c_str());
				break;
			}
			case kxf::LogLevel::Debug:
			{
				::_DMESSAGE("[%s] %s", xSE_NAME_A, utf8.c_str());
				break;
			}
			case kxf::LogLevel::Trace:
			{
				::_VMESSAGE("[%s] %s", xSE_NAME_A, utf8.c_str());
				break;
			}
			default:
			{
				::_MESSAGE("[%s] %s", xSE_NAME_A, utf8.c_str());
				break;
			}
		};
	}
	#endif
}

template<class TFormat, class... Args>
void xSE_LOG(const TFormat& format, Args&&... arg)
{
	xSE_LOG_LEVEL(kxf::LogLevel::Information, format, std::forward<Args>(arg)...);
}

template<class TFormat, class... Args>
void xSE_LOG_CRITICAL(const TFormat& format, Args&&... arg)
{
	xSE_LOG_LEVEL(kxf::LogLevel::Critical, format, std::forward<Args>(arg)...);
}

template<class TFormat, class... Args>
void xSE_LOG_ERROR(const TFormat& format, Args&&... arg)
{
	xSE_LOG_LEVEL(kxf::LogLevel::Error, format, std::forward<Args>(arg)...);
}

template<class TFormat, class... Args>
void xSE_LOG_WARNING(const TFormat& format, Args&&... arg)
{
	xSE_LOG_LEVEL(kxf::LogLevel::Warning, format, std::forward<Args>(arg)...);
}

template<class TFormat, class... Args>
void xSE_LOG_DEBUG(const TFormat& format, Args&&... arg)
{
	xSE_LOG_LEVEL(kxf::LogLevel::Debug, format, std::forward<Args>(arg)...);
}

template<class TFormat, class... Args>
void xSE_LOG_TRACE(const TFormat& format, Args&&... arg)
{
	xSE_LOG_LEVEL(kxf::LogLevel::Trace, format, std::forward<Args>(arg)...);
}

//////////////////////////////////////////////////////////////////////////
// Other forward declarations
//////////////////////////////////////////////////////////////////////////
struct PluginInfo;
class TESObjectREFR;
class GFxMovieView;
class GFxValue;
