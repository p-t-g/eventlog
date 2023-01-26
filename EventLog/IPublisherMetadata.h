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

#include <string>
#include <optional>
#include <cstdint>
#include <vector>

#include "CommonTypes.h"
#include "RefObject.h"
#include "RefPtr.h"
#include "IEventMetadataEnumerator.h"

namespace Windows::EventLog
{
class PublisherChannelInfo
{
	std::string mReferencePath{};
	uint32_t mReferenceIndex{};
	uint32_t mReferenceID{};
	uint32_t mReferenceFlags{};
	uint32_t mMessageID{};
	std::string mMessage{};
public:
	PublisherChannelInfo() = default;
	PublisherChannelInfo(const PublisherChannelInfo  &) = default;
	PublisherChannelInfo(PublisherChannelInfo &&) = default;
	~PublisherChannelInfo() = default;
	PublisherChannelInfo &operator=(const PublisherChannelInfo &) = default;
	PublisherChannelInfo &operator=(PublisherChannelInfo &&) = default;

	PublisherChannelInfo(const std::string &referencePath, uint32_t referenceIndex, 
		uint32_t referenceID, uint32_t referenceFlags, uint32_t messageID, const std::string &message)
		: mReferencePath{referencePath}, mReferenceIndex{referenceIndex}
		, mReferenceID{referenceID}, mReferenceFlags{referenceFlags}
		, mMessageID{messageID}, mMessage{message}
	{}

	std::string getChannelReferencePath() const { return mReferencePath; }
	uint32_t getChannelReferenceIndex() const { return mReferenceIndex; }
	uint32_t getChannelReferenceID() const { return mReferenceID; }
	uint32_t getChannelReferenceFlags() const { return mReferenceFlags; } 
	uint32_t getMessageID() const { return mMessageID; }
	std::string getMessage() const { return mMessage; }
};

// EvtPublisherMetadataChannelReferences
// Names are all different for this versus the others 
class IPublisherChannelArray : public IRefObject
{
public:

	virtual ~IPublisherChannelArray() = default;

	virtual uint32_t getSize() const = 0;

	// EvtPublisherMetadataChannelReferencePath
	// Identifies the name attribute of the channel.
	virtual std::string getChannelReferencePath(uint32_t index) const = 0;
	// ?? virtual std::string lookupChannelReferencePath(UINT32 referenceID) const = 0;

	// EvtPublisherMetadataChannelReferenceIndex
	// Identifies the zero-based index value of the channel in the list of channels
	// I have no idea what this means.
	virtual uint32_t getChannelReferenceIndex(uint32_t index) const = 0;

	// EvtPublisherMetadataChannelReferenceID
	// Identifies the value attribute of the channel.
	// This is like Evt***Value for the others. They didn't use consistent naming.
	virtual uint32_t getChannelReferenceID(uint32_t index) const = 0;

	// EvtPublisherMetadataChannelReferenceFlags
	// Identifies the flags value that indicates whether this channel is imported
	// from another provider.
	virtual uint32_t getChannelReferenceFlags(uint32_t index) const = 0;

	// EvtPublisherMetadataChannelReferenceMessageID
	// Identifies the message attribute of the channel.
	// The property contains the resource identifier that is assigned to the message string. 
	virtual uint32_t getMessageId(uint32_t index) const = 0;

	// Return the message string, if any, corresponding to MessageID. If there is no 
	// message, returns an empty string.
	virtual std::string getMessage(uint32_t index) const = 0;

	// Retrieves all the values at index. 
	// Note that this doesn't result in fewer calls to the underlying Windows API. 
	// Provided for convenience.
	virtual PublisherChannelInfo getChannelInfo(uint32_t index) const = 0;

	// Find the index of the entry with the corresponding referenceID.
	// Typically the referenceID comes from event metadata. 
	// Returns DWORD(-1) if not found.
	virtual uint32_t findIndex(uint32_t referenceID) const = 0;

};

class PublisherLevelInfo
{
	std::string mName{};
	uint32_t mValue{};
	uint32_t mMessageID{};
	std::string mMessage{};
public:
	PublisherLevelInfo() = default;
	PublisherLevelInfo(const PublisherLevelInfo &) = default;
	PublisherLevelInfo(PublisherLevelInfo &&) = default;
	PublisherLevelInfo &operator=(const PublisherLevelInfo &) = default;
	PublisherLevelInfo &operator=(PublisherLevelInfo &&) = default;
	~PublisherLevelInfo() = default;

	PublisherLevelInfo(const std::string &name, uint32_t value, uint32_t messageID,
		const std::string &message)
		: mName{name}, mValue{value}, mMessageID{messageID}, mMessage{message}
	{}

	std::string getName() const { return mName; }
	uint32_t getValue() const { return mValue; }
	uint32_t getMessageID() const { return mMessageID; }
	std::string getMessage() const { return mMessage; }

};

// EvtPublisherMetadataLevel*
class IPublisherLevelArray : public IRefObject
{
public:
	virtual ~IPublisherLevelArray() = default;

