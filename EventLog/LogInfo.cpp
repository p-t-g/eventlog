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

#include "LogInfo.h"

#include "EvtHandle.h"
#include "EvtVariant.h"

namespace Windows::EventLog
{

static 
std::optional<Timestamp> getLogInfoPropertyTimestamp(const LogHandle &hLog, EVT_LOG_PROPERTY_ID propertyId)
{
	EVT_VARIANT v{};
	hLog.getProperty(propertyId, v);
	if (v.Type == EvtVarTypeNull)
		return {};
	return { { Variant::getRawFileTime(v) } };
}

static
std::optional<uint64_t> getLogInfoPropertyUInt64(const LogHandle &hLog, EVT_LOG_PROPERTY_ID propertyId)
{
	EVT_VARIANT v{};
	hLog.getProperty(propertyId, v);
	if (v.Type == EvtVarTypeNull)
		return {};
	return { Variant::getUInt64(v) };
}

static
std::optional<uint32_t> getLogInfoPropertyUInt32(const LogHandle &hLog, EVT_LOG_PROPERTY_ID propertyId)
{
	EVT_VARIANT v{};
	hLog.getProperty(propertyId, v);
	if (v.Type == EvtVarTypeNull)
		return {};
	return { Variant::getUInt32(v) };
}

static
std::optional<bool> getLogInfoPropertyBool(const LogHandle &hLog, EVT_LOG_PROPERTY_ID propertyId)
{
	EVT_VARIANT v{};
	hLog.getProperty(propertyId, v);
	if (v.Type == EvtVarTypeNull)
		return {};
	return { Variant::getBool(v) };
}

class LogInfoImpl
{
public:
	std::optional<Timestamp> mCreationTime{};
	std::optional<Timestamp> mLastAccessTime{};
	std::optional<Timestamp> mLastWriteTime{};
	std::optional<uint64_t> mFileSize{};
	std::optional<uint32_t> mAttributes{};
	std::optional<uint64_t> mNumberOfLogRecords{};
	std::optional<uint64_t> mOldestRecordNumber{};
	std::optional<bool> mFull{};

	LogInfoImpl(const LogHandle &hLog)
		: mCreationTime(getLogInfoPropertyTimestamp(hLog, EvtLogCreationTime))
		, mLastAccessTime(getLogInfoPropertyTimestamp(hLog, EvtLogLastAccessTime)) 
		, mLastWriteTime(getLogInfoPropertyTimestamp(hLog, EvtLogLastWriteTime))
		, mFileSize(getLogInfoPropertyUInt64(hLog, EvtLogFileSize))
		, mAttributes(getLogInfoPropertyUInt32(hLog, EvtLogAttributes))
		, mNumberOfLogRecords(getLogInfoPropertyUInt64(hLog, EvtLogNumberOfLogRecords))
		, mOldestRecordNumber(getLogInfoPropertyUInt64(hLog, EvtLogOldestRecordNumber))
		, mFull(getLogInfoPropertyBool(hLog, EvtLogFull))
	{
	}
};

LogInfo::LogInfo(const std::string &channelPath, const DoOpenChannel &)
	: d_ptr(std::make_unique<LogInfoImpl>(LogHandle::openChannel(channelPath)))
{}

LogInfo::LogInfo(const std::string &filePath, const DoOpenFile &)
	: d_ptr(std::make_unique<LogInfoImpl>(LogHandle::openFile(filePath)))
{}

LogInfo::~LogInfo()
{}

Ref<LogInfo> LogInfo::openFile(const std::string &filePath)
{
	return RefObject<LogInfo>::createRef(filePath, DoOpenFile{});
}

Ref<LogInfo> LogInfo::openChannel(const std::string &channelPath)
{
	return RefObject<LogInfo>::createRef(channelPath, DoOpenChannel{});
}

std::optional<Timestamp> LogInfo::getCreationTime() const
{
	return d_ptr->mCreationTime;
}

std::optional<Timestamp> LogInfo::getLastAccessTime() const
{
	return d_ptr->mLastAccessTime;
} 

std::optional<Timestamp> LogInfo::getLastWriteTime() const
{
	return d_ptr->mLastWriteTime;
}

std::optional<uint64_t> LogInfo::getFileSize() const
{
	return d_ptr->mFileSize;
}

std::optional<uint32_t> LogInfo::getAttributes() const
{
	return d_ptr->mAttributes;
}

std::optional<uint64_t> LogInfo::getNumberOfLogRecords() const
{
	return d_ptr->mNumberOfLogRecords;
}

std::optional<uint64_t> LogInfo::getOldestRecordNumber() const
{
	return d_ptr->mOldestRecordNumber;
}

std::optional<bool> LogInfo::isFull() const
{
	return d_ptr->mFull;
}

Ref<ILogInfo> ILogInfo::openChannel(const std::string &channelPath)
{
	return LogInfo::openChannel(channelPath);
}

Ref<ILogInfo> ILogInfo::openFile(const std::string &path)
{
	return LogInfo::openFile(path);
}

}