/*
	Copyright (C) 2022-2023 Patrick Griffiths

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	claim that you wrote the original software. If you use this software
	in a product, an acknowledgment in the product documentation would be
	appreciated but is not required.

	2. Altered source versions must be plainly marked as such, and must not be
	misrepresented as being the original software.

	3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Would be nice to avoid this but it's not big like windows.h
#include <guiddef.h>

namespace Windows
{

// The Windows Event Log API uses an unsigned 64 bit int so this just wraps 
// it in a type to give it some meaning. Could have used FILETIME, but 
// don't want to drag in windows.h
struct Timestamp
{
	// 100 nanos since January 1 1601. 
	uint64_t timestamp;
};

std::string to_string(const Timestamp &);

namespace EventLog
{

enum class ChannelIsolation : uint32_t
{
	Application = 0,
	System,
	Custom
};

std::string to_string(ChannelIsolation);

enum class ChannelType : uint32_t
{
	Admin = 0,
	Operational,
	Analytic,
	Debug
};

std::string to_string(ChannelType);

enum class ChannelClockType : uint32_t
{
	SystemTime,
	QPC
};

std::string to_string(ChannelClockType);

enum class ChannelSIDType : uint32_t 
{
	None,
	Publishing
};

std::string to_string(ChannelSIDType);

enum class Direction
{
	Forward = 1,
	Reverse = 2
};

enum class SeekOption : uint32_t
{
	RelativeToFirst,
	RelativeToLast,
	RelativeToCurrent
};

enum class QueryNextStatus : int
{
	Success = 1,
	NoMoreItems,
	Timeout
};

} // namespace EventLog

std::string to_string(GUID g);

} // namespace Windows