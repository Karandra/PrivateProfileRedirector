#pragma once
#include "Common.h"
#include <kxf/EventSystem/Event.h>
#include <kxf/Core/IEncodingConverter.h>

namespace PPR
{
	class GameEvent: public kxf::BasicEvent
	{
		public:
			kxf_EVENT_MEMBER(GameEvent, PluginsLoaded);
			kxf_EVENT_MEMBER(GameEvent, InputLoaded);
			kxf_EVENT_MEMBER(GameEvent, DataLoaded);

			kxf_EVENT_MEMBER(GameEvent, NewGame);
			kxf_EVENT_MEMBER(GameEvent, GameSave);
			kxf_EVENT_MEMBER(GameEvent, GameSaved);
			kxf_EVENT_MEMBER(GameEvent, GameLoad);
			kxf_EVENT_MEMBER(GameEvent, GameLoaded);
			kxf_EVENT_MEMBER(GameEvent, DeleteSavedGame);

		private:
			const void* m_Data = nullptr;
			size_t m_Length = 0;

		public:
			GameEvent() = default;
			GameEvent(const void* data, size_t length)
				:m_Data(data), m_Length(length)
			{
			}

		public:
			kxf::String GetSaveFile() const
			{
				if (!m_Data || m_Length == 0)
				{
					return {};
				}

				auto id = GetEventID();
				if (id == EvtGameSave || id == EvtGameLoad || id == EvtDeleteSavedGame)
				{
					return kxf::EncodingConverter_WhateverWorks.ToWideChar(std::basic_string_view(reinterpret_cast<const char*>(m_Data), m_Length));
				}
				return {};
			}
			bool IsGameLoadedSuccessfully() const
			{
				if (GetEventID() == EvtGameLoaded)
				{
					return m_Data != nullptr;
				}
				return false;
			}
	};
}
