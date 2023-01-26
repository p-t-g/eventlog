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

#include "PublisherMetadata.h"

#include "EvtHandle.h"
#include "EvtVariant.h"
#include "Exception.h"
#include "PublisherMetadataImpl.h"
#include "StringUtils.h"

#include <unordered_map>

namespace Windows::EventLog
{
//
// Format Message Helpers
//

static bool isIgnoredFormatMessageError(DWORD err)
{
	static constexpr DWORD ignored[] =
	{
		// Miconfig or unsupported language
		ERROR_EVT_MESSAGE_NOT_FOUND,
		ERROR_EVT_MESSAGE_ID_NOT_FOUND,
		ERROR_EVT_MESSAGE_LOCALE_NOT_FOUND,
		// Following happen with retrieving raw messages (no inserts).
		ERROR_EVT_UNRESOLVED_PARAMETER_INSERT,
		ERROR_EVT_UNRESOLVED_VALUE_INSERT,
		ERROR_EVT_MAX_INSERTS_REACHED,
		// Misconfigured stuff we can't do anything about.
		ERROR_MUI_FILE_NOT_FOUND,
		ERROR_MUI_FILE_NOT_LOADED
	};

	for (auto e : ignored)
	{
		if (err == e)
			return true;
	}
	return false;
}

static std::unique_ptr<wchar_t[]> formatMessage(EVT_HANDLE hP, EVT_HANDLE hE, uint32_t flags, uint32_t *actualSize)
{
	uint32_t size = 64;
	std::unique_ptr<wchar_t []> msg = std::make_unique<wchar_t[]>(size);
	BOOL success = EvtFormatMessage(hP, hE, 0, 0, nullptr, flags, size, msg.get(), (PDWORD) &size);
	if (!success)
	{
		uint32_t err = ::GetLastError();
		if (err == ERROR_INSUFFICIENT_BUFFER)
		{
			msg = std::make_unique<wchar_t[]>(size);
			success = EvtFormatMessage(hP, hE, 0, 0, nullptr, flags, size, msg.get(), (PDWORD) &size);
			if (!success)
			{
				err = ::GetLastError();
				THROW_(SystemError, err);
			}
		}
		else
		{
			if (isIgnoredFormatMessageError(err))
			{
				size = 0;
				msg = nullptr;
			}
			else
			{ 
				THROW_(SystemError, err);
			}
		}
	}
	*actualSize = size;
	return msg;
}

static
std::vector<std::string> formatKeyword(const EventRecordHandle &recordHandle)
{
	std::vector<std::string> result;{}

	uint32_t size = 0;
	std::unique_ptr<wchar_t[]> msg = formatMessage(nullptr, recordHandle, EvtFormatMessageKeyword, &size);

	if (size > 0 && msg)
	{
		uint32_t index = 0;
		uint32_t currentLen = uint32_t(wcslen(&msg[index]));
		result.emplace_back(to_utf8(&msg[index], currentLen));

		index = currentLen + 1;

		while (index < size)
		{
			// Sometimes double null termination, sometimes not. 
			if (msg[index] == 0)
				break;

			currentLen = uint32_t(wcslen(&msg[index]));
			result.emplace_back(to_utf8(&msg[index], currentLen));

			index += currentLen + 1;
		}
	}

	return result;
}

static
std::vector<std::string> formatKeyword(const PublisherMetadataHandle &publisherMetadataHandle,
	const EventRecordHandle &recordHandle)
{
	std::vector<std::string> result;{}

	uint32_t size = 0;
	std::unique_ptr<wchar_t[]> msg = formatMessage(publisherMetadataHandle, recordHandle, EvtFormatMessageKeyword, &size);

	if (size > 0 && msg)
	{
		uint32_t index = 0;
		uint32_t currentLen = uint32_t(wcslen(&msg[index]));
		result.emplace_back(to_utf8(&msg[index], currentLen));

		index = currentLen + 1;

		while (index < size)
		{
			// Sometimes double null termination, sometimes not. 
			if (msg[index] == 0)
				break;

			currentLen = uint32_t(wcslen(&msg[index]));
			result.emplace_back(to_utf8(&msg[index], currentLen));

			index += currentLen + 1;
		}
	}

	return result;
}

static 
std::string formatMessage(const PublisherMetadataHandle &hMetadata, uint32_t messageID)
{
	uint32_t size = 256;
	std::unique_ptr<wchar_t[]> buffer(std::make_unique<wchar_t[]>(size));
	std::string msg;

	SysErr err = hMetadata.formatMessage(messageID, size, buffer.get(), &size);
	if (!err)
	{
		msg = to_utf8(buffer.get());
		// TODO: msg = to_utf8(buffer.get(), size_t(size) - 1u);
	}
	else
	{
		if (err == ERROR_INSUFFICIENT_BUFFER)
		{
			buffer = std::make_unique<wchar_t[]>(size);
			err = hMetadata.formatMessage(messageID, size, buffer.get(), &size);
			if (err == ERROR_SUCCESS)
			{
				msg = to_utf8(buffer.get());
				// TODO: msg = to_utf8(buffer.get(), size_t(size) - 1u);
			}
			else
			{
				if (!isIgnoredFormatMessageError(err.getCode()))
				{
					THROW_(SystemError, err);
				}
			}
		}
		else
		{ 
			if (!isIgnoredFormatMessageError(err.getCode()))
			{
				THROW_(SystemError, err);
			}
		}
	}

	return msg;
}

static
std::string formatMessage(const EventRecordHandle &recordHandle, uint32_t flags)
{
	uint32_t size = 0;
	auto buf = formatMessage(nullptr, recordHandle, flags, &size);
	if (buf && size > 0)
	{
		size_t currentLen = wcslen(&buf[0]);
		return to_utf8(&buf[0], currentLen);
	}
	return {};
}

static
std::string formatMessage(const PublisherMetadataHandle &publisherMetadataHandle, 
	const EventRecordHandle &recordHandle, uint32_t flags)
{
	uint32_t size = 0;
	auto buf = formatMessage(publisherMetadataHandle,
		recordHandle, flags, &size);
	if (buf && size > 0)
	{
		size_t currentLen = wcslen(&buf[0]);
		return to_utf8(&buf[0], currentLen);
	}
	return {};
}

template<typename T, typename V>
static uint32_t findIndexImpl(const T *p, V value)
{
	uint32_t index = uint32_t(-1);
	// zero is 'no value' 
	if (value == 0)
		return index;
	uint32_t count = p->getSize();
	for (uint32_t i = 0; i < count; ++i)
	{
		if (p->getValue(i) == value)
		{
			index = i;
			break;
		}
	}
	return index;
}

static 
std::optional<uint32_t> getEventMetadataPropertyUInt32(const EventMetadataHandle &h,
	EVT_EVENT_METADATA_PROPERTY_ID propertyId)
{
	EVT_VARIANT v{};
	h.getProperty(propertyId, v);
	if (!Variant::isNull(v))
		return Variant::getUInt32(v);
	return {};
}

static 
std::optional<uint64_t> getEventMetadataPropertyUInt64(const EventMetadataHandle &h, 
	EVT_EVENT_METADATA_PROPERTY_ID propertyId)
{
	EVT_VARIANT v{};
	h.getProperty(propertyId, v);
	if (!Variant::isNull(v))
		return Variant::getUInt64(v);
	return {};
}

static
std::optional<std::string> getEventMetadataPropertyString(const EventMetadataHandle &h, 
	EVT_EVENT_METADATA_PROPERTY_ID propertyId)
{
	EvtVariantPtr pv = h.getProperty(propertyId);
	if (!Variant::isNull(*pv))
		return Variant::getString(*pv);
	return {};
}

//
// EventMetadata.
//

class EventMetadata : public IEventMetadata
{
public:
	// So this is interesting :
	// - The example code says the top byte is reserved.
	// - The docs say the top word (2 bytes) is reserved.
	// - The open source .NET wrapper just uses all the bits.
	// So there's no clear story about this. I'm going to go
	// with the example code as it runs well and the output seems 
	// to make sense. 
	// TODO: move this.
	static constexpr uint64_t KEYWORD_RESERVED_BITS_MASK = 0x00FFFFFFFFFFFFFF;

