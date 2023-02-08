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
#include "EvtHandle.h"

#include "CommonTypes.h"
#include "StringUtils.h"

namespace Windows::EventLog
{

std::string to_string(ChannelIsolation value)
{
	switch (value)
	{
	case ChannelIsolation::Application:
		return "Application";
	case ChannelIsolation::System:
		return "System";
	case ChannelIsolation::Custom:
		return "Custom";
	}
	return {};
}

std::string to_string(ChannelType value)
{
	switch (value)
	{
	case ChannelType::Admin:
		return "Admin";
	case ChannelType::Analytic:
		return "Analytic";
	case ChannelType::Debug:
		return "Debug";
	case ChannelType::Operational:
		return "Operational";
	}
	return {};
}

std::string to_string(ChannelClockType value)
{
	switch (value)
	{
	case ChannelClockType::QPC:
		return "QPC";
	case ChannelClockType::SystemTime:
		return "SystemTime";
	}
	return {};
}

std::string to_string(ChannelSIDType value)
{
	switch (value)
	{
	case ChannelSIDType::None:
		return "None";
	case ChannelSIDType::Publishing:
		return "Publishing";
	}
	return {};
}

// 
// EvtHandle
//

EvtHandle::EvtHandle(EvtHandle &&rhs) noexcept
	: mHandle(rhs.mHandle)
{ 
	rhs.mHandle = nullptr;
}

EvtHandle &EvtHandle::operator=(EvtHandle &&rhs) noexcept
{
	EvtHandle{std::move(rhs)}.swap(*this);
	return *this;
}

EvtHandle::~EvtHandle() noexcept
{
	this->close();
}

SysErr EvtHandle::close() noexcept
{
	SysErr err{};
	if (mHandle)
	{
		BOOL success = ::EvtClose(mHandle);
		if (!success)
			err = ::GetLastError();
		mHandle = nullptr;
	}
	return err;
}

EVT_HANDLE EvtHandle::handle() const noexcept
{
	return this->mHandle;
}

bool EvtHandle::isNull() const noexcept
{
	return this->mHandle == nullptr;
}

EvtHandle &EvtHandle::operator=(EVT_HANDLE h) noexcept
{
	EvtHandle{h}.swap(*this);
	return *this;
}

//
// ChannelEnumHandle 
//

ChannelEnumHandle ChannelEnumHandle::open()
{
	auto hChannelEnum = ::EvtOpenChannelEnum(nullptr, 0);
	if (!hChannelEnum)
	{
		THROW_(SystemException, ::GetLastError());
	}
	return ChannelEnumHandle(hChannelEnum);
}

std::optional<std::wstring> ChannelEnumHandle::nextChannelPath(const UseWideChar_t &) const
{
	DWORD length = 512u;
	std::wstring channel(size_t(length), 0);

	BOOL success = ::EvtNextChannelPath(mHandle.handle(), length, &channel[0], &length);
	if (success)
	{
		// Remove redundant null
		channel.resize(size_t{length} - 1u);
		return channel;
	}
	else
	{
		DWORD err = ::GetLastError();
		switch(err)
		{
		case ERROR_INSUFFICIENT_BUFFER:
			channel.resize(length);
			success = ::EvtNextChannelPath(mHandle.handle(), length, &channel[0], &length);
			if (success)
			{
				// Remove redundant null
				channel.resize(size_t{length} - 1u);
				return channel;
			}
			else
			{
				THROW_(SystemException, ::GetLastError());
			}
			break;
		case ERROR_NO_MORE_ITEMS:
			return {};
		default:
			THROW_(SystemException, err);
		}
	}
}

std::optional<std::string> ChannelEnumHandle::nextChannelPath() const
{
	auto value = nextChannelPath(UseWideChar_t{});
	if (value.has_value())
		return std::optional<std::string>(to_utf8(value.value()));
	return {};
}

//
// ChannelConfigHandle
//

ChannelConfigHandle ChannelConfigHandle::open(const std::string &channelPath)
{
	auto hChannelConfig = ::EvtOpenChannelConfig(nullptr, to_utf16(channelPath).c_str(), 0);
	if (!hChannelConfig)
	{
		THROW_(SystemException, ::GetLastError());
	}

	return ChannelConfigHandle(hChannelConfig);
}

void ChannelConfigHandle::setPropertyValue(EVT_CHANNEL_CONFIG_PROPERTY_ID Id, PEVT_VARIANT pv) const
{
	BOOL success = ::EvtSetChannelConfigProperty(handle(), Id, 0, pv);
	if (!success)
	{
		THROW_(SystemException, ::GetLastError());
	}
}

void ChannelConfigHandle::getPropertyValue(EVT_CHANNEL_CONFIG_PROPERTY_ID Id, PEVT_VARIANT pv, uint32_t size) const
{
	DWORD byteSize = size;
	BOOL success = ::EvtGetChannelConfigProperty(handle(), Id, 0, byteSize, pv, &byteSize);
	if (!success)
	{
		THROW_(SystemException, ::GetLastError());
	}
}

EvtVariantPtr ChannelConfigHandle::getPropertyValue(EVT_CHANNEL_CONFIG_PROPERTY_ID Id, 
	uint32_t dataSize, uint32_t *totalUsedSize) const
{
	DWORD size = sizeof(EVT_VARIANT) + dataSize;
	EvtVariantPtr ptr = allocEvtVariant(size);
	BOOL success = ::EvtGetChannelConfigProperty(handle(), Id, 0, size, ptr.get(), &size);
	if (success)
	{
		if (totalUsedSize)
			*totalUsedSize = size;
	}
	else
	{
		DWORD err = ::GetLastError();
		if (ERROR_INSUFFICIENT_BUFFER == err)
		{
			ptr = allocEvtVariant(size);
			success = ::EvtGetChannelConfigProperty(handle(), Id, 0, size, ptr.get(), &size);
			if (success)
			{
				if (totalUsedSize)
					*totalUsedSize = size;
			}
			else
			{
				THROW_(SystemException, ::GetLastError());
			}
		}
	}
	return ptr;
}

void ChannelConfigHandle::save() const
{
	BOOL success = ::EvtSaveChannelConfig(handle(), 0);
	if (!success)
	{
		THROW_(SystemException, ::GetLastError());
	}
}

//
// PublishEnumHandle
//

PublisherEnumHandle PublisherEnumHandle::open()
{
	auto hPublisherEnum = ::EvtOpenPublisherEnum(nullptr, 0);
	if (!hPublisherEnum)
	{
		THROW_(SystemException, ::GetLastError());
	}
	return PublisherEnumHandle(hPublisherEnum);
}

std::optional<std::wstring> PublisherEnumHandle::nextPublisherId(int) const
{
	DWORD size = 64u;

	std::unique_ptr<wchar_t[]> buffer = std::make_unique<wchar_t[]>(size);
	BOOL success = ::EvtNextPublisherId(mHandle.handle(), size, buffer.get(), &size);
	if (success)
	{
		return buffer.get();
	}
	else
	{
		DWORD err = ::GetLastError();
		switch (err)
		{
		case ERROR_INSUFFICIENT_BUFFER:
		{
			
			buffer = std::make_unique<wchar_t[]>(size);
			success = ::EvtNextPublisherId(mHandle.handle(), size, buffer.get(), &size);
			if (success)
			{
				return buffer.get();
			}
			else
			{
				THROW_(SystemException, ::GetLastError());
			}

		}
		case ERROR_NO_MORE_ITEMS:
			return {};
		default:
			THROW_(SystemException, err);
		}
	}
}

std::optional<std::string> PublisherEnumHandle::nextPublisherId() const
{
	auto publisher = nextPublisherId(0);
	if (publisher.has_value())
		return to_utf8(publisher.value());
	return {};
}

//
// PublisherMetadataHandle
// 

PublisherMetadataHandle PublisherMetadataHandle::openProvider(const std::string &publisherId)
{
	std::wstring id = to_utf16(publisherId);
	auto hPublisherMetadata = ::EvtOpenPublisherMetadata(nullptr, id.c_str(), nullptr, 0, 0);
	if (!hPublisherMetadata)
	{
		DWORD err = ::GetLastError();
		THROW_(SystemException, err);
	}

	return PublisherMetadataHandle(hPublisherMetadata);
}

PublisherMetadataHandle PublisherMetadataHandle::openArchiveFile(const std::string &publisherId, const std::string &logFilePath)
{
	auto hPublisherMetadata = ::EvtOpenPublisherMetadata(nullptr, to_utf16(publisherId).c_str(), 
		to_utf16(logFilePath).c_str(), 0, 0);
	if (!hPublisherMetadata)
	{
		THROW_(SystemException, ::GetLastError());
	}

	return PublisherMetadataHandle(hPublisherMetadata);
}

void PublisherMetadataHandle::getProperty(EVT_PUBLISHER_METADATA_PROPERTY_ID propId, EVT_VARIANT &v) const
{
	DWORD size = sizeof(EVT_VARIANT);
	BOOL success = ::EvtGetPublisherMetadataProperty(mHandle.handle(), propId, 0, size,
		&v, &size);
	if (!success)
	{
		DWORD err = ::GetLastError();
		THROW_(SystemException, err);
	}
}

EvtVariantPtr PublisherMetadataHandle::getProperty(EVT_PUBLISHER_METADATA_PROPERTY_ID propId, uint32_t size,
	uint32_t *actualSize) const
{
	EvtVariantPtr pVal = allocEvtVariant(size);

	BOOL success = ::EvtGetPublisherMetadataProperty(mHandle.handle(), propId, 0, size, pVal.get(), (DWORD*) &size);
	if (success)
	{
		if (actualSize)
			*actualSize = size;
	}
	else
	{
		DWORD err = ::GetLastError();
		if (err == ERROR_INSUFFICIENT_BUFFER)
		{
			// Resize.
			pVal = allocEvtVariant(size);
			success = ::EvtGetPublisherMetadataProperty(mHandle.handle(), propId, 0, size, pVal.get(), (DWORD*) &size);
			if (success)
			{
				if (actualSize)
					*actualSize = size;
			}
			else
			{
				THROW_(SystemException, ::GetLastError());
			}
		}
		else
		{
			THROW_(SystemException, ::GetLastError());
		}
	}
	return pVal;
}

EventMetadataEnumHandle PublisherMetadataHandle::openEventMetadataEnum() const
{
	EVT_HANDLE h = ::EvtOpenEventMetadataEnum(mHandle.handle(), 0);
	if (!h)
	{
		THROW_(SystemException, ::GetLastError());
	}
	return EventMetadataEnumHandle(h);
}

DWORD PublisherMetadataHandle::formatMessage(uint32_t messageID, uint32_t bufferSize, wchar_t *buffer, uint32_t *bufferUsed) const
{
	DWORD err = ERROR_SUCCESS;
	EVT_HANDLE hMetadata = this->mHandle.handle();
	DWORD bufUsed = 0;
	BOOL success = ::EvtFormatMessage(hMetadata, nullptr, messageID, 0, nullptr, EvtFormatMessageId, bufferSize, buffer, &bufUsed);
	if (!success)
	{ 
		err = ::GetLastError();
		*bufferUsed = bufUsed;
	}
	return err;
}

DWORD PublisherMetadataHandle::formatMessage(EVT_HANDLE hEvent, uint32_t messageID, uint32_t flags, uint32_t bufferSize,
	wchar_t *buffer, uint32_t *bufferUsed) const
{
	DWORD err = ERROR_SUCCESS;
	EVT_HANDLE hMetadata = this->mHandle.handle();
	BOOL success = ::EvtFormatMessage(hMetadata, hEvent, messageID, 0, nullptr, flags, bufferSize, buffer, (DWORD*) bufferUsed);
	if (!success)
	{ 
		err = ::GetLastError();
	}
	return err;
}

//
// ObjectArrayPropertyHandle
//

ObjectArrayPropertyHandle ObjectArrayPropertyHandle::getChannelReferences(const PublisherMetadataHandle &h)
{
	EVT_VARIANT v{};
	h.getProperty(EvtPublisherMetadataChannelReferences, v);
	if (v.Type != EvtVarTypeEvtHandle)
	{
		THROW(InvalidDataTypeException);
	}
	return ObjectArrayPropertyHandle(v.EvtHandleVal);
}

ObjectArrayPropertyHandle ObjectArrayPropertyHandle::getLevels(const PublisherMetadataHandle &h)
{
	EVT_VARIANT v{};
	h.getProperty(EvtPublisherMetadataLevels, v);
	if (v.Type != EvtVarTypeEvtHandle)
	{
		THROW(InvalidDataTypeException);
	}
	return ObjectArrayPropertyHandle(v.EvtHandleVal);
}

ObjectArrayPropertyHandle ObjectArrayPropertyHandle::getTasks(const PublisherMetadataHandle &h)
{
	EVT_VARIANT v{};
	h.getProperty(EvtPublisherMetadataTasks, v);
	if (v.Type != EvtVarTypeEvtHandle)
	{
		THROW(InvalidDataTypeException);
	}
	return ObjectArrayPropertyHandle(v.EvtHandleVal);
}

ObjectArrayPropertyHandle ObjectArrayPropertyHandle::getOpcodes(const PublisherMetadataHandle &h)
{
	EVT_VARIANT v{};
	h.getProperty(EvtPublisherMetadataOpcodes, v);
	if (v.Type != EvtVarTypeEvtHandle)
	{
		THROW(InvalidDataTypeException);
	}
	return ObjectArrayPropertyHandle(v.EvtHandleVal);
}

ObjectArrayPropertyHandle ObjectArrayPropertyHandle::getKeywords(const PublisherMetadataHandle &h)
{
	EVT_VARIANT v{};
	h.getProperty(EvtPublisherMetadataKeywords, v);
	if (v.Type != EvtVarTypeEvtHandle)
	{
		THROW(InvalidDataTypeException);
	}
	return ObjectArrayPropertyHandle(v.EvtHandleVal);
}

void ObjectArrayPropertyHandle::getProperty(uint32_t propertyId, uint32_t index, EVT_VARIANT &v) const
{
	DWORD size = sizeof(EVT_VARIANT);
	BOOL success = ::EvtGetObjectArrayProperty(mHandle.handle(), propertyId, index, 0,  size, &v, &size);
	if (!success)
	{
		DWORD err = ::GetLastError();
		THROW_(SystemException, err);
	}
}

EvtVariantPtr ObjectArrayPropertyHandle::getProperty(uint32_t propertyId, uint32_t index, uint32_t size, uint32_t *actualSize) const
{
	EvtVariantPtr pv = allocEvtVariant(size);
	BOOL success = ::EvtGetObjectArrayProperty(mHandle.handle(), propertyId, index, 0, size, pv.get(), (PDWORD) & size);
	if (success)
	{
		if (actualSize)
			*actualSize = size;
	}
	else
	{
		DWORD err = ::GetLastError();
		if (ERROR_INSUFFICIENT_BUFFER == err)
		{
			pv = allocEvtVariant(size);
			success = ::EvtGetObjectArrayProperty(mHandle.handle(), propertyId, index, 0, size, pv.get(), (PDWORD) &size);
			if (success)
			{
				if (actualSize)
					*actualSize = size;
			}
			else
			{
				THROW_(SystemException, ::GetLastError());
			}
		}
		else
		{
			THROW_(SystemException, err);
		}
	}
	return pv;
}

uint32_t ObjectArrayPropertyHandle::getSize() const
{
	DWORD size = 0;
	BOOL success = ::EvtGetObjectArraySize(mHandle.handle(), &size);
	if (!success)
	{
		DWORD err = ::GetLastError();
		// This seems to happen with misconfiguration.
		// Just return 0 as there is nothing we can do and
		// throwing is pointless.
		if (err == ERROR_FILE_NOT_FOUND)
		{
			return 0;
		}
		THROW_(SystemException, err);
	}
	return size;
}

SysErr ObjectArrayPropertyHandle::getSize(uint32_t *size) const
{
	if (!size) 
		return ERROR_INVALID_PARAMETER;

	SysErr err{ERROR_SUCCESS};
	DWORD arraySize = 0;
	BOOL success = ::EvtGetObjectArraySize(mHandle.handle(), &arraySize);
	if (success)
		*size =  arraySize;
	else
		err = ::GetLastError();

	return err;
}

//
// LogHandle
//

LogHandle LogHandle::openChannel(const std::string &channelPath)
{
	auto hLog = ::EvtOpenLog(nullptr, to_utf16(channelPath).c_str(), EvtOpenChannelPath);
	if (!hLog)
	{
		THROW_(SystemException, ::GetLastError());
	}
	return LogHandle(hLog);
}

LogHandle LogHandle::openFile(const std::string &filePath)
{
	auto hLog = ::EvtOpenLog(nullptr, to_utf16(filePath).c_str(), EvtOpenFilePath);
	if (!hLog)
	{
		THROW_(SystemException, ::GetLastError());
	}
	return LogHandle(hLog);
}

void LogHandle::getProperty(EVT_LOG_PROPERTY_ID propertyId, EVT_VARIANT &v) const
{
	DWORD size = sizeof(EVT_VARIANT);
	BOOL success = ::EvtGetLogInfo(mHandle.handle(), propertyId, size, &v, &size);
	if (!success)
	{
		THROW_(SystemException, ::GetLastError());
	}
}

//
// QueryHandle
//

QueryHandle QueryHandle::query(const wchar_t *channel, const wchar_t *queryText, uint32_t flags)
{
	EVT_HANDLE h = ::EvtQuery(nullptr, channel, queryText, flags);
	if (!h)
	{
		DWORD err = ::GetLastError();
		THROW_(SystemException, err);
	}

	return QueryHandle(h);
}

QueryNextStatus QueryHandle::next(uint32_t eventSize, EVT_HANDLE *events, uint32_t timeout, uint32_t flags, uint32_t *numberReturned)
{
	DWORD count = 0;
	BOOL success = ::EvtNext(mHandle.handle(), eventSize, events, timeout, flags, &count);
	if (!success)
	{
		DWORD err = ::GetLastError();
		switch (err)
		{
		case ERROR_NO_MORE_ITEMS:
			return QueryNextStatus::NoMoreItems;
		case ERROR_TIMEOUT:
			return QueryNextStatus::Timeout;
		default:
			THROW_(SystemException, err);
		}				
	}
	*numberReturned = count;
	return QueryNextStatus::Success;
}

static inline DWORD to_EvtSeekFlag(SeekOption option)
{
	switch (option)
	{
	case SeekOption::RelativeToFirst:
		return EvtSeekRelativeToFirst;
	case SeekOption::RelativeToLast:
		return EvtSeekRelativeToLast;
	case SeekOption::RelativeToCurrent:
		return EvtSeekRelativeToCurrent;
	default:
		// never happens
		return 0;
	}
}

void QueryHandle::seek(int64_t position, SeekOption whence)
{
	DWORD flags = to_EvtSeekFlag(whence);
	BOOL success = ::EvtSeek(mHandle.handle(), position, nullptr, 0, flags);
	if (!success)
	{
		DWORD err = ::GetLastError();
		THROW_(SystemException, err);
	}
}


//
// EventMetadataHandle
//

void EventMetadataHandle::getProperty(EVT_EVENT_METADATA_PROPERTY_ID propertyId, EVT_VARIANT &v) const
{
	uint32_t size = sizeof(EVT_VARIANT);
	BOOL success = ::EvtGetEventMetadataProperty(mEventMetadataHandle.handle(), propertyId, 0, size, &v, (PDWORD) & size);
	if (!success)
	{
		DWORD err = ::GetLastError();
		THROW_(SystemException, err);
	}
}

EvtVariantPtr EventMetadataHandle::getProperty(EVT_EVENT_METADATA_PROPERTY_ID propertyId, uint32_t size, uint32_t *actualSize) const
{
	EvtVariantPtr value = allocEvtVariant(size);
	BOOL success = ::EvtGetEventMetadataProperty(mEventMetadataHandle.handle(), propertyId, 0, size, value.get(), (PDWORD)&size);
	if (!success)
	{
		DWORD err = ::GetLastError();
		if (ERROR_INSUFFICIENT_BUFFER == err)
		{
			value = allocEvtVariant(size);
			success = ::EvtGetEventMetadataProperty(mEventMetadataHandle.handle(), propertyId, 0, size, value.get(), (PDWORD)  & size);
			if (!success)
			{
				err = ::GetLastError();
				THROW_(SystemException, err);
			}
		}
		else
		{
			THROW_(SystemException, err);
		}
	}

	if (actualSize)
		*actualSize = size;
	return value;
}

///////////////////////////////////////////////////////////////////////////////
// 

EventMetadataHandle EventMetadataEnumHandle::next() const
{
	EVT_HANDLE hEventMetadata = ::EvtNextEventMetadata(mhEventMetadataEnum.handle(), 0);
	if (!hEventMetadata)
	{
		DWORD err = ::GetLastError();
		if (ERROR_NO_MORE_ITEMS == err)
		{
			return EventMetadataHandle{nullptr};
		}
		else
		{
			THROW_(SystemException, err);
		}
	}
	return EventMetadataHandle(hEventMetadata);
}

}