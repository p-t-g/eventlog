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

#include "EventRecord.h"

#include "EvtHandle.h"
#include "EvtVariant.h"
#include "PublisherMetadata.h"
#include "WinSys.h"

namespace Windows::EventLog
{

template<typename U>
using CMemArrayPtr = std::unique_ptr<U [], CMemFree>;

using EvtVariantArrayPtr = CMemArrayPtr<EVT_VARIANT>;

template<typename T> CMemArrayPtr<T> allocCMemArray(size_t byteSize)
{
	auto p = ::malloc(byteSize);
	if (!p) throw std::bad_alloc();
	return CMemArrayPtr<T>(static_cast<T*>(p));
}

static EvtVariantArrayPtr allocEvtVariantArray(size_t byteSize)
{
	return allocCMemArray<EVT_VARIANT>(byteSize);
}


//
// RenderContext
//

class RenderContext
{
	EVT_HANDLE mRenderCtx;
public:
	constexpr RenderContext() noexcept : mRenderCtx{nullptr} {}

	RenderContext(DWORD ValuePathCount, LPCWSTR *ValuePaths, DWORD Flags)
		: mRenderCtx(::EvtCreateRenderContext(ValuePathCount, ValuePaths, Flags))
	{
		if (!mRenderCtx)
		{
			THROW_(SystemError, ::GetLastError());
		}
	}

	RenderContext(RenderContext &&rhs) noexcept
		: mRenderCtx(rhs.mRenderCtx)
	{
		rhs.mRenderCtx = nullptr;
	}

	RenderContext &operator=(RenderContext &&rhs) noexcept
	{
		this->mRenderCtx = rhs.mRenderCtx;
		rhs.mRenderCtx = nullptr;
		return *this;
	}

	~RenderContext()
	{
		if (mRenderCtx)
		{ 
			::EvtClose(mRenderCtx);
			mRenderCtx = nullptr;
		}
	}

	operator EVT_HANDLE() const noexcept { return mRenderCtx; }