	virtual uint32_t getSize() const = 0;

	// EvtPublisherMetadataLevelName
	virtual std::string getName(uint32_t index) const = 0;

	// EvtPublisherMetadataLevelValue
	virtual uint32_t getValue(uint32_t index) const = 0;

	// EvtPublisherMetadataLevelMessageID
	virtual uint32_t getMessageId(uint32_t index) const = 0;

	// Returns the message associated with the message ID or empty string if none.
	virtual std::string getMessage(uint32_t index) const = 0;

	virtual PublisherLevelInfo getLevelInfo(uint32_t index) const = 0;

	// Find the index of the element with the given value.
	virtual uint32_t findIndex(uint32_t value) const = 0;

	// Returns the localized message associated with the level entry for the 
	// given value key. If no message is present, returns the name of the value
	// as per getName(index)
	virtual std::string getDisplay(uint32_t value) const = 0;
};

class PublisherTaskInfo
{
	std::string mName{};
	GUID mEventGuid{};
	uint32_t mValue{};
	uint32_t mMessageID{};
	std::string mMessage{};
public:
	PublisherTaskInfo() = default;
	PublisherTaskInfo(const PublisherTaskInfo &) = default;
	PublisherTaskInfo(PublisherTaskInfo &&) = default;
	PublisherTaskInfo &operator=(const PublisherTaskInfo &) = default;
	PublisherTaskInfo &operator=(PublisherTaskInfo &&) = default;
	~PublisherTaskInfo() = default;

	PublisherTaskInfo(const std::string &name, const GUID &eventGuid, uint32_t value,
		uint32_t messageId, const std::string &message)
		: mName{ name }, mEventGuid{ eventGuid }, mValue{ value }, mMessageID{ messageId },
		mMessage{ message }
	{}

	std::string getName() const { return mName; }
	GUID getEventGuid() const { return mEventGuid; }
	uint32_t getValue() const { return mValue; }
	uint32_t getMessageID() const { return mMessageID; }
	std::string getMessage() const { return mMessage; }
};

// EvtPublisherMetadataTask*
class IPublisherTaskArray : public IRefObject 
{
public:
	virtual ~IPublisherTaskArray() = default;

	virtual uint32_t getSize() const = 0;

	// EvtPublisherMetadataTaskName
	virtual std::string getName(uint32_t index) const = 0;

	// EvtPublisherMetadataTaskEventGuid
	virtual GUID getEventGuid(uint32_t index) const = 0;

	// EvtPublisherMetadataTaskValue
	virtual uint32_t getValue(uint32_t index) const = 0;

	// EvtPublisherMetadataTaskMessageID
	virtual uint32_t getMessageId(uint32_t index) const = 0;

	virtual std::string getMessage(uint32_t index) const = 0;

	virtual PublisherTaskInfo getTaskInfo(uint32_t index) const = 0;

	virtual uint32_t findIndex(uint32_t value) const = 0;

	virtual std::string getDisplay(uint32_t value) const = 0;
};

class PublisherOpcodeInfo
{
	std::string mName{};
	uint32_t mValue{};
	uint32_t mMessageID{};
	std::string mMessage{};
public:
	PublisherOpcodeInfo() = default;
	PublisherOpcodeInfo(const PublisherOpcodeInfo &) = default;
	PublisherOpcodeInfo(PublisherOpcodeInfo &&) = default;
	PublisherOpcodeInfo &operator=(const PublisherOpcodeInfo &) = default;
	PublisherOpcodeInfo &operator=(PublisherOpcodeInfo &&) = default;
	~PublisherOpcodeInfo() = default;

	PublisherOpcodeInfo(const std::string &name, uint32_t value, uint32_t messageID,
		const std::string &message)
		: mName{ name }, mValue{ value }, mMessageID{ messageID }, mMessage{ message }
	{}

	std::string getName() const { return mName; }
	uint32_t getValue() const { return mValue; }
	uint32_t getMessageID() const { return mMessageID; }
	std::string getMessage() const { return mMessage; }

};

class IPublisherOpcodeArray : public IRefObject
{
public:
	virtual ~IPublisherOpcodeArray() = default;

	virtual uint32_t getSize() const = 0;

	// EvtPublisherMetadataOpcodeName
	virtual std::string getName(uint32_t index) const = 0;

	// EvtPublisherMetadataOpcodeValue
	virtual uint32_t getValue(uint32_t index) const = 0;

	// EvtPublisherMetadataOpcodeMessageID
	virtual uint32_t getMessageId(uint32_t index) const = 0;

	virtual std::string getMessage(uint32_t index) const = 0;

	virtual PublisherOpcodeInfo getOpcodeInfo(uint32_t index) const = 0;