	friend class RefObject<EventMetadata>;

	static RefPtr<IEventMetadata> create(EventMetadataHandle hEventMetadata, const IPublisherMetadata *publisher);

	~EventMetadata();

	std::optional<uint32_t> getId() const override;
	std::optional<uint32_t> getVersion() const override;

	// Zero value means not specified, but we also allow null value as it seems 
	// to be possible
	std::optional<uint32_t> getChannel() const override;
	std::optional<uint32_t> getLevel() const override;
	std::optional<uint32_t> getOpcode() const override;
	std::optional<uint32_t> getTask() const override;
	std::optional<uint64_t> getKeyword() const override;
	std::optional<uint32_t> getMessageID() const override;

	std::optional<std::string> getTemplate() const override;

	std::string getChannelDisplay() const override;
	std::string getLevelDisplay() const override;
	std::string getOpcodeDisplay() const override;
	std::string getTaskDisplay() const override;
	std::vector<std::string> getKeywordsDisplay() const override;
	std::string getMessageDisplay() const override;

private:

	// Identifies the value attribute of the event definition. The variant type for
	// this property is EvtVarTypeUInt32.
	std::optional<uint32_t> mId{};

	// Identifies the version attribute of the event definition. The variant type
	// for this property is EvtVarTypeUInt32.
	std::optional<uint32_t> mVersion{};

	// Identifies the channel attribute of the event definition. The variant type 
	// for this property is EvtVarTypeUInt32. This property does not contain the 
	// channel identifier that you specified in the event definition but instead 
	// contains the value attribute of the channel. The value is zero if the event 
	// definition does not specify a channel.
	std::optional<uint32_t> mChannel{};

	// Identifies the level attribute of the event definition. The variant type 
	// for this property is EvtVarTypeUInt32. This property does not contain the 
	// level name that you specified in the event definition but instead contains 
	// the value attribute of the level. The value is zero if the event definition
	// does not specify a level.
	std::optional<uint32_t> mLevel{};

	// Identifies the opcode attribute of the event definition. The variant type
	// for this property is EvtVarTypeUInt32. This property does not contain the
	// opcode name that you specified in the event definition but instead contains 
	// the value attribute of the opcode. The value is zero if the event definition 
	// does not specify an opcode.
	std::optional<uint32_t> mOpcode{};

	// Identifies the keyword attribute of the event definition. The variant type 
	// for this property is EvtVarTypeUInt64. This property does not contain the 
	// list of keyword names that you specified in the event definition but 
	// instead contains a 64-bitmask of all the keywords. The top 16 bits of the 
	// mask are reserved for internal use and should be ignored when determining 
	// the keyword bits that the event definition set.
	std::optional<uint32_t> mTask{};

	// Identifies the keyword attribute of the event definition. The variant type
	// for this property is EvtVarTypeUInt64. This property does not contain the 
	// list of keyword names that you specified in the event definition but 
	// instead contains a 64-bitmask of all the keywords. The top 16 bits of the 
	// mask are reserved for internal use and should be ignored when determining
	// the keyword bits that the event definition set.
	std::optional<uint64_t> mKeyword{};

	// Identifies the message attribute of the event definition. The variant type
	// for this property is EvtVarTypeUInt32. The property contains the resource 
	// identifier that is assigned to the message string. To get the message 
	// string, call the EvtFormatMessage function. If the event definition does 
	// not specify a message, the value is –1.
	std::optional<uint32_t> mMessageId{};

	// Identifies the template attribute of the event definition. The variant 
	// type for this property is EvtVarTypeString. This property does not contain
	// the template name that you specified in the event definition but instead 
	// contains an XML string that includes the template node and each data node; 
	// the string does not include the UserData. The value is an empty string if
	// the event definition does not specify a template.
	std::optional<std::string> mTmpl{};

	// Channel display string
	std::string mChannelDisplay{};

	// Level display string
	std::string mLevelDisplay{};

	// Opcode display string
	std::string mOpcodeDisplay{};

	// Task display string
	std::string mTaskDisplay{};

	// Keyword display strings
	std::vector<std::string> mKeywordsDisplay{};

	// Message display string. 
	std::string mMessageDisplay{};

private:
	explicit EventMetadata(EventMetadataHandle h, const IPublisherMetadata *publisher);

