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

#include "IEventRecord.h"

namespace Windows::EventLog
{

struct FormattedEventRecord
{
	std::string message;
	std::string level;
	std::string task;
	std::string opcode;
	std::vector<std::string> keywords;
	std::string channelMessage;
	std::string providerMessage;
};

class EventRecordHandle;
class EventRecord : public IEventRecord
{
public:	
	friend class RefObject<EventRecord>;

	static Ref<EventRecord> create(const EventRecordHandle &hRecord);

	~EventRecord() = default;

	std::optional<std::string> getProviderName() const override;
	std::optional<GUID> getProviderGuid() const override;
	std::optional<uint16_t> getEventId() const override;
	std::optional<uint16_t> getQualifers() const override;
	std::optional<uint8_t> getLevel() const override;
	std::optional<uint16_t> getTask() const override;
	std::optional<uint8_t> getOpcode() const override;
	std::optional<int64_t> getKeywords() const override;
	std::optional<Timestamp> getTimeCreated() const override;
	std::optional<uint64_t> getRecordId() const override;
	std::optional<GUID> getActivityId() const override;
	std::optional<uint32_t> getProcessId() const override;
	std::optional<uint32_t> getThreadId() const override;
	std::optional<std::string> getChannel() const override;
	std::optional<std::string> getComputer() const override;
	std::optional<std::string> getUser() const override;
	std::optional<uint8_t> getVersion() const override;
	std::string getMessage() const override; // this is the event message
	std::string getLevelDisplay() const override;
	std::string getTaskDisplay() const override;
	std::string getOpcodeDisplay() const override;
	std::vector<std::string> getKeywordsDisplay() const override;
	std::string getChannelMessage() const override;
	std::string getProviderMessage() const override;

private:
	explicit EventRecord(const EventRecordHandle &hRecord);

private:
	std::optional<std::string> mProviderName{};
	std::optional<GUID> mProviderGuid{};
	std::optional<uint16_t> mEventId{};
	std::optional<uint16_t> mQualifers{};
	std::optional<uint8_t> mLevel{};
	std::optional<uint16_t> mTask{};
	std::optional<uint8_t> mOpcode{};
	std::optional<int64_t> mKeywords{};
	std::optional<Timestamp> mTimeCreated{};
	std::optional<uint64_t> mRecordId{};
	std::optional<GUID> mActivityId{};
	std::optional<GUID> mRelatedActivityId{};
	std::optional<uint32_t> mProcessId{};
	std::optional<uint32_t> mThreadId{};
	std::optional<std::string> mChannel{};
	std::optional<std::string> mComputer{};
	std::optional<std::string> mUser{};
	std::optional<uint8_t> mVersion{};

	FormattedEventRecord mRecord{};

private:
	EventRecord(const EventRecord &) = delete;
	EventRecord &operator=(const EventRecord &) = delete;
};


}