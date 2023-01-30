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

#include "CommonTypes.h"
#include "RefObject.h"

#include <optional>

namespace Windows::EventLog
{

class IEventRecord : public IRefObject
{
public:
	// Empty record to avoid RefPtr(nullptr)
	static Ref<IEventRecord> createEmpty();

	virtual ~IEventRecord() = default;

	virtual std::optional<std::string> getProviderName() const = 0;
	virtual std::optional<GUID> getProviderGuid() const = 0;
	virtual std::optional<uint16_t> getEventId() const = 0;
	virtual std::optional<uint16_t> getQualifers() const = 0;
	virtual std::optional<uint8_t> getLevel() const = 0;
	virtual std::optional<uint16_t> getTask() const = 0;
	virtual std::optional<uint8_t> getOpcode() const = 0;
	virtual std::optional<int64_t> getKeywords() const = 0;
	virtual std::optional<Timestamp> getTimeCreated() const = 0;
	virtual std::optional<uint64_t> getRecordId() const = 0;
	virtual std::optional<GUID> getActivityId() const = 0;
	virtual std::optional<uint32_t> getProcessId() const = 0;
	virtual std::optional<uint32_t> getThreadId() const = 0;
	virtual std::optional<std::string> getChannel() const = 0;
	virtual std::optional<std::string> getComputer() const = 0;
	virtual std::optional<std::string> getUser() const = 0; 
	virtual std::optional<uint8_t> getVersion() const = 0;

	virtual std::string getMessage() const = 0; 
	virtual std::string getLevelDisplay() const = 0;
	virtual std::string getTaskDisplay() const = 0;
	virtual std::string getOpcodeDisplay() const = 0;
	virtual std::vector<std::string> getKeywordsDisplay() const = 0;
	virtual std::string getChannelMessage() const = 0;
	virtual std::string getProviderMessage() const = 0;
};

}