	// Opcodes are weird. It's necessary to glue the opcode and task
	// togeter with the opcode in the hiword and task in the lo word
	// Why? Because opcodes can be per task or global and this is how
	// MSFT decided to store them. Global/no task opcodes have loword
	// set to 0 (zero).
	virtual uint32_t findIndex(uint32_t opHiWord_TaskLoWord) const = 0;

	virtual std::string getDisplay(uint32_t opHiWord_TaskLoWord) const = 0;
};

class PublisherKeywordInfo
{
	std::string mName;
	uint64_t mValue;
	uint32_t mMessageId;
	std::string mMessage;
public:
	PublisherKeywordInfo() = default;
	PublisherKeywordInfo(const PublisherKeywordInfo &) = default;
	PublisherKeywordInfo(PublisherKeywordInfo &&) = default;
	PublisherKeywordInfo &operator=(const PublisherKeywordInfo &) = default;
	PublisherKeywordInfo &operator=(PublisherKeywordInfo &&) = default;
	~PublisherKeywordInfo() = default;

	PublisherKeywordInfo(const std::string name, uint64_t value, uint32_t messageID, const std::string &message)
		: mName(name), mValue(value), mMessageId(messageID), mMessage(message)
	{}

	std::string getName() const { return mName; }
	uint64_t getValue() const { return mValue; }
	uint32_t getMessageId() const { return mMessageId; }
	std::string getMessage() const { return mMessage; }
};

class IPublisherKeywordArray : public IRefObject
{
public:
	virtual ~IPublisherKeywordArray() = default;

	virtual uint32_t getSize() const = 0;

	// EvtPublisherMetadataKeywordName
	virtual std::string getName(uint32_t index) const = 0;

	// EvtPublisherMetadataKeywordValue
	virtual uint64_t getValue(uint32_t index) const = 0;

	// EvtPublisherMetadataKeywordMessageID
	virtual uint32_t getMessageId(uint32_t index) const = 0; 

	virtual std::string getMessage(uint32_t index) const = 0;

	virtual PublisherKeywordInfo getKeywordInfo(uint32_t index) const = 0;

	// Keywords use bits. 
	virtual std::vector<std::string> getDisplay(uint64_t value) const = 0;
};

class IPublisherMetadata : public IRefObject
{
public:

	// Can return RefPtr(nullptr) if the provider metadata fails to load.
	// It's handled this way because the intent of the cache is to be used
	// when rendering events for display, and a failure to load metadata 
	// does not imply a failure to render the event. 
	static RefPtr<IPublisherMetadata> cacheOpenProvider(const std::string &provider);

	virtual ~IPublisherMetadata() = default;

	virtual std::optional<GUID> getPublisherGuid() const = 0;

	// EvtPublisherMetadataResourceFilePath
	virtual std::optional<std::string> getResourceFilePath() const = 0;

	// EvtPublisherMetadataParameterFilePath
	virtual std::optional<std::string> getParametersFilePath() const = 0;

	// EvtPublisherMetadataMessageFilePath
	virtual std::optional<std::string> getMessageFilePath() const = 0;

	// EvtPublisherMetadataHelpLink
	virtual std::optional<std::string> getHelpLink() const = 0;

	// EvtPublisherMetadataPublisherMessageID
	virtual std::optional<uint32_t> getPublisherMessageId() const = 0;

	virtual std::string getPublisherMessage() const = 0;

	// EvtPublisherMetadataChannelReferences
	virtual Ref<IPublisherChannelArray> getChannels() const = 0;

	// EvtPublisherMetadataLevels
	virtual Ref<IPublisherLevelArray> getLevels() const = 0;

	// EvtPublisherMetadataTasks
	virtual Ref<IPublisherTaskArray> getTasks() const = 0;

	// EvtPublisherMetadataOpcodes
	virtual Ref<IPublisherOpcodeArray> getOpcodes() const = 0;

	// EvtPublisherMetadataKeywords
	virtual Ref<IPublisherKeywordArray> getKeywords() const = 0;

	virtual Ref<IEventMetadataEnumerator> openEventMetadataEnum() const = 0;

	// Retrieves a message given the message ID. Typically the *MessageID value comes 
	// from one of the object property arrays. Unless you're doing something low-level
	// this probably isn't much use. It's really only included for completeness. 
	virtual std::string formatMessage(uint32_t messageID) const = 0;

	// Crap. This thing is important but it exposes too much.
	// virtual EventFormatRecord format(const EventRecordHandle &h) const = 0;

	// For events to lookup channel display from value (which is basically a key)
	virtual std::string lookupChannelDisplay(uint32_t channelValue) const = 0;

	virtual std::string lookupLevelDisplay(uint32_t levelValue) const = 0;

	virtual std::string lookupTaskDisplay(uint32_t taskValue) const = 0;

	virtual std::string lookupOpcodesDisplay(uint32_t op_task) const = 0;

	virtual std::vector<std::string> lookupKeywordsDisplay(uint64_t maskBits) const = 0;

};

}