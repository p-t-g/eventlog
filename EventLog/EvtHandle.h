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

#ifndef TSI_INCLUDED_WINDOWS_H
#define TSI_INCLUDED_WINDOWS_H
#include <Windows.h>
#endif 

#ifndef TSI_INCLUDED_WINEVT_H
#define TSI_INCLUDED_WINEVT_H
#include <winevt.h>
#endif

#include "CommonTypes.h"
#include "EvtVariant.h"
#include "WinSys.h"

#include <cstddef>
#include <string>
#include <optional>
#include <vector>

#include <process.h>

namespace Windows::EventLog
{ 

// Unique 
class EvtHandle
{
	EVT_HANDLE mHandle;
public:

	constexpr EvtHandle() noexcept
		: mHandle{nullptr}
	{}

	constexpr explicit EvtHandle(EVT_HANDLE h) noexcept
		: mHandle{h}
	{}

	constexpr EvtHandle(std::nullptr_t) noexcept 
		: mHandle{nullptr}
	{}

	EvtHandle(EvtHandle &&rhs) noexcept;	
	EvtHandle &operator=(EvtHandle &&rhs) noexcept;
	EvtHandle &operator=(EVT_HANDLE h) noexcept;
	~EvtHandle() noexcept;

	SysErr close() noexcept;
	
	EVT_HANDLE handle() const noexcept;
	
	bool isNull() const noexcept;

	explicit operator bool() const noexcept
	{
		return !isNull();
	}

	friend bool operator==(const EvtHandle &a, const EvtHandle &b) noexcept
	{
		return a.mHandle == b.mHandle;
	}

	friend bool operator!=(const EvtHandle &a, const EvtHandle &b) noexcept
	{
		return a.mHandle != b.mHandle;
	}

	void swap(EvtHandle &a)
	{
		EVT_HANDLE tmp = a.mHandle;
		a.mHandle = mHandle;
		mHandle = tmp;
	}
};

struct EvtHandleClose
{
	SysErr operator()(EVT_HANDLE h) 
	{
		SysErr err{};
		if (h)
		{
			BOOL success = ::EvtClose(h);
			if (!success)
				err = SysErr::getLast();
		}
		return err;
	}
};

class ChannelEnumHandle
{
	EvtHandle mHandle;
public:

	static ChannelEnumHandle open();

	constexpr explicit ChannelEnumHandle() noexcept
		: mHandle{nullptr}
	{}

	constexpr explicit ChannelEnumHandle(std::nullptr_t) noexcept 
		: mHandle{nullptr}
	{}

	ChannelEnumHandle(ChannelEnumHandle &&h) = default;
	ChannelEnumHandle &operator=(ChannelEnumHandle &&rhs) = default;
	~ChannelEnumHandle() = default;

	std::optional<std::wstring> nextChannelPath(const UseWideChar_t &) const;
	std::optional<std::string> nextChannelPath() const;

	explicit operator bool() const noexcept
	{
		return !mHandle.isNull();
	}

	friend bool operator==(const ChannelEnumHandle &a, const ChannelEnumHandle &b)
	{
		return a.mHandle == b.mHandle;
	}

	friend bool operator!=(const ChannelEnumHandle &a, const ChannelEnumHandle &b)
	{
		return a.mHandle != b.mHandle;
	}

private:

	constexpr explicit ChannelEnumHandle(EVT_HANDLE h) noexcept
		: mHandle{h}
	{}

private:
	ChannelEnumHandle(const ChannelEnumHandle &) = delete;
	ChannelEnumHandle &operator=(const ChannelEnumHandle &) = delete;
};

template<typename E, typename = std::enable_if_t<std::is_enum_v<E> > >
auto as_underlying(E value) -> std::underlying_type_t<E>
{
	return static_cast<std::underlying_type_t<E> >(value);
}

class ChannelConfigHandle  
{
public:

	static ChannelConfigHandle open(const std::string &channelPath);

	constexpr ChannelConfigHandle() noexcept 
		: mHandle{nullptr} 
	{}

	constexpr ChannelConfigHandle(std::nullptr_t) noexcept
		: mHandle{nullptr}
	{}

	~ChannelConfigHandle() = default;
	ChannelConfigHandle(ChannelConfigHandle &&rhs) = default;
	ChannelConfigHandle &operator=(ChannelConfigHandle &&rhs) = default;

	void setPropertyValue(EVT_CHANNEL_CONFIG_PROPERTY_ID Id, PEVT_VARIANT pv) const;
	void getPropertyValue(EVT_CHANNEL_CONFIG_PROPERTY_ID Id, PEVT_VARIANT pv,
		uint32_t size = sizeof(EVT_VARIANT)) const;

