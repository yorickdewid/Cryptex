// Copyright (c) 2017 Quenza Inc. All rights reserved.
//
// This file is part of the Cryptox project.
//
// Use of this source code is governed by a private license
// that can be found in the LICENSE file. Content can not be 
// copied and/or distributed without the express of the author.

#pragma once

#include <array>
#include <string>

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //
// AUTOGENERATED SOURCE, DO NOT EDIT MANUALLY. RUN THE MESSAGE
// GENERATOR TO UPDATE THE EVENT LOGGER.
//
// Manifest version : 1
// Event items      : 3
// Manifest         : 10-manifest.xml
// Generated        : 28-05-2018 00:05:41
//
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //

#define _EV_MIN_ITEM_ID 10
#define _EV_MAX_ITEM_ID 1000

// List of aliases matching the error code. The alias is optional
// and is generated by the message generator.
enum struct AliasErrorCode
{
	//
};

struct EventLevel
{
	enum Level
	{
		Level1 = 1,
		Level2 = 2,
		Level3 = 3,
	};

	// Get shortcode from level
	static char GetCharCode(Level level) noexcept
	{
		switch (level)
		{
		case Level1: return 'Q';
		case Level2: return 'P';
		case Level3: return 'V';
		}

		return '\0';
	}
};

struct EventItem final
{
	EventLevel::Level level{ EventLevel::Level1 };
	const int errorCode;
	const std::string title;
	const std::string desc;

public:
	EventItem(int code, const std::string& title)
		: errorCode{ code }
		, title{ title }
	{
	}
	EventItem(int code, const std::string& title, const std::string& desc)
		: errorCode{ code }
		, title{ title }
		, desc{ desc }
	{
	}
	EventItem(int code, EventLevel::Level level, const std::string& title)
		: level{ level }
		, errorCode{ code }
		, title{ title }
	{
	}
	EventItem(int code, EventLevel::Level level, const std::string& title, const std::string& desc)
		: level{ level }
		, errorCode{ code }
		, title{ title }
		, desc{ desc }
	{
	}

	std::string InfoLine() const
	{
		return ""
			+ std::string{ "Error: " }
			+ EventLevel::GetCharCode(level)
			+ std::to_string(errorCode)
			+ ": " + title;
	}
};

// List of events generated by the message generator.
static const std::array<EventItem, 3> g_eventItemList = std::array<EventItem, 3>
{
	EventItem{ 10, EventLevel::Level1, "Event 1" },
	EventItem{ 100, EventLevel::Level2, "Event 2" },
	EventItem{ 1000, EventLevel::Level3, "Event 3" },
};

namespace Detail
{

struct MatchEvent final
{
	int m_code;

	inline MatchEvent(int code)
		: m_code{ code }
	{
	}

	inline bool operator()(const EventItem& event) const
	{
		return m_code == event.errorCode;
	}
};

// Retrieve event by error code
const EventItem& FetchEvent(int code)
{
	MatchEvent matcher{ code };
	if (code < _EV_MIN_ITEM_ID || code > _EV_MAX_ITEM_ID) {
		throw 1;
	}
	const auto result = std::find_if(g_eventItemList.cbegin(), g_eventItemList.cend(), matcher);
	if (result == g_eventItemList.cend()) {
		throw 1;
	}
	return (*result);
}

} // namespace Detail

namespace EventLog
{

struct FatalException : public std::exception
{
	std::string tmpStorage;
	const EventItem& m_eventLink;

public:
	FatalException(int code)
		: m_eventLink{ Detail::FetchEvent(code) }
	{
		tmpStorage = m_eventLink.InfoLine();
	}

	explicit FatalException(AliasErrorCode code)
		: m_eventLink{ Detail::FetchEvent(static_cast<int>(code)) }
	{
		tmpStorage = m_eventLink.InfoLine();
	}

	inline const EventItem Event() const noexcept
	{
		return m_eventLink;
	}

	virtual char const *what() const
	{
		return tmpStorage.c_str();
	}
};

template<typename... ArgsType>
FatalException MakeException(ArgsType&&... args)
{
	return FatalException{ std::forward<ArgsType>(args)... };
}

// Get the number of registered events in the static array.
inline size_t EventItemCount() noexcept { return 3; }
// Check if fatal exception has description.
inline bool HasDescription(const FatalException& ev) { return !ev.Event().desc.empty(); }
// Get the event message info line from an fatal exception.
inline std::string GetInfoLine(const FatalException& ev) { return ev.Event().InfoLine(); }
// Get the title from an fatal exception.
inline std::string GetTitle(const FatalException& ev) { return ev.Event().title; }
// Get the description from an fatal exception, if any.
inline std::string GetDescription(const FatalException& ev) { return ev.Event().desc; }

} // namespace EventLog

