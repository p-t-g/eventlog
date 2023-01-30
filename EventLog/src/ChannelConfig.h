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

#include "IChannelConfig.h"

#include <memory>

namespace Windows::EventLog
{

class ChannelConfigImpl;
class ChannelConfig : public IChannelConfig
{
public:
	friend class RefObject<ChannelConfig>;

	static Ref<IChannelConfig> create(const std::string &path);

	~ChannelConfig();

	bool getConfigEnabled() const override;
	void setConfigEnabled(bool isEnabled) override;

	ChannelIsolation getConfigIsolation() const override;

	ChannelType getConfigType() const override;

	std::string getConfigOwningPublisher() const override;

	bool getConfigClassicEventLog() const override;

	std::string getConfigAccess() const override;
	void setConfigAccess(const std::string &access) override;

	bool getLoggingConfigRetention() const override;
	void setLoggingConfigRetention(bool retention) override;

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
	bool getLoggingConfigAutoBackup() const override;
	void setLoggingConfigAutoBackup(bool autobackup) override;

	/*
	The maximum size, in bytes, of the log file. The default (and minimum) 
	value is 1 MB. If the physical log size is less than the configured
	maximum size and additional space is required in the log to store events,
	the service will allocate another block of space even if the resulting 
	physical size of the log will be larger than the configured maximum size. 
	The service allocates blocks of 1 MB so the physical size could grow to up
	to 1 MB larger than the configured max size.
	*/
	uint64_t getLoggingConfigMaxSize() const override;
	void setLoggingConfigMaxSize(uint64_t) override;

	std::string getLoggingConfigLogFilePath() const override;
	void setLoggingConfigLogFilePath(const std::string &path) override;

	std::optional<uint32_t> getPublishingConfigLevel() const override;
	// Disable the debug or analytic channel first
	void setPublishingConfigLevel(std::optional<uint32_t> value) override;

	std::optional<uint64_t> getPublishingConfigKeywords() const override;
	// Disable the debug or analytic channel first
	void setPublishingConfigKeywords(std::optional<uint64_t> value) override;

	std::optional<GUID> getPublishingConfigControlGuid() const override;

	std::optional<uint32_t> getPublishingConfigBufferSize() const override;

	std::optional<uint32_t> getPublishingConfigMinBuffers() const override;

	std::optional<uint32_t> getPublishingConfigMaxBuffers() const override;

	std::optional<uint32_t> getPublishingConfigLatency() const override;

	std::optional<ChannelClockType> getPublishingConfigClockType() const override;

	std::optional<ChannelSIDType> getPublishingConfigSidType() const override;

	std::vector<std::string> getPublisherList() const override;

	std::optional<uint32_t> getPublishingConfigFileMax() const override;
	void setPublishingConfigFileMax(std::optional<uint32_t> value) override;

	void save() override;

private:
	std::unique_ptr<ChannelConfigImpl> d_ptr;

	explicit ChannelConfig(const std::string &path);

private:
	ChannelConfig(const ChannelConfig &) = delete;
	ChannelConfig &operator=(const ChannelConfig &) = delete;
};

}