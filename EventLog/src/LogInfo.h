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

#include "ILogInfo.h"

namespace Windows::EventLog
{

class LogInfoImpl;
class LogInfo : public ILogInfo
{
public:

	friend class RefObject<LogInfo>;

	static Ref<LogInfo> openChannel(const std::string &channelPath);
	static Ref<LogInfo> openFile(const std::string &path);

	~LogInfo();

	std::optional<Timestamp> getCreationTime() const;
	std::optional<Timestamp> getLastAccessTime() const;
	std::optional<Timestamp> getLastWriteTime() const;
	std::optional<uint64_t> getFileSize() const;
	std::optional<uint32_t> getAttributes() const;
	std::optional<uint64_t> getNumberOfLogRecords() const;
	std::optional<uint64_t> getOldestRecordNumber() const;
	std::optional<bool> isFull() const;

private:
	struct DoOpenChannel {};
	struct DoOpenFile {};

	LogInfo(const std::string &channelPath, const DoOpenChannel &);
	LogInfo(const std::string &filePath, const DoOpenFile &);

	std::unique_ptr<LogInfoImpl> d_ptr;

};

}