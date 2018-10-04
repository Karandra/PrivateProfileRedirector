#pragma once
#include "stdafx.h"
#include "ScriptExtenderDefines.h"

xSE_API(bool) xSE_QUERYFUNCTION(const xSE_Interface* xSE, PluginInfo* info);
xSE_API(bool) xSE_LOADFUNCTION(const xSE_Interface* xSE);

//////////////////////////////////////////////////////////////////////////
class RedirectorSEInterface
{
	using PluginHandle = uint32_t;
	using ConsoleCommandHandler = bool(*)(void*, void*, TESObjectREFR*, void*, void*, void*, double*, void*);

	friend bool xSE_QUERYFUNCTION(const xSE_Interface*, PluginInfo*);
	friend bool xSE_LOADFUNCTION(const xSE_Interface*);

	private:
		template<class T> static void RegisterScaleformFunctionChar(GFxMovieView* view, GFxValue* root, const char* name)
		{
			#if xSE_HAS_SCALEFORM_INTERFACE

			#if xSE_PLATFORM_SKSE || xSE_PLATFORM_SKSE64 || xSE_PLATFORM_SKSEVR || xSE_PLATFORM_F4SE
			::RegisterFunction<T>(root, view, name);
			#endif

			#else
			static_assert(false, "Unsupported xSE platform");
			#endif
		}

	public:
		static RedirectorSEInterface& GetInstance();
		
		static bool RegisterScaleform(GFxMovieView* view, GFxValue* root);
		template<class T> static void RegisterScaleformFunction(GFxMovieView* view, GFxValue* root, const char* name)
		{
			RegisterScaleformFunctionChar<T>(root, view, name);
		}
		template<class T> static void RegisterScaleformFunction(GFxMovieView* view, GFxValue* root, const std::string& name)
		{
			RegisterScaleformFunctionChar<T>(root, view, name.c_str());
		}

	public:
		PluginHandle m_PluginHandle;
		const xSE_Interface* m_XSE = NULL;
		xSE_ScaleformInterface* m_Scaleform = NULL;

		xSE_ConsoleCommandInfo* m_RefreshINICommand = NULL;
		ConsoleCommandHandler m_OriginalRefreshINIHandler = NULL;

	private:
		bool OnQuery(PluginHandle pluginHandle, const xSE_Interface* xSE, xSE_ScaleformInterface* scaleforem);
		bool OnLoad();

		xSE_ConsoleCommandInfo* FindConsoleCommand(const std::string_view& fullName) const;
		void OverrideRefreshINI();

	private:
		RedirectorSEInterface();
		~RedirectorSEInterface();

	public:
		bool CanUseSEFunctions() const;
		PluginHandle GetPluginHandle() const
		{
			return m_PluginHandle;
		}
		
		bool HasSEInterface() const
		{
			return m_XSE != NULL;
		}
		const xSE_Interface* GetSEInterface() const
		{
			return m_XSE;
		}

		bool HasScaleform() const
		{
			return m_Scaleform != NULL;
		}
		xSE_ScaleformInterface* GetScaleform() const
		{
			return m_Scaleform;
		}
};