	EVT_HANDLE handle() const { return mRenderCtx; }
private:
};

// Null avoidance sentinel.
class EmptyEventRecord : public IEventRecord
{
public:
	static Ref<EmptyEventRecord> create() { return RefObject<EmptyEventRecord>::createRef(); }
	std::optional<std::string> getProviderName() const override { return {}; }
	std::optional<GUID> getProviderGuid() const override { return {}; }
	std::optional<uint16_t> getEventId() const override { return {}; }
	std::optional<uint16_t> getQualifers() const override { return {}; }
	std::optional<uint8_t> getLevel() const override { return {}; }
	std::optional<uint16_t> getTask() const override { return {}; }
	std::optional<uint8_t> getOpcode() const override { return {}; }
	std::optional<int64_t> getKeywords() const override { return {}; }
	std::optional<Timestamp> getTimeCreated() const override { return {}; }
	std::optional<uint64_t> getRecordId() const override { return {}; }
	std::optional<GUID> getActivityId() const override { return {}; }
	std::optional<uint32_t> getProcessId() const override { return {}; }
	std::optional<uint32_t> getThreadId() const override { return {}; }
	std::optional<std::string> getChannel() const override { return {}; }
	std::optional<std::string> getComputer() const override { return {}; }
	std::optional<std::string> getUser() const override { return {}; }
	std::optional<uint8_t> getVersion() const override { return {}; }
	std::string getMessage() const override { return {}; }
	std::string getLevelDisplay() const override { return {}; }
	std::string getTaskDisplay() const override { return {}; }
	std::string getOpcodeDisplay() const override { return {}; }
	std::vector<std::string> getKeywordsDisplay() const override { return {}; }
	std::string getChannelMessage() const override { return {}; }
	std::string getProviderMessage() const override { return {}; }
};

static EVT_HANDLE getDefaultSystemRenderContext()
{
	static RenderContext context{0, nullptr, EvtRenderContextSystem};
	return context.handle();
}

static std::optional<std::string> getUserFromSID(const EVT_VARIANT &pUser)
{
	if (pUser.Type == EvtVarTypeNull)
	{
		return {};
	}
	else if (pUser.Type == EvtVarTypeSid)
	{
		return lookupAccount(pUser.SidVal);
	}
	else
	{
		THROW(InvalidDataTypeException);
	}
}

//
// EventRecord implementation
//

EventRecord::EventRecord(const EventRecordHandle &hRecord)
{
	DWORD propertyCount = 0;
	DWORD size = 1024;
	EvtVariantArrayPtr va = allocEvtVariantArray(size);

	//
	// First, 'render' the system values.
	//

	BOOL success = ::EvtRender(getDefaultSystemRenderContext(), hRecord, EvtRenderEventValues, size, va.get(), &size, &propertyCount);
	if (!success)
	{
		DWORD err = ::GetLastError();
		if (err == ERROR_INSUFFICIENT_BUFFER)
		{
			va = allocEvtVariantArray(size);

			success = ::EvtRender(getDefaultSystemRenderContext(), hRecord, EvtRenderEventValues, size, va.get(), &size, &propertyCount);
			if (!success)
			{
				err = ::GetLastError();
				THROW_(SystemError, err);
			}
		}
		else
		{
			THROW_(SystemError, err);
		}
	}

	mProviderName = Variant::getMaybeString(va[EvtSystemProviderName]);
	mProviderGuid = Variant::getMaybeGuid(va[EvtSystemProviderGuid]);
	mEventId = Variant::getMaybeUInt16(va[EvtSystemEventID]);
	mQualifers = Variant::getMaybeUInt16(va[EvtSystemQualifiers]);
	mLevel = Variant::getMaybeByte(va[EvtSystemLevel]);
	mTask = Variant::getMaybeUInt16(va[EvtSystemTask]);
	mOpcode = Variant::getMaybeByte(va[EvtSystemOpcode]);

	// Keywords is weird. No idea why it uses this type code.
	if (va[EvtSystemKeywords].Type == EvtVarTypeHexInt64 ||
		va[EvtSystemKeywords].Type == EvtVarTypeInt64 ||
		va[EvtSystemKeywords].Type == EvtVarTypeUInt64)
	{
		// Docs say chop top 16. 
		va[EvtSystemKeywords].UInt64Val &= 0x0000FFFFFFFFFFFF;
		mKeywords = va[EvtSystemKeywords].Int64Val;
	}
	// else: leave it empty?

	mTimeCreated = Variant::getMaybeTimestamp(va[EvtSystemTimeCreated]);
	mRecordId = Variant::getMaybeUInt64(va[EvtSystemEventRecordId]);
	mActivityId = Variant::getMaybeGuid(va[EvtSystemActivityID]);
	mRelatedActivityId = Variant::getMaybeGuid(va[EvtSystemRelatedActivityID]);
	mProcessId = Variant::getMaybeUInt32(va[EvtSystemProcessID]);
	mThreadId = Variant::getMaybeUInt32(va[EvtSystemThreadID]);
	mChannel = Variant::getMaybeString(va[EvtSystemChannel]);
	mComputer = Variant::getMaybeString(va[EvtSystemComputer]);

	mUser = getUserFromSID(va[EvtSystemUserID]);
	mVersion = Variant::getMaybeByte(va[EvtSystemVersion]);

	//
	// Now format the human readable messages.
	// 

	if (mProviderName.has_value())
	{
		const std::string &providerName = mProviderName.value();
		// If the publisher isn't found try to format without.
		RefPtr<PublisherMetadata> publisher = PublisherMetadata::cacheOpenProvider(providerName);
		if (publisher)
		{
			mRecord = publisher->format(hRecord);
		}
		else
		{
			mRecord = PublisherMetadata::formatEvent(hRecord);
		}
	}
	else
	{
		// Try to format without the provider.
		mRecord = PublisherMetadata::formatEvent(hRecord);
	}
}

Ref<EventRecord> EventRecord::create(const EventRecordHandle &hRecord)
{
	return RefObject<EventRecord>::createRef(hRecord);
}

std::optional<std::string> EventRecord::getProviderName() const
{
	return mProviderName;
}

std::optional<GUID> EventRecord::getProviderGuid() const
{
	return mProviderGuid;
}

std::optional<uint16_t> EventRecord::getEventId() const
{
	return mEventId;
}

std::optional<uint16_t> EventRecord::getQualifers() const
{
	return mQualifers;
}

std::optional<uint8_t> EventRecord::getLevel() const
{
	return mLevel;
}

std::optional<uint16_t> EventRecord::getTask() const
{
	return mTask;
}

std::optional<uint8_t> EventRecord::getOpcode() const
{
	return mOpcode;
}

std::optional<int64_t> EventRecord::getKeywords() const
{
	return mKeywords;
}

std::optional<Timestamp> EventRecord::getTimeCreated() const
{
	return mTimeCreated;
}

std::optional<uint64_t> EventRecord::getRecordId() const
{
	return mRecordId;
}

std::optional<GUID> EventRecord::getActivityId() const
{
	return mActivityId;
}

std::optional<uint32_t> EventRecord::getProcessId() const
{
	return mProcessId;
}

std::optional<uint32_t> EventRecord::getThreadId() const
{
	return mThreadId;
}

std::optional<std::string> EventRecord::getChannel() const
{
	return mChannel;
}

std::optional<std::string> EventRecord::getComputer() const
{
	return mComputer;
}

std::optional<std::string> EventRecord::getUser() const
{
	return {};
}

std::optional<uint8_t> EventRecord::getVersion() const
{
	return mVersion;
}

std::string EventRecord::getMessage() const
{
	return mRecord.message;
}

std::string EventRecord::getLevelDisplay() const
{
	return mRecord.level;
}

std::string EventRecord::getTaskDisplay() const
{
	return mRecord.task;
}

std::string EventRecord::getOpcodeDisplay() const 
{
	return mRecord.opcode;
}

std::vector<std::string> EventRecord::getKeywordsDisplay() const
{
	return mRecord.keywords;
}

std::string EventRecord::getChannelMessage() const
{
	return mRecord.channelMessage;
}

std::string EventRecord::getProviderMessage() const 
{
	return mRecord.providerMessage;
}

Ref<IEventRecord> IEventRecord::createEmpty()
{
	return EmptyEventRecord::create();
}

}