	EvtVariantPtr getPropertyValue(EVT_CHANNEL_CONFIG_PROPERTY_ID Id, 
		uint32_t dataSize = 512u, uint32_t *totalUsedSize = nullptr) const;

	void save() const;

	explicit operator bool() const noexcept
	{
		return !mHandle.isNull();
	}

	friend bool operator==(const ChannelConfigHandle &a, const ChannelConfigHandle &b) noexcept
	{
		return a.mHandle == b.mHandle;
	}

	friend bool operator!=(const ChannelConfigHandle &a, const ChannelConfigHandle &b) noexcept
	{
		return a.mHandle != b.mHandle;
	}

private:

	constexpr explicit ChannelConfigHandle(EVT_HANDLE h) noexcept
		: mHandle{h}
	{}

private:
	EVT_HANDLE handle() const { return mHandle.handle(); }
	EvtHandle mHandle;

private:
	ChannelConfigHandle(const ChannelConfigHandle &) = delete;
	ChannelConfigHandle &operator=(const ChannelConfigHandle &) = delete;
	
};

class PublisherEnumHandle 
{
	EvtHandle mHandle;
public:
	static PublisherEnumHandle open();
	
	constexpr PublisherEnumHandle() noexcept : mHandle{nullptr} {}
	constexpr PublisherEnumHandle(std::nullptr_t) noexcept : mHandle{nullptr} {}

	~PublisherEnumHandle() = default;
	PublisherEnumHandle(PublisherEnumHandle &&rhs) = default;
	PublisherEnumHandle &operator=(PublisherEnumHandle &&rhs) = default;

	std::optional<std::wstring> nextPublisherId(int) const;
	std::optional<std::string> nextPublisherId() const;

	explicit operator bool() const noexcept
	{
		return !mHandle.isNull();
	}

	friend bool operator==(const PublisherEnumHandle &a, const PublisherEnumHandle &b)
	{
		return a.mHandle == b.mHandle;
	}

	friend bool operator!=(const PublisherEnumHandle &a, const PublisherEnumHandle &b)
	{
		return a.mHandle != b.mHandle;
	}

private:

	constexpr explicit PublisherEnumHandle(EVT_HANDLE h) noexcept
		: mHandle(h)
	{}

private:
	PublisherEnumHandle(const PublisherEnumHandle &) = delete;
	PublisherEnumHandle &operator=(const PublisherEnumHandle &) = delete;
};

class PublisherMetadataHandle;
class EventMetadataHandle
{
	EvtHandle mEventMetadataHandle;
public:
	constexpr EventMetadataHandle()
		: mEventMetadataHandle{ nullptr }
	{}

	constexpr explicit EventMetadataHandle(std::nullptr_t)
		: mEventMetadataHandle{ nullptr }
	{}

	constexpr explicit EventMetadataHandle(EVT_HANDLE h) 
		: mEventMetadataHandle{h}
	{}

	EventMetadataHandle(EventMetadataHandle &&) = default;
	EventMetadataHandle &operator=(EventMetadataHandle &&) = default;
	~EventMetadataHandle() = default;

	explicit operator bool() const noexcept
	{
		return !mEventMetadataHandle.isNull();
	}

	friend bool operator==(const EventMetadataHandle &a, const EventMetadataHandle &b)
	{
		return a.mEventMetadataHandle == b.mEventMetadataHandle;
	}

	friend bool operator!=(const EventMetadataHandle &a, const EventMetadataHandle &b)
	{
		return a.mEventMetadataHandle != b.mEventMetadataHandle;
	}

	void getProperty(EVT_EVENT_METADATA_PROPERTY_ID propertyId, EVT_VARIANT &v) const;
	EvtVariantPtr getProperty(EVT_EVENT_METADATA_PROPERTY_ID propertyId, uint32_t size = 512u, uint32_t *actualSize = nullptr) const;

private: 
	EventMetadataHandle(const EventMetadataHandle &) = delete;
	EventMetadataHandle &operator=(const EventMetadataHandle &) = delete;
};

class EventMetadataEnumHandle
{
	EvtHandle mhEventMetadataEnum;
public:

	constexpr EventMetadataEnumHandle() noexcept 
		: mhEventMetadataEnum{nullptr} 
	{}

	constexpr explicit EventMetadataEnumHandle(EVT_HANDLE hEventMetadataEnum) noexcept
		: mhEventMetadataEnum{hEventMetadataEnum}
	{}

