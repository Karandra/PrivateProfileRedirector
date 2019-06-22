#pragma once
#include "stdafx.h"
#include "ScriptExtenderDefines.h"

xSE_API(bool) xSE_QUERYFUNCTION(const xSE_Interface* xSE, PluginInfo* info);
xSE_API(bool) xSE_LOADFUNCTION(const xSE_Interface* xSE);

namespace PPR
{
	class SEInterface final
	{
		using PluginHandle = uint32_t;
		using ConsoleCommandHandler = bool(*)(void*, void*, TESObjectREFR*, void*, void*, void*, double*, void*);

		friend bool ::xSE_QUERYFUNCTION(const xSE_Interface*, PluginInfo*);
		friend bool ::xSE_LOADFUNCTION(const xSE_Interface*);

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
			static SEInterface& GetInstance();
		
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
			const xSE_Interface* m_XSE = nullptr;
			xSE_ScaleformInterface* m_Scaleform = nullptr;
			bool m_CanUseSEFunctions = false;

			xSE_ConsoleCommandInfo* m_RefreshINICommand = nullptr;
			ConsoleCommandHandler m_OriginalRefreshINIHandler = nullptr;

		private:
			bool OnQuery(PluginHandle pluginHandle, const xSE_Interface* xSE, xSE_ScaleformInterface* scaleform);
			bool OnCheckVersion(uint32_t interfaceVersion, uint32_t compiledVersion);
			bool OnLoad();

			xSE_ConsoleCommandInfo* FindConsoleCommand(const std::string_view& fullName) const;
			void OverrideRefreshINI();

		private:
			SEInterface();
			~SEInterface();

		public:
			bool CanUseSEFunctions() const;
			PluginHandle GetPluginHandle() const
			{
				return m_PluginHandle;
			}
		
			bool HasSEInterface() const
			{
				return m_XSE != nullptr;
			}
			const xSE_Interface* GetSEInterface() const
			{
				return m_XSE;
			}

			bool HasScaleform() const
			{
				return m_Scaleform != nullptr;
			}
			xSE_ScaleformInterface* GetScaleform() const
			{
				return m_Scaleform;
			}
	};
}