	EventMetadata(const EventMetadata &) = delete;
	EventMetadata(EventMetadata &&) = delete;
	EventMetadata &operator=(const EventMetadata &) = delete;
	EventMetadata &operator=(EventMetadata &&) = delete;
};

//
// EventMedata implementation
//
EventMetadata::EventMetadata(EventMetadataHandle hEventMetadata, const IPublisherMetadata *publisher)
	: mId(getEventMetadataPropertyUInt32(hEventMetadata, EventMetadataEventID))
	, mVersion(getEventMetadataPropertyUInt32(hEventMetadata, EventMetadataEventVersion))
	, mChannel(getEventMetadataPropertyUInt32(hEventMetadata, EventMetadataEventChannel))
	, mLevel(getEventMetadataPropertyUInt32(hEventMetadata, EventMetadataEventLevel))
	, mOpcode(getEventMetadataPropertyUInt32(hEventMetadata, EventMetadataEventOpcode))
	, mTask(getEventMetadataPropertyUInt32(hEventMetadata, EventMetadataEventTask))
	, mKeyword(getEventMetadataPropertyUInt64(hEventMetadata, EventMetadataEventKeyword))
	, mMessageId(getEventMetadataPropertyUInt32(hEventMetadata, EventMetadataEventMessageID))
	, mTmpl(getEventMetadataPropertyString(hEventMetadata, EventMetadataEventTemplate))
{
	if (mKeyword.has_value())
		mKeyword.value() = mKeyword.value() & 0x00ffffffffffffffull;
	
	// Get human readable 

	if (mChannel.has_value())
	{
		auto value = mChannel.value();
		if (value > 0) // zero is no value 
		{ 
			mChannelDisplay = publisher->lookupChannelDisplay(value);			
		}
	}

	if (mLevel.has_value())
	{
		auto value = mLevel.value();
		if (value > 0) // zero is no value 
		{
			mLevelDisplay = publisher->lookupLevelDisplay(value);
		}
	}

	if (mOpcode.has_value())
	{
		// MSFT handles tasks specific opcodes by putting the opcode in 
		// the hiword and task in the loword. Global opcodes has task of 0. 
		// Follow the lookup code to see how it works. 
		auto value = mOpcode.value();
		if (value > 0)
		{
			uint32_t opcodeTask = mOpcode.value() << 16; 
			if (mTask.has_value())
			{ 
				// Doesn't matter if it's zero or not.
				opcodeTask |= mTask.value();
			}
			mOpcodeDisplay = publisher->lookupOpcodesDisplay(opcodeTask);
		}
	}

	if (mTask.has_value())
	{
		auto value = mTask.value();
		if (value > 0)
		{			
			mTaskDisplay = publisher->lookupTaskDisplay(value);
		}
	}

	if (mKeyword.has_value())
	{
		auto value = mKeyword.value();

		value &= KEYWORD_RESERVED_BITS_MASK;
		if (value > 0)
		{ 
			mKeywordsDisplay = publisher->lookupKeywordsDisplay(value);
		}
	}

	if (mMessageId.has_value())
	{
		auto messageId = mMessageId.value();
		if (messageId != uint32_t(-1))
		{ 
			mMessageDisplay = publisher->formatMessage(messageId);
		}
	}
}

EventMetadata::~EventMetadata()
{}

RefPtr<IEventMetadata> EventMetadata::create(EventMetadataHandle hEventMetadata, const IPublisherMetadata *publisher)
{
	return RefObject<EventMetadata>::create(std::move(hEventMetadata), publisher);
}

std::optional<uint32_t> EventMetadata::getId() const
{
	return mId;
}

std::optional<uint32_t> EventMetadata::getVersion() const 
{
	return mVersion;
}

std::optional<uint32_t> EventMetadata::getChannel() const
{
	return mChannel;
}	

std::optional<uint32_t> EventMetadata::getLevel() const
{
	return mLevel;
}

std::optional<uint32_t> EventMetadata::getOpcode() const
{
	return mOpcode;
}

std::optional<uint32_t> EventMetadata::getTask() const
{
	return mTask;
}

std::optional<uint64_t> EventMetadata::getKeyword() const
{
	return mKeyword;
}

std::optional<uint32_t> EventMetadata::getMessageID() const
{
	return mMessageId;
}

std::optional<std::string> EventMetadata::getTemplate() const
{
	return mTmpl;
}

std::string EventMetadata::getChannelDisplay() const
{
	return mChannelDisplay;
}

std::string EventMetadata::getLevelDisplay() const
{
	return mLevelDisplay;
}

std::string EventMetadata::getOpcodeDisplay() const
{
	return mOpcodeDisplay;
}

std::string EventMetadata::getTaskDisplay() const
{
	return mTaskDisplay;
}

std::vector<std::string> EventMetadata::getKeywordsDisplay() const
{
	return mKeywordsDisplay;
}

std::string EventMetadata::getMessageDisplay() const
{
	return mMessageDisplay;
}

//
// EventMetadataEnum
//

class EventMetadataEnum : public IEventMetadataEnumerator
{
	EventMetadataEnumHandle mEventMetadataEnumHandle;
	RefPtr<const IPublisherMetadata> mPublisherMeta;
	// This is NOT what we want.
	mutable EventMetadataHandle mCurrent;
	// This is:
	RefPtr<IEventMetadata> mCurrentItem;
public:
	friend class RefObject<EventMetadataEnum>;

	static Ref<IEventMetadataEnumerator> create(EventMetadataEnumHandle h, RefPtr<const IPublisherMetadata> publisherMeta);

	~EventMetadataEnum() = default;
	RefPtr<IEventMetadata> getCurrent() const override;
	bool next() override;

private:
	EventMetadataEnum(EventMetadataEnumHandle h, RefPtr<const IPublisherMetadata> publisherMeta);
};

EventMetadataEnum::EventMetadataEnum(EventMetadataEnumHandle h, RefPtr<const IPublisherMetadata> publisherMeta)
	: mEventMetadataEnumHandle{ std::move(h) }
	, mPublisherMeta{ std::move(publisherMeta) }
	, mCurrent{ nullptr }
{}

Ref<IEventMetadataEnumerator> EventMetadataEnum::create(EventMetadataEnumHandle h, RefPtr<const IPublisherMetadata> publisherMeta)
{
	return RefObject<EventMetadataEnum>::createRef(std::move(h), publisherMeta);
}

RefPtr<IEventMetadata> EventMetadataEnum::getCurrent() const
{
	// return EventMetadata::create(std::move(mCurrent), mPublisherMeta.get());
	return mCurrentItem;
}

bool EventMetadataEnum::next()
{
	bool hasNext = false;
	auto hEventMetadata = mEventMetadataEnumHandle.next();
	if (hEventMetadata)
	{
		hasNext = true;
		mCurrentItem = EventMetadata::create(std::move(hEventMetadata), mPublisherMeta.get());
	}
	return hasNext;
}

//
// PublisherMetadataCache
//

class PublisherMetadataCache
{
public:
	// Use RefPtr because it is default constructable. 
	using CacheType = std::unordered_map<std::string, RefPtr<PublisherMetadata> > ;
	using iterator = typename CacheType::iterator;
	using const_iterator = typename CacheType::const_iterator;

	PublisherMetadataCache() = default;
	~PublisherMetadataCache() = default;

	RefPtr<PublisherMetadata> cacheLookup(const std::string &publisher) 
	{
		using std::make_pair;

		iterator publisherMetaIt = mCache.find(publisher);

		// If the publisher metadata is not in the cache, create it and return it
		// Otherwise just return the cached item
		
		if (publisherMetaIt != mCache.end())
		{
			return publisherMetaIt->second;
		}
		else
		{
			// Opening the publisher metadata can fail e.g. the publisher is
			// misconfigured. 
			try
			{
				Ref<PublisherMetadata> publisherMeta = PublisherMetadata::create(publisher);
				RefPtr<PublisherMetadata> publisherMetaPtr = publisherMeta.ptr();
				return mCache.insert(make_pair(publisher, publisherMetaPtr)).first->second;
			}
			catch (std::exception &)
			{
				// If opening fails, we add a null to the cache for the provider.
				// This way, we don't repeatedly try to open the provider that is doomed to fail.
				return mCache.insert(make_pair(publisher, RefPtr<PublisherMetadata>(nullptr))).first->second;
			}
		}
	}

	iterator begin() { return mCache.begin(); }
	const_iterator begin() const { return mCache.begin(); }

	iterator end() { return mCache.end(); }
	const_iterator end() const { return mCache.end(); }

private:
	CacheType mCache{};

	// NO! 
	PublisherMetadataCache(const PublisherMetadataCache &) = delete;
	PublisherMetadataCache &operator=(const PublisherMetadataCache &) = delete;
};

static PublisherMetadataCache &cache()
{
	static PublisherMetadataCache theCache;
	return theCache;
}

class PublisherChannelArray : public IPublisherChannelArray
{
	struct Chan
	{
		std::string referencePath{};
		uint32_t referenceIndex{ 0 };
		uint32_t referenceID{ uint32_t(-1) };
		uint32_t flags{ 0 };
		uint32_t messageId{ uint32_t(-1) };
		std::string message{};
	};

	std::vector<Chan> mChan;

public:
	friend class RefObject<PublisherChannelArray>;

	static Ref<IPublisherChannelArray> create(const PublisherMetadataHandle &hMetadata);

	~PublisherChannelArray() = default;

	uint32_t getSize() const override;

	// EvtPublisherMetadataChannelReferencePath
	// Identifies the name attribute of the channel.
	std::string getChannelReferencePath(uint32_t index) const override;

	// EvtPublisherMetadataChannelReferenceIndex
	// Identifies the zero-based index value of the channel in the list of channels
	// I have no idea what this means.
	uint32_t getChannelReferenceIndex(uint32_t index) const override;

	// EvtPublisherMetadataChannelReferenceID
	// Identifies the value attribute of the channel.
	uint32_t getChannelReferenceID(uint32_t index) const override;

	// EvtPublisherMetadataChannelReferenceFlags
	// Identifies the flags value that indicates whether this channel is imported
	// from another provider.
	uint32_t getChannelReferenceFlags(uint32_t index) const override;

	// EvtPublisherMetadataChannelReferenceMessageID
	// Identifies the message attribute of the channel.
	// The property contains the resource identifier that is assigned to the message string. 
	uint32_t getMessageId(uint32_t index) const override;

	std::string getMessage(uint32_t index) const override;

	// Convenience method returning all data for index in one object.
	// Unless you really need all the data, this is no faster than individual calls to
	// the getter for each data item.
	PublisherChannelInfo getChannelInfo(uint32_t index) const;

	// Lookup index of entry with given referenceID, or DWORD(-1)
	uint32_t findIndex(uint32_t referenceID) const override;


private:
	explicit PublisherChannelArray(const PublisherMetadataHandle &hMetadata);

	PublisherChannelArray(const PublisherChannelArray &) = delete;
	PublisherChannelArray &operator=(const PublisherChannelArray &) = delete;
};

class PublisherLevelArray : public IPublisherLevelArray
{
	struct Level
	{
		std::string name{};
		uint32_t value{ uint32_t(-1) };
		uint32_t messageId{ uint32_t(-1) };
		std::string message{};
	};
	std::vector<Level> mLevels;

public:
	friend class RefObject<PublisherLevelArray>;

	static Ref<IPublisherLevelArray> create(const PublisherMetadataHandle &h);

	~PublisherLevelArray() = default;

	uint32_t getSize() const;

	// EvtPublisherMetadataLevelName
	std::string getName(uint32_t index) const override;

	// EvtPublisherMetadataLevelValue
	UINT32 getValue(uint32_t index) const override;

	// EvtPublisherMetadataLevelMessageID
	UINT32 getMessageId(uint32_t index) const override;

	// Returns the message associated with the message id.
	// Returns empty string if none (messagdId == -1)
	std::string getMessage(uint32_t index) const override;

	PublisherLevelInfo getLevelInfo(uint32_t index) const override;

	// lookup the index of entry with given value.
	// value of 0 is invalid and always returns -1.
	// return DWORD(-1) if not present. 
	uint32_t findIndex(uint32_t value) const override;

	// Returns the localized message associated with the level entry for the 
	// given value key. If no message is present, returns the name of the value
	// as per getName(index). 
	// value == 0 is the invalid value; empty string is returned.
	std::string getDisplay(uint32_t value) const override;

private:
	explicit PublisherLevelArray(const PublisherMetadataHandle &h);

	PublisherLevelArray(const PublisherLevelArray &) = delete;
	PublisherLevelArray &operator=(const PublisherLevelArray &) = delete;
};

class PublisherTaskArray : public IPublisherTaskArray
{
	struct Task
	{
		std::string name{};
		GUID eventGuid{};
		uint32_t value{ uint32_t(-1) };
		uint32_t messageId{ uint32_t(-1) };
		std::string message{};
	};
	std::vector<Task> mTasks;

public:
	friend class RefObject<PublisherTaskArray>;

	static Ref<IPublisherTaskArray> create(const PublisherMetadataHandle &h);

	~PublisherTaskArray() = default;

	uint32_t getSize() const override;

	// EvtPublisherMetadataTaskName
	std::string getName(uint32_t index) const override;

	// EvtPublisherMetadataTaskEventGuid
	GUID getEventGuid(uint32_t index) const override;

	// EvtPublisherMetadataTaskValue
	uint32_t getValue(uint32_t index) const override;

	// EvtPublisherMetadataTaskMessageID
	uint32_t getMessageId(uint32_t index) const override;

	std::string getMessage(uint32_t index) const override;

	PublisherTaskInfo getTaskInfo(uint32_t index) const override;

	// Returns the index of the entry with a value equal to the given value.
	uint32_t findIndex(uint32_t value) const override;

	std::string getDisplay(uint32_t value) const override;

private:
	explicit PublisherTaskArray(const PublisherMetadataHandle &hMetadata);

	PublisherTaskArray(const PublisherTaskArray &) = delete;
	PublisherTaskArray &operator=(const PublisherTaskArray &) = delete;
};

// EvtPublisherMetadataOpcode
class PublisherOpcodeArray : public IPublisherOpcodeArray 
{
	struct Opcode
	{
		std::string name{};
		uint32_t value{ uint32_t(-1) };
		uint32_t messageId{ uint32_t(-1) };
		std::string message{};
	};
	std::vector<Opcode> mOpcodes;

public:
	friend class RefObject<PublisherOpcodeArray>;

	static Ref<IPublisherOpcodeArray> create(const PublisherMetadataHandle &h);

	~PublisherOpcodeArray() = default;

	uint32_t getSize() const override;

	// EvtPublisherMetadataOpcodeName
	std::string getName(uint32_t index) const override;

	// EvtPublisherMetadataOpcodeValue
	uint32_t getValue(uint32_t index) const override;

	// EvtPublisherMetadataOpcodeMessageID
	uint32_t getMessageId(uint32_t index) const override;

	std::string getMessage(uint32_t index) const override;

	PublisherOpcodeInfo getOpcodeInfo(uint32_t index) const override;

	uint32_t findIndex(uint32_t opHiWord_TaskLoWord) const override;

	std::string getDisplay(uint32_t opHiWord_TaskLoWord) const override;

private:
	explicit PublisherOpcodeArray(const PublisherMetadataHandle &h);
	PublisherOpcodeArray(const PublisherOpcodeArray &) = delete;
	PublisherOpcodeArray &operator=(const PublisherOpcodeArray &) = delete;
};

// EvtPublisherMetadataKeywords
class PublisherKeywordArray : public IPublisherKeywordArray
{
	struct Keyword
	{
		std::string name{};
		uint64_t  value{uint64_t(-1)};
		uint32_t messageId{uint32_t(-1)};
		std::string message{};
	};
	std::vector<Keyword> mKeywords{};

public:
	friend class RefObject<PublisherKeywordArray>;

	static Ref<IPublisherKeywordArray> create(const PublisherMetadataHandle &h);

	~PublisherKeywordArray() = default;

	uint32_t getSize() const override;

	// EvtPublisherMetadataKeywordName
	std::string getName(uint32_t index) const override;

	// EvtPublisherMetadataKeywordValue
	uint64_t getValue(uint32_t index) const override;

	// EvtPublisherMetadataKeywordMessageID
	uint32_t getMessageId(uint32_t index) const override;

	std::string getMessage(uint32_t index) const override;

	PublisherKeywordInfo getKeywordInfo(uint32_t index) const override;

	std::vector<std::string> getDisplay(uint64_t value) const override;

private:
	explicit PublisherKeywordArray(const PublisherMetadataHandle &hMetadata);
	PublisherKeywordArray(const PublisherKeywordArray &) = delete;
	PublisherKeywordArray &operator=(const PublisherKeywordArray &) = delete;
};

static 
uint32_t getObjectArrayPropertyUInt32(const ObjectArrayPropertyHandle &h, uint32_t propertyId, uint32_t index)
{
	EVT_VARIANT v{};
	h.getProperty(propertyId, index, v);
	return Variant::getUInt32(v);
}

static 
uint64_t getObjectArrayPropertyUInt64(const ObjectArrayPropertyHandle &h, uint32_t propertyId, uint32_t index)
{
	EVT_VARIANT v{};
	h.getProperty(propertyId, index, v);
	return Variant::getUInt64(v);
}

static 
std::string getObjectArrayPropertyString(const ObjectArrayPropertyHandle &h, uint32_t propertyId, uint32_t index)
{
	// The double copy sucks. The ugliness of eliminating it sucks more. 
	auto value = h.getProperty(propertyId, index);
	return Variant::getString(*value);
}

static 
GUID getObjectArrayPropertyGuid(const ObjectArrayPropertyHandle &h, uint32_t propertyId, uint32_t index)
{
	EvtVariantPtr pv = h.getProperty(propertyId, index);
	return Variant::getGuid(*pv);
}

// Returns the size of the array. If an error occurs returns 0.
static uint32_t getObjectArraySize(const ObjectArrayPropertyHandle &h)
{
	// Badly configured providers return unhelpful errors that aren't
	// actionable so we just say the array is empty and the caller can
	// just skip it and e.g. show empty/blank values for the misconfigured
	// metadata. We call the overload of getSize() that does nothing if an
	// error occurs. 

	uint32_t size = 0;
	/* ignore errors */ h.getSize(&size);

	return size;
}

static 
std::optional<GUID> getPublisherMetadataPropertyGuid(const PublisherMetadataHandle &h, EVT_PUBLISHER_METADATA_PROPERTY_ID propertyId)
{
	EvtVariantPtr pv = h.getProperty(propertyId);
	if (!Variant::isNull(*pv))
		return Variant::getGuid(*pv);
	return {};
}

static 
std::optional<std::string> getPublisherMetadataPropertyString(const PublisherMetadataHandle &h, EVT_PUBLISHER_METADATA_PROPERTY_ID propertyId)
{
	auto pv = h.getProperty(propertyId);
	if (!Variant::isNull(*pv))
		return Variant::getString(*pv);
	return {};
}

static
std::optional<uint32_t> getPublisherMetadataPropertyUInt32(const PublisherMetadataHandle &h, EVT_PUBLISHER_METADATA_PROPERTY_ID propertyId)
{
	EVT_VARIANT v{};
	h.getProperty(propertyId, v);
	if (!Variant::isNull(v))
		return Variant::getUInt32(v);
	return {};
}

//
// PublisherChannelArray implementation
//

Ref<IPublisherChannelArray> PublisherChannelArray::create(const PublisherMetadataHandle &hMetadata)
{
	return RefObject<PublisherChannelArray>::createRef(hMetadata);
}

PublisherChannelArray::PublisherChannelArray(const PublisherMetadataHandle &hMetadata)
{
	auto channels = ObjectArrayPropertyHandle::getChannelReferences(hMetadata);

	// In testing, it became apparent that some providers have borked their configuration
	// and it results in lots of errors with vague error codes (like resource not found)
	// that the caller can't do anything about. If this happens, we just bail out of 
	// the eager initialization and return an empty object. It'll report size of 
	// zero and the caller (if they use it correctly), will just skip it 

	uint32_t size = getObjectArraySize(channels);
	if (size > 0)
	{
		mChan.resize(size);

		for (uint32_t index = 0; index < size; ++index)
		{
			mChan[index].referencePath = getObjectArrayPropertyString(channels, EvtPublisherMetadataChannelReferencePath, index);
			mChan[index].referenceIndex = getObjectArrayPropertyUInt32(channels, EvtPublisherMetadataChannelReferenceIndex, index);
			mChan[index].referenceID = getObjectArrayPropertyUInt32(channels, EvtPublisherMetadataChannelReferenceID, index);
			mChan[index].flags = getObjectArrayPropertyUInt32(channels, EvtPublisherMetadataChannelReferenceFlags, index);

			uint32_t messageId = getObjectArrayPropertyUInt32(channels, EvtPublisherMetadataChannelReferenceMessageID, index);
			mChan[index].messageId = messageId;
			if (messageId != uint32_t(-1))
				mChan[index].message = formatMessage(hMetadata, messageId);
		}
	}
}

uint32_t PublisherChannelArray::getSize() const 
{ 
	return uint32_t(mChan.size()); 
}

std::string PublisherChannelArray::getChannelReferencePath(uint32_t index) const
{
	return mChan.at(index).referencePath;
}

uint32_t PublisherChannelArray::getChannelReferenceIndex(uint32_t index) const
{
	return mChan.at(index).referenceIndex;
}

uint32_t PublisherChannelArray::getChannelReferenceID(uint32_t index) const
{
	return mChan.at(index).referenceID;
}

uint32_t PublisherChannelArray::getChannelReferenceFlags(uint32_t index) const
{
	auto &chan = mChan.at(index);
	return chan.flags;
}

uint32_t PublisherChannelArray::getMessageId(uint32_t index) const
{
	auto &chan = mChan.at(index);
	return chan.messageId;
}

std::string PublisherChannelArray::getMessage(uint32_t index) const
{
	auto &chan = mChan.at(index);
	return chan.message;
}


PublisherChannelInfo PublisherChannelArray::getChannelInfo(uint32_t index) const 
{
	return PublisherChannelInfo{getChannelReferencePath(index), getChannelReferenceIndex(index),
		getChannelReferenceID(index), getChannelReferenceFlags(index), getMessageId(index), 
		getMessage(index)};
}

uint32_t PublisherChannelArray::findIndex(uint32_t referenceID) const
{
	uint32_t index = uint32_t(-1);
	uint32_t count = getSize();
	for (uint32_t i = 0; i < count; ++i)
	{
		if (getChannelReferenceID(i) == referenceID)
		{
			index = i;
			break;
		}
	}
	return index;
}

//
// PublisherLevelArray
//

Ref<IPublisherLevelArray> PublisherLevelArray::create(const PublisherMetadataHandle &hMetadata)
{
	return RefObject<PublisherLevelArray>::createRef(hMetadata);
}

PublisherLevelArray::PublisherLevelArray(const PublisherMetadataHandle &hMetadata)
{
	ObjectArrayPropertyHandle levelsArrayHandle = ObjectArrayPropertyHandle::getLevels(hMetadata);

	uint32_t size = getObjectArraySize(levelsArrayHandle);
	if (size > 0)
	{
		mLevels.resize(size);

		for (uint32_t index = 0; index < size; ++index)
		{
			mLevels[index].name = getObjectArrayPropertyString(levelsArrayHandle, EvtPublisherMetadataLevelName, index);
			mLevels[index].value = getObjectArrayPropertyUInt32(levelsArrayHandle, EvtPublisherMetadataLevelValue, index);
			uint32_t messageId = getObjectArrayPropertyUInt32(levelsArrayHandle, EvtPublisherMetadataLevelMessageID, index);
			mLevels[index].messageId = messageId;
			if (messageId != uint32_t(-1))
				mLevels[index].message = formatMessage(hMetadata, messageId);
		}
	}
}

uint32_t PublisherLevelArray::getSize() const
{
	return static_cast<uint32_t>(mLevels.size());
}

std::string PublisherLevelArray::getName(uint32_t index) const
{
	auto &level = mLevels.at(index);
	return level.name;
}

// EvtPublisherMetadataLevelValue
uint32_t PublisherLevelArray::getValue(uint32_t index) const
{
	auto &level = mLevels.at(index);
	return level.value;

}

PublisherLevelInfo PublisherLevelArray::getLevelInfo(uint32_t index) const 
{
	return PublisherLevelInfo{getName(index), getValue(index), getMessageId(index),
		getMessage(index)};
}

uint32_t PublisherLevelArray::findIndex(uint32_t value) const
{
	return findIndexImpl(this, value);
}

// EvtPublisherMetadataLevelMessageID
uint32_t PublisherLevelArray::getMessageId(uint32_t index) const
{
	auto &level = mLevels.at(index);
	return level.messageId;
}

std::string PublisherLevelArray::getMessage(uint32_t index) const
{
	auto &level = mLevels.at(index);
	return level.message;
}

std::string PublisherLevelArray::getDisplay(uint32_t value) const
{
	uint32_t index = findIndex(value);
	if (index == uint32_t(-1))
		return {};
	if (getMessageId(index) == uint32_t(-1))
		return getName(index);
	else
		return getMessage(index);
}

//
// PublisherTaskArray
// 

Ref<IPublisherTaskArray> PublisherTaskArray::create(const PublisherMetadataHandle &hMetadata)
{
	return RefObject<PublisherTaskArray>::createRef(hMetadata);
}

PublisherTaskArray::PublisherTaskArray(const PublisherMetadataHandle &hMetadata)
{
	ObjectArrayPropertyHandle tasksHandle = ObjectArrayPropertyHandle::getTasks(hMetadata);

	uint32_t size = getObjectArraySize(tasksHandle);
	if (size > 0)
	{
		mTasks.resize(size);

		for (uint32_t index = 0; index < size; ++index)
		{			
			mTasks[index].name = getObjectArrayPropertyString(tasksHandle, EvtPublisherMetadataTaskName, index);
			mTasks[index].eventGuid = getObjectArrayPropertyGuid(tasksHandle, EvtPublisherMetadataTaskEventGuid, index);
			mTasks[index].value = getObjectArrayPropertyUInt32(tasksHandle, EvtPublisherMetadataTaskValue, index);
			uint32_t messageId = getObjectArrayPropertyUInt32(tasksHandle, EvtPublisherMetadataTaskMessageID, index);
			mTasks[index].messageId = messageId;
			if (messageId != uint32_t(-1))
				mTasks[index].message = formatMessage(hMetadata, messageId);
		}
	}
}

uint32_t PublisherTaskArray::getSize() const
{ 
	// return mObjArrayPropHandle.getSize();
	return uint32_t(mTasks.size());
}

std::string PublisherTaskArray::getName(uint32_t index) const
{
	auto &task = mTasks.at(index);
	return task.name;
}

GUID PublisherTaskArray::getEventGuid(uint32_t index) const
{
	auto &task = mTasks.at(index);
	return task.eventGuid;
}

uint32_t PublisherTaskArray::getValue(uint32_t index) const
{
	auto & task = mTasks.at(index);
	return task.value;

}

uint32_t PublisherTaskArray::getMessageId(uint32_t index) const
{
	auto & task = mTasks.at(index);
	return task.messageId;
}

std::string PublisherTaskArray::getMessage(uint32_t index) const
{
	auto &task = mTasks.at(index);
	return task.message;
}

PublisherTaskInfo PublisherTaskArray::getTaskInfo(uint32_t index) const
{
	return PublisherTaskInfo{getName(index), getEventGuid(index), getValue(index),
		getMessageId(index), getMessage(index)};
}

uint32_t PublisherTaskArray::findIndex(uint32_t value) const
{
	return findIndexImpl(this, value);
}

std::string PublisherTaskArray::getDisplay(uint32_t value) const
{
	uint32_t index = findIndex(value);
	if (index == uint32_t(-1))
		return {};

	if (getMessageId(index) != UINT(-1))
		return getMessage(index);
	else
		return getName(index);

}

//
// PublisherOpcodeArray
//

Ref<IPublisherOpcodeArray> PublisherOpcodeArray::create(const PublisherMetadataHandle &hMetadata)
{
	return RefObject<PublisherOpcodeArray>::createRef(hMetadata);
}

PublisherOpcodeArray::PublisherOpcodeArray(const PublisherMetadataHandle &hMetadata)
{
	ObjectArrayPropertyHandle opcodesHandle = ObjectArrayPropertyHandle::getOpcodes(hMetadata);

	uint32_t size = getObjectArraySize(opcodesHandle);
	if (size > 0)
	{
		mOpcodes.resize(size);

		for (uint32_t index = 0; index < size; ++index)
		{
			mOpcodes[index].name = getObjectArrayPropertyString(opcodesHandle, EvtPublisherMetadataOpcodeName, index);
			mOpcodes[index].value = getObjectArrayPropertyUInt32(opcodesHandle, EvtPublisherMetadataOpcodeValue, index);
			uint32_t messageId = getObjectArrayPropertyUInt32(opcodesHandle, EvtPublisherMetadataOpcodeMessageID, index);
			mOpcodes[index].messageId = messageId;
			if (messageId != uint32_t(-1))
				mOpcodes[index].message = formatMessage(hMetadata, messageId);
		}
	}
}

uint32_t PublisherOpcodeArray::getSize() const
{
	return uint32_t(mOpcodes.size());
}

std::string PublisherOpcodeArray::getName(uint32_t index) const 
{
	auto &opcode = mOpcodes.at(index);
	return opcode.name;
}

uint32_t PublisherOpcodeArray::getValue(uint32_t index) const
{
	auto &opcode = mOpcodes.at(index);
	return opcode.value;
}

uint32_t PublisherOpcodeArray::getMessageId(uint32_t index) const
{
	auto &opcode = mOpcodes.at(index);
	return opcode.messageId;
}

std::string PublisherOpcodeArray::getMessage(uint32_t index) const 
{
	auto &opcode = mOpcodes.at(index);
	return opcode.message;
}

PublisherOpcodeInfo PublisherOpcodeArray::getOpcodeInfo(uint32_t index) const 
{
	return PublisherOpcodeInfo{ getName(index), getValue(index), getMessageId(index),
		getMessage(index) };
}

uint32_t PublisherOpcodeArray::findIndex(uint32_t opcodeHiWord_TaskLoWord) const
{
	// The rationale for this search is a little under documented by MSFT.
	// The XML schema allows for 'global' opcodes or to be associate with 
	// a task. MSFT handles this by reserving task 0 for global opcodes, 
	// and storing the custom task in the high word of the retrieved DWORD.
	// 
	// In dumping the entire event metadata on a dev machine, it never occurred 
	// that the opcode (HIWORD) matched, and the LOWORD was NOT either 0 or 
	// a match to the task value retrieved from the event and passed-in.
	//
	// Since it is theoretically possible for this to happen we just return -1
	// but it appears the system (probably the manifest compiler?) prevents this.
	uint32_t index = uint32_t(-1);

	uint32_t opcodeValue= HIWORD(opcodeHiWord_TaskLoWord);
	uint32_t taskValue = LOWORD(opcodeHiWord_TaskLoWord);

	uint32_t size = this->getSize();
	for (uint32_t i = 0; i < size; ++i)
	{
		auto ocv = this->getValue(i);
		if (opcodeValue == HIWORD(ocv))
		{
			if (0 == LOWORD(ocv))
			{
				index = i;
				// Keep going to see if we get a task match
			}
			else
			{
				if (taskValue == LOWORD(ocv))
				{
					// Got a match on both opcode and task. Done.
					index = i;
					break;
				}
				// else: never seen it, but we just return -1 if so.
				//       does not happen in practice.
			}
		}
	}
	return index;
}

std::string PublisherOpcodeArray::getDisplay(uint32_t opcode_task) const
{
	auto index = this->findIndex(opcode_task);
	if (index == uint32_t(-1)) // AFAIK, this won't ever happen.
		return {};
	if (getMessageId(index) != uint32_t(-1))
		return getMessage(index);
	else
		return getName(index);
}

//
// PublisherKeywordArray
//

Ref<IPublisherKeywordArray> PublisherKeywordArray::create(const PublisherMetadataHandle &hMetadata)
{
	return RefObject<PublisherKeywordArray>::createRef(hMetadata);
}

PublisherKeywordArray::PublisherKeywordArray(const PublisherMetadataHandle &hMetadata)
{
	ObjectArrayPropertyHandle keywordArrayHandle = ObjectArrayPropertyHandle::getKeywords(hMetadata);

	uint32_t size = getObjectArraySize(keywordArrayHandle);
	if (size > 0)
	{
		mKeywords.resize(size);

		for (uint32_t index = 0; index < size; ++index)
		{
			mKeywords[index].name = getObjectArrayPropertyString(keywordArrayHandle, EvtPublisherMetadataKeywordName, index);
			mKeywords[index].value = getObjectArrayPropertyUInt64(keywordArrayHandle, EvtPublisherMetadataKeywordValue, index);
			uint32_t messageId = getObjectArrayPropertyUInt32(keywordArrayHandle, EvtPublisherMetadataKeywordMessageID, index);
			mKeywords[index].messageId = messageId;
			if (messageId != uint32_t(-1))
			{ 
				mKeywords[index].message = formatMessage(hMetadata, messageId);
			}
		}
	}
}

uint32_t PublisherKeywordArray::getSize() const
{
	return uint32_t(mKeywords.size());
}

// EvtPublisherMetadataKeywordName
std::string PublisherKeywordArray::getName(uint32_t index) const
{
	auto &keyword = mKeywords.at(index);
	return keyword.name;
}

// EvtPublisherMetadataKeywordValue
uint64_t PublisherKeywordArray::getValue(uint32_t index) const
{
	auto & keyword = mKeywords.at(index);
	return keyword.value;
}

// EvtPublisherMetadataKeywordMessageID
uint32_t PublisherKeywordArray::getMessageId(uint32_t index) const
{
	auto & keyword = mKeywords.at(index);
	return keyword.messageId;
}

std::string PublisherKeywordArray::getMessage(uint32_t index) const 
{
	auto &kw = this->mKeywords.at(index);
	return kw.message;
}

PublisherKeywordInfo PublisherKeywordArray::getKeywordInfo(uint32_t index) const
{
	return PublisherKeywordInfo{ getName(index), getValue(index), getMessageId(index), getMessage(index) };
}

std::vector<std::string> PublisherKeywordArray::getDisplay(uint64_t value) const
{
	auto size = mKeywords.size();
	size_t count = 0;
	for (size_t i = 0; i < size; ++i)
	{
		// Maybe this needs explaining.
		// Keywords correspond to bits in the value.
		// So in the event metadata will set e.g. bits X,Y,Z in it's keyword value.
		// To find the corresponding keyword metadata, search the keyword metadata 
		// array to find a keyword with bit X set in it's value.

		// Bit set? If so, we have one more.
		if (value & mKeywords[i].value)
			count += 1;
	}

	// Didn't find any (ruh roh). Return empty.
	if (count == 0)
		return {};

	// Size it once, no need for push_back.
	// The arrays are small and running them twice is NBD vs.
	// additional memory shenanigans for dynamic resizing.
	std::vector<std::string> result{count};


	count = 0;
	for (size_t i = 0; i < size; ++i)
	{
		// Bit set?
		if (value & mKeywords[i].value)
		{
			// Yup. Fall back to the name if the message message is missing.
			if (mKeywords[i].messageId != uint32_t(-1))
				result[count] = mKeywords[i].message;
			else 
				result[count] = mKeywords[i].name;
			count += 1;
		}
	}

	return result;
}

//
// Static named constructor/factory function
//

RefPtr<IPublisherMetadata> IPublisherMetadata::cacheOpenProvider(const std::string &provider)
{
	return PublisherMetadata::cacheOpenProvider(provider);
}

//
// PublisherMetadataImpl
//

PublisherMetadataImpl::PublisherMetadataImpl(PublisherMetadataHandle publisherMetaHandle)
	: mPublisherGuid{ getPublisherMetadataPropertyGuid(publisherMetaHandle, EvtPublisherMetadataPublisherGuid) }
	, mResourceFilePath{ getPublisherMetadataPropertyString(publisherMetaHandle, EvtPublisherMetadataResourceFilePath) }
	, mParametersFilePath{ getPublisherMetadataPropertyString(publisherMetaHandle, EvtPublisherMetadataParameterFilePath) }
	, mMessageFilePath{ getPublisherMetadataPropertyString(publisherMetaHandle, EvtPublisherMetadataMessageFilePath) }
	, mHelpLink{ getPublisherMetadataPropertyString(publisherMetaHandle, EvtPublisherMetadataHelpLink) }
	, mMessageId{ getPublisherMetadataPropertyUInt32(publisherMetaHandle, EvtPublisherMetadataPublisherMessageID) }
	, mChannels{ PublisherChannelArray::create(publisherMetaHandle) }
	, mLevels{ PublisherLevelArray::create(publisherMetaHandle) }
	, mTasks{ PublisherTaskArray::create(publisherMetaHandle) }
	, mOpcodes{ PublisherOpcodeArray::create(publisherMetaHandle) }
	, mKeywords{ PublisherKeywordArray::create(publisherMetaHandle) }
{
	if (mMessageId.has_value())
	{
		uint32_t msgId = mMessageId.value();
		if (msgId != uint32_t(-1))
		{
			mMessage = formatMessage(publisherMetaHandle, msgId);
		}
	}

	// Do this last as it invalidates publisherMetaHandle.
	mPublisherMetadataHandle = std::move(publisherMetaHandle);
}

FormattedEventRecord PublisherMetadataImpl::format(const EventRecordHandle &recordHandle) const
{
	return FormattedEventRecord{
		formatMessage(mPublisherMetadataHandle, recordHandle, EvtFormatMessageEvent),
		formatMessage(mPublisherMetadataHandle, recordHandle, EvtFormatMessageLevel),
		formatMessage(mPublisherMetadataHandle, recordHandle, EvtFormatMessageTask),
		formatMessage(mPublisherMetadataHandle, recordHandle, EvtFormatMessageOpcode),
		formatKeyword(mPublisherMetadataHandle, recordHandle),
		formatMessage(mPublisherMetadataHandle, recordHandle, EvtFormatMessageChannel),
		formatMessage(mPublisherMetadataHandle, recordHandle, EvtFormatMessageProvider),
	};
}

//
// PublisherMetadata implementation
//

FormattedEventRecord PublisherMetadata::formatEvent(const EventRecordHandle &recordHandle)
{
	using Windows::EventLog::formatMessage;
	using Windows::EventLog::formatKeyword;

	return FormattedEventRecord{
		formatMessage(recordHandle, EvtFormatMessageEvent),
		formatMessage(recordHandle, EvtFormatMessageLevel),
		formatMessage(recordHandle, EvtFormatMessageTask),
		formatMessage(recordHandle, EvtFormatMessageOpcode),
		formatKeyword(recordHandle),
		formatMessage(recordHandle, EvtFormatMessageChannel),
		formatMessage(recordHandle, EvtFormatMessageProvider),
	};
}

PublisherMetadata::~PublisherMetadata()
{}

RefPtr<PublisherMetadata> PublisherMetadata::cacheOpenProvider(const std::string &id)
{
	return cache().cacheLookup(id);
}

Ref<IPublisherMetadata> PublisherMetadata::openProvider(const std::string &publisherId)
{
	return PublisherMetadata::create(publisherId);
}

Ref<IPublisherMetadata> PublisherMetadata::openArchiveLogFile(const std::string &publisherId, const std::string &filePath)
{
	return PublisherMetadata::create(publisherId, filePath);
}

Ref<PublisherMetadata> PublisherMetadata::create(const std::string &providerId)
{
	return RefObject<PublisherMetadata>::createRef(providerId);
}

Ref<PublisherMetadata> PublisherMetadata::create(const std::string &providerId, const std::string &filePath)
{
	return RefObject<PublisherMetadata>::createRef(providerId, filePath);
}

PublisherMetadata::PublisherMetadata(const std::string &providerId)
	: d_ptr(std::make_unique<PublisherMetadataImpl>(PublisherMetadataHandle::openProvider(providerId)))
{
}

PublisherMetadata::PublisherMetadata(const std::string &providerId, const std::string &filepath)
	: d_ptr(std::make_unique<PublisherMetadataImpl>(PublisherMetadataHandle::openArchiveFile(providerId, filepath)))
{
}

// EvtPublisherMetadataPublisherGuid
std::optional<GUID> PublisherMetadata::getPublisherGuid() const
{
	return d_ptr->mPublisherGuid;
}

// EvtPublisherMetadataResourceFilePath
std::optional<std::string> PublisherMetadata::getResourceFilePath() const
{
	return d_ptr->mResourceFilePath;
}

// EvtPublisherMetadataParameterFilePath
std::optional<std::string> PublisherMetadata::getParametersFilePath() const
{
	return d_ptr->mParametersFilePath;
}

// EvtPublisherMetadataMessageFilePath
std::optional<std::string> PublisherMetadata::getMessageFilePath() const
{
	return d_ptr->mMessageFilePath;
}

// EvtPublisherMetadataHelpLink
std::optional<std::string> PublisherMetadata::getHelpLink() const
{
	return d_ptr->mHelpLink;
}

// EvtPublisherMetadataPublisherMessageID
std::optional<uint32_t> PublisherMetadata::getPublisherMessageId() const
{
	return d_ptr->mMessageId;
}

std::string PublisherMetadata::getPublisherMessage() const 
{
	return d_ptr->mMessage;
}

// EvtPublisherMetadataChannelReferences
Ref<IPublisherChannelArray> PublisherMetadata::getChannels() const
{
	return d_ptr->mChannels;
}

// EvtPublisherMetadataLevels
Ref<IPublisherLevelArray> PublisherMetadata::getLevels() const
{
	return d_ptr->mLevels;
}

// EvtPublisherMetadataTasks
Ref<IPublisherTaskArray> PublisherMetadata::getTasks() const
{
	return d_ptr->mTasks;
}

// EvtPublisherMetadataOpcodes
Ref<IPublisherOpcodeArray> PublisherMetadata::getOpcodes() const
{
	return d_ptr->mOpcodes;
}

// EvtPublisherMetadataKeywords
Ref<IPublisherKeywordArray> PublisherMetadata::getKeywords() const
{
	return d_ptr->mKeywords;
}

Ref<IEventMetadataEnumerator> PublisherMetadata::openEventMetadataEnum() const 
{
	return EventMetadataEnum::create(
		d_ptr->mPublisherMetadataHandle.openEventMetadataEnum(), 
		RefPtr<const IPublisherMetadata>(this));
}


std::string PublisherMetadata::formatMessage(uint32_t messageID) const
{
	uint32_t size = 256;
	std::unique_ptr<wchar_t[]> buffer(std::make_unique<wchar_t[]>(size));
	std::string msg;

	SysErr err = d_ptr->mPublisherMetadataHandle.formatMessage(messageID, size, buffer.get(), &size);
	if (!err)
	{
		msg = to_utf8(buffer.get());
		// TODO: msg = to_utf8(buffer.get(), size_t(size) - 1u);
	}
	else
	{
		if (err == ERROR_INSUFFICIENT_BUFFER)
		{
			buffer = std::make_unique<wchar_t[]>(size);
			err = d_ptr->mPublisherMetadataHandle.formatMessage(messageID, size, buffer.get(), &size);
			if (err == ERROR_SUCCESS)
			{
				msg = to_utf8(buffer.get());
				// TODO: msg = to_utf8(buffer.get(), size_t(size) - 1u);
			}
			else
			{
				if (!isIgnoredFormatMessageError(err.getCode()))
				{
					THROW_(SystemError, err);
				}
			}
		}
		else
		{ 
			if (!isIgnoredFormatMessageError(err.getCode()))
			{
				THROW_(SystemError, err);
			}
		}
	}

	return msg;
}

std::string PublisherMetadata::lookupChannelDisplay(uint32_t channelValue) const
{
	Ref<IPublisherChannelArray> channels = getChannels();
	uint32_t index = channels->findIndex(channelValue);
	if (index == uint32_t(-1))
		return {};
	// Should we just add a getDisplay() method?
	uint32_t messageId = channels->getMessageId(index);
	if (messageId != uint32_t(-1))
	{
		return channels->getMessage(index);
	}
	else
	{
		return channels->getChannelReferencePath(index);
	}
}

std::string PublisherMetadata::lookupLevelDisplay(uint32_t levelValue) const
{
	Ref<IPublisherLevelArray> levels = getLevels();
	return levels->getDisplay(levelValue);
}

std::string PublisherMetadata::lookupTaskDisplay(uint32_t taskValue) const
{
	Ref<IPublisherTaskArray> tasks = getTasks();
	return tasks->getDisplay(taskValue);
}

std::string PublisherMetadata::lookupOpcodesDisplay(uint32_t op_task) const
{
	Ref<IPublisherOpcodeArray> opcodes = getOpcodes();
	return opcodes->getDisplay(op_task);
}

std::vector<std::string> PublisherMetadata::lookupKeywordsDisplay(uint64_t maskBits) const
{
	auto keywords = getKeywords();
	return keywords->getDisplay(maskBits);
}

FormattedEventRecord PublisherMetadata::format(const EventRecordHandle &recordHandle) const
{
	return d_ptr->format(recordHandle);
}

}