	EventMetadataEnumHandle(EventMetadataEnumHandle &&rhs) = default;
	EventMetadataEnumHandle &operator=(EventMetadataEnumHandle &&rhs) = default;
	~EventMetadataEnumHandle() = default;

	EventMetadataHandle next() const;

private:
	EventMetadataEnumHandle(const EventMetadataEnumHandle &) = delete;
	EventMetadataEnumHandle &operator=(const EventMetadataEnumHandle &) = delete;
};

class PublisherMetadataHandle
{
	EvtHandle mHandle;
public:

	static PublisherMetadataHandle openProvider(const std::string &publisherId);
	static PublisherMetadataHandle openArchiveFile(const std::string &publishedId, const std::string &filePath);
	
	constexpr PublisherMetadataHandle() noexcept : mHandle{nullptr} {}
	constexpr explicit PublisherMetadataHandle(std::nullptr_t) noexcept : mHandle{nullptr} {}
	PublisherMetadataHandle(PublisherMetadataHandle &&rhs) = default;
	PublisherMetadataHandle &operator=(PublisherMetadataHandle &&rhs) = default;

	EvtVariantPtr getProperty(EVT_PUBLISHER_METADATA_PROPERTY_ID propId, uint32_t size = 512u, uint32_t *actualSize = nullptr) const;
	void getProperty(EVT_PUBLISHER_METADATA_PROPERTY_ID propId, EVT_VARIANT &v) const;

	explicit operator bool() const noexcept
	{
		return !mHandle.isNull();
	}

	operator EVT_HANDLE() const noexcept
	{
		return mHandle.handle();
	}

	EventMetadataEnumHandle openEventMetadataEnum() const;

	friend bool operator==(const PublisherMetadataHandle &a, const PublisherMetadataHandle &b) noexcept
	{
		return a.mHandle == b.mHandle;
	}

	friend bool operator!=(const PublisherMetadataHandle &a, const PublisherMetadataHandle &b) noexcept
	{
		return a.mHandle != b.mHandle;
	}

	SysErr formatMessage(uint32_t messageID, uint32_t bufferSize, wchar_t * buffer, uint32_t *bufferUsed) const;
	SysErr formatMessage(EVT_HANDLE hEvent, uint32_t messageID, uint32_t flags, uint32_t bufferSize, wchar_t *buffer, uint32_t *bufferUsed) const;

private:
	constexpr explicit PublisherMetadataHandle(EVT_HANDLE h) noexcept : mHandle{h} {}

private:
	PublisherMetadataHandle(const PublisherMetadataHandle &) = delete;
	PublisherMetadataHandle &operator=(const PublisherMetadataHandle &) = delete;

};

class ObjectArrayPropertyHandle 
{
	EvtHandle mHandle; 
public:
	static ObjectArrayPropertyHandle getChannelReferences(const PublisherMetadataHandle &h);
	static ObjectArrayPropertyHandle getLevels(const PublisherMetadataHandle &h);
	static ObjectArrayPropertyHandle getTasks(const PublisherMetadataHandle &h);
	static ObjectArrayPropertyHandle getOpcodes(const PublisherMetadataHandle &h);
	static ObjectArrayPropertyHandle getKeywords(const PublisherMetadataHandle &h);
	
	constexpr ObjectArrayPropertyHandle() noexcept : mHandle{nullptr} {}
	constexpr explicit ObjectArrayPropertyHandle(std::nullptr_t) noexcept : mHandle {nullptr} {}
	~ObjectArrayPropertyHandle() = default;
	ObjectArrayPropertyHandle(ObjectArrayPropertyHandle &&rhs) = default;
	ObjectArrayPropertyHandle &operator=(ObjectArrayPropertyHandle &&rhs) = default;

	explicit operator bool() const noexcept
	{
		return !mHandle.isNull();
	}

	friend bool operator==(const ObjectArrayPropertyHandle &a, const ObjectArrayPropertyHandle &b)
	{
		return a.mHandle == b.mHandle;
	}

	friend bool operator!=(const ObjectArrayPropertyHandle &a, const ObjectArrayPropertyHandle &b)
	{
		return a.mHandle != b.mHandle;
	}

	EvtVariantPtr getProperty(uint32_t propertyId, uint32_t index, uint32_t size = 512u, uint32_t *actualSize = nullptr) const;

	void getProperty(uint32_t propertyId, uint32_t index, EVT_VARIANT &v) const;

	// returns the size of the array.
	// throws SystemError
	uint32_t getSize() const;

