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

#include <memory>
#include <optional>

namespace Windows::EventLog
{

class IChannelConfig : public IRefObject
{
public:

	// Use this to create an instance. 
	static Ref<IChannelConfig> create(const std::string &channel);

	~IChannelConfig() = default;

	virtual bool getConfigEnabled() const = 0;
	virtual void setConfigEnabled(bool isEnabled) = 0;

	virtual ChannelIsolation getConfigIsolation() const = 0;

	virtual ChannelType getConfigType() const = 0;

	virtual std::string getConfigOwningPublisher() const = 0;

	virtual bool getConfigClassicEventLog() const = 0;

	virtual std::string getConfigAccess() const = 0;
	virtual void setConfigAccess(const std::string &access) = 0;

	virtual bool getLoggingConfigRetention() const = 0;
	virtual void setLoggingConfigRetention(bool retention) = 0;

	/*
	Determines whether to create a new log file when the current log file 
	reaches its maximum size. Set to true to request that the service create
	a new file when the log file reaches its maximum size; otherwise, false.
	You can set autoBackup to true only if retention is set to true. The 
	default is false. There is no limit to the number of backup files that 
	the service can create (limited only by available disk space). The backup
	file names are of the form Archive-channelName-timestamp.evtx and are 
	located in %windir%\System32\winevt\Logs folder.
	*/
	virtual bool getLoggingConfigAutoBackup() const = 0;
	virtual void setLoggingConfigAutoBackup(bool autobackup) = 0;

	/*
	The maximum size, in bytes, of the log file. The default (and minimum) 
	value is 1 MB. If the physical log size is less than the configured
	maximum size and additional space is required in the log to store events,
	the service will allocate another block of space even if the resulting 
	physical size of the log will be larger than the configured maximum size. 
	The service allocates blocks of 1 MB so the physical size could grow to up
	to 1 MB larger than the configured max size.
	*/
	virtual uint64_t getLoggingConfigMaxSize() const = 0;
	virtual void setLoggingConfigMaxSize(uint64_t) = 0;

	virtual std::string getLoggingConfigLogFilePath() const = 0;
	virtual void setLoggingConfigLogFilePath(const std::string &path) = 0;

	virtual std::optional<uint32_t> getPublishingConfigLevel() const = 0;
	// Disable the debug or analytic channel first
	virtual void setPublishingConfigLevel(std::optional<uint32_t> value) = 0;

	virtual std::optional<uint64_t> getPublishingConfigKeywords() const = 0;
	// Disable the debug or analytic channel first
	virtual void setPublishingConfigKeywords(std::optional<uint64_t> value) = 0;

	virtual std::optional<GUID> getPublishingConfigControlGuid() const = 0;

	virtual std::optional<uint32_t> getPublishingConfigBufferSize() const = 0;

	virtual std::optional<uint32_t> getPublishingConfigMinBuffers() const = 0;

	virtual std::optional<uint32_t> getPublishingConfigMaxBuffers() const = 0;

	virtual std::optional<uint32_t> getPublishingConfigLatency() const = 0;

	virtual std::optional<ChannelClockType> getPublishingConfigClockType() const = 0;

	virtual std::optional<ChannelSIDType> getPublishingConfigSidType() const = 0;

	virtual std::vector<std::string> getPublisherList() const = 0;

	virtual std::optional<uint32_t> getPublishingConfigFileMax() const = 0;
	virtual void setPublishingConfigFileMax(std::optional<uint32_t> value) = 0;

	virtual void save() = 0;
};

}