#pragma once
#include "Common.h"
#include <kxf/System/CFunction.h>
#include <kxf/System/DynamicLibrary.h>
#include <kxf/EventSystem/IEvtHandler.h>

namespace ENBAPI
{
	enum class CallbackType: int32_t
	{
		None = 0,

		EndFrame = 1,
		BeginFrame = 2,
		PreSave = 3,
		PostLoad = 4,
		OnInit = 5,
		OnExit = 6,
		PreReset = 7,
		PostReset = 8
	};
	enum class ParameterType: int32_t
	{
		None = 0,

		Float = 1,
		Int = 2,
		Hex = 3,
		Bool = 4,
		Color3 = 5,
		Color4 = 6,
		Vec3 = 7
	};

	struct ParameterValue
	{
		std::array<uint8_t, 16> Data = {};
		uint32_t Size = 0;
		ParameterType Type = ParameterType::None;
	};
}

namespace ENBAPI
{
	class ENBLink final
	{
		private:
			using ENBGetVersion = int32_t(__cdecl)();
			using ENBGetSDKVersion = int32_t(__cdecl)();
			using ENBCallback = void(__stdcall)(CallbackType type);
			using ENBSetCallbackFunction = void(__cdecl)(ENBCallback* callback);
			using ENBGetParameter = int32_t(__cdecl)(const char* fileName, const char* category, const char* keyname, ParameterValue* value);
			using ENBSetParameter = int32_t(__cdecl)(const char* fileName, const char* category, const char* keyname, ParameterValue* value);

		private:
			kxf::DynamicLibrary m_ENBLib;
			ENBGetVersion* m_ENBGetVersion = nullptr;
			ENBGetSDKVersion* m_ENBGetSDKVersion = nullptr;
			ENBSetCallbackFunction* m_ENBSetCallbackFunction = nullptr;
			ENBGetParameter* m_ENBGetParameter = nullptr;
			ENBSetParameter* m_ENBSetParameter = nullptr;
			bool m_IsLoaded = false;
			
			kxf::CFunction<ENBCallback, kxf::FFI::ABI::StdCall> m_CallbackStore;

		private:
			template<class T>
			T CopyFromRawValue(const ParameterValue& rawValue) const
			{
				T value{};
				std::memcpy(&value, rawValue.Data.data(), sizeof(value));

				return value;
			}

			template<class T>
			void CopyToRawValue(ParameterValue& rawValue, const T& value) const
			{
				std::memcpy(rawValue.Data.data(), &value, sizeof(value));
				rawValue.Size = sizeof(value);
			}

		public:
			ENBLink() noexcept = default;

		public:
			bool IsNull() const noexcept
			{
				return !m_IsLoaded;
			}
			bool Load(const kxf::String& filePath = {});

			void BindCallback(kxf::IEvtHandler& evtHandler);

			std::optional<ParameterValue> RawGetParameter(const kxf::String& fileName, const kxf::String& category, const kxf::String& keyName) const;
			bool RawSetParameter(const kxf::String& fileName, const kxf::String& category, const kxf::String& keyName, const ParameterValue& value);

			template<class T>
			requires(std::is_same_v<T, bool> || std::is_floating_point_v<T> || std::is_integral_v<T>)
			std::optional<T> GetParameter(const kxf::String& fileName, const kxf::String& category, const kxf::String& keyName) const
			{
				if (auto rawValue = RawGetParameter(fileName, category, keyName))
				{
					if constexpr(std::is_same_v<T, bool>)
					{
						if (rawValue->Type == ParameterType::Bool)
						{
							return CopyFromRawValue<int32_t>(*rawValue) != 0;
						}
					}
					else if constexpr(std::is_floating_point_v<T>)
					{
						if (rawValue->Type == ParameterType::Float)
						{
							return static_cast<T>(CopyFromRawValue<float>(*rawValue));
						}
					}
					else if constexpr(std::is_integral_v<T>)
					{
						if (rawValue->Type == ParameterType::Int)
						{
							return static_cast<T>(CopyFromRawValue<int32_t>(*rawValue));
						}
					}
				}
				return {};
			}

			template<class T>
			requires(std::is_same_v<T, bool> || std::is_floating_point_v<T> || std::is_integral_v<T>)
			bool SetParameter(const kxf::String& fileName, const kxf::String& category, const kxf::String& keyName, const T& value)
			{
				ParameterValue rawValue;
				if constexpr(std::is_same_v<T, bool>)
				{
					rawValue.Type = ParameterType::Bool;
					CopyToRawValue(rawValue, static_cast<int32_t>(value ? 1 : 0));
				}
				else if constexpr(std::is_floating_point_v<T>)
				{
					rawValue.Type = ParameterType::Float;
					CopyToRawValue(rawValue, static_cast<float>(value));
				}
				else if constexpr(std::is_integral_v<T>)
				{
					rawValue.Type = ParameterType::Int;
					CopyToRawValue(rawValue, static_cast<int32_t>(value));
				}

				if (rawValue.Type != ParameterType::None && rawValue.Size != 0)
				{
					return RawSetParameter(fileName, category, keyName, rawValue);
				}
				return false;
			}
	};
}