	// Retrieves the sie of the array. Does not throw.
	// size is pointer to variable to receive the size.
	// returns ERROR_SUCCESS on success, error code otherwise.
	SysErr getSize(uint32_t *size) const;

private:
	constexpr explicit ObjectArrayPropertyHandle(EVT_OBJECT_ARRAY_PROPERTY_HANDLE h) noexcept
		: mHandle{h}
	{}

private:
	// NO!
	ObjectArrayPropertyHandle(const ObjectArrayPropertyHandle &) = delete;
	ObjectArrayPropertyHandle &operator=(const ObjectArrayPropertyHandle &) = delete;
};

// Handle returned by EvtOpenLog
class LogHandle
{
	EvtHandle mHandle;
public:
	static LogHandle openChannel(const std::string &channelPath);
	static LogHandle openFile(const std::string &filePath);

	constexpr LogHandle() noexcept : mHandle{nullptr} {}
	constexpr explicit LogHandle(std::nullptr_t) noexcept : mHandle{nullptr} {}

	~LogHandle() = default;
	LogHandle(LogHandle &&rhs) = default;
	LogHandle &operator=(LogHandle &&rhs) = default;

	explicit operator bool() const noexcept 
	{
		return !mHandle.isNull();
	}

	friend bool operator==(const LogHandle &a, const LogHandle &b)
	{
		return a.mHandle == b.mHandle;
	}

	friend bool operator!=(const LogHandle &a, const LogHandle &b)
	{
		return a.mHandle != b.mHandle;
	}

	void getProperty(EVT_LOG_PROPERTY_ID propertyId, EVT_VARIANT &v) const;

private:
	constexpr explicit LogHandle(EVT_HANDLE h) noexcept : mHandle{h} {}

	LogHandle(const LogHandle &) = delete;
	LogHandle &operator=(const LogHandle &) = delete;
};

// Handle returned byt EvtQuery
class QueryHandle
{
	EvtHandle mHandle;
public:
	
	// see EvtQuery()
	static QueryHandle query(const wchar_t *channel, const wchar_t *queryText, uint32_t flags);

	constexpr QueryHandle() noexcept : mHandle{nullptr} {}
	constexpr explicit QueryHandle(std::nullptr_t) noexcept : mHandle{nullptr} {}
	QueryHandle(QueryHandle &&rhs) = default;
	~QueryHandle() = default;

	QueryHandle &operator=(QueryHandle &&rhs) = default;
	
	explicit operator bool() const noexcept
	{
		return !mHandle.isNull();
	}

	friend bool operator==(const QueryHandle &a, const QueryHandle &b) noexcept
	{
		return a.mHandle == b.mHandle;
	}

	friend bool operator!=(const QueryHandle &a, const QueryHandle &b) noexcept
	{
		return a.mHandle != b.mHandle;
	}

	// See EvtNext
	// Returns true if handles are retrieved,false if no more items are available. 
	QueryNextStatus next(uint32_t eventSize, EVT_HANDLE *events, uint32_t timeout, 
		uint32_t flags, uint32_t *numberReturned);

	void seek(int64_t position, SeekOption flags);

	// TODO: void seek(EVT_HANDLE hBookmark, int64_t position);

	SysErr close()
	{
		return this->mHandle.close();
	}

	bool isNull() const noexcept
	{
		return this->mHandle.isNull();
	}

private:
	constexpr explicit QueryHandle(EVT_HANDLE h) noexcept : mHandle(h) {}
};

// Non-owning. Event record handle.  
class EventRecordHandle
{
	EVT_HANDLE mHandle;
public:

	constexpr EventRecordHandle() noexcept
		: mHandle{ nullptr } {}

	constexpr explicit EventRecordHandle(std::nullptr_t) noexcept
		: mHandle{ nullptr } {}

	constexpr explicit EventRecordHandle(EVT_HANDLE hEventRec) noexcept
		: mHandle{ hEventRec }
	{}

	EventRecordHandle(const EventRecordHandle &) = default;
	EventRecordHandle(EventRecordHandle &&) = default;
	~EventRecordHandle() = default;

	EventRecordHandle &operator=(const EventRecordHandle &) = default;
	EventRecordHandle &operator=(EventRecordHandle &&) = default;

	explicit operator bool() const noexcept
	{
		return mHandle != nullptr;
	}

	friend bool operator==(const EventRecordHandle &a, const EventRecordHandle &b)
	{
		return a.mHandle == b.mHandle;
	}

	friend bool operator!=(const EventRecordHandle &a, const EventRecordHandle &b)
	{
		return a.mHandle != b.mHandle;
	}

	operator EVT_HANDLE() const { return mHandle; }

};

}