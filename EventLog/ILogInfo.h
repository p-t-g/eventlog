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

#include <optional>
#include <string>
#include <memory>

#include "CommonTypes.h"
#include "RefObject.h"
#include "RefPtr.h"

namespace Windows::EventLog
{

class ILogInfo : public IRefObject
{
public:
	virtual ~ILogInfo() = default;

	static Ref<ILogInfo> openChannel(const std::string &channelPath);
	static Ref<ILogInfo> openFile(const std::string &path);

	virtual std::optional<Timestamp> getCreationTime() const = 0;
	virtual std::optional<Timestamp> getLastAccessTime() const = 0;
	virtual std::optional<Timestamp> getLastWriteTime() const = 0;
	virtual std::optional<uint64_t> getFileSize() const = 0;
	virtual std::optional<uint32_t> getAttributes() const = 0;
	virtual std::optional<uint64_t> getNumberOfLogRecords() const = 0;
	virtual std::optional<uint64_t> getOldestRecordNumber() const = 0;
	virtual std::optional<bool> isFull() const = 0;
};

}