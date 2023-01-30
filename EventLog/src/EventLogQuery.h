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

#include "IEventLogQuery.h"

#include <memory>

namespace Windows::EventLog 
{

// Event log query implementation. 
class EventLogQueryImpl;
class EventLogQuery : public IEventLogQuery
{
public:
	friend class RefObject<EventLogQuery>;

	static Ref<IEventLogQuery> create();

	~EventLogQuery();

	void queryChannelXPath(const std::string &channel, const std::string &queryXPath, Direction dir);

	void queryFileXPath(const std::string &filePath, const std::string queryXPath, Direction dir);

	void queryStructuredXML(const std::string &structuredXML, Direction dir);

	Ref<IQueryBatchResult> getNextBatch(uint32_t batchSize, uint32_t timeout) override;

	void seek(int64_t position, SeekOption flags) override;

	void close() override;

private:
	std::unique_ptr<EventLogQueryImpl> d_ptr;

	EventLogQuery();

	EventLogQuery(const EventLogQuery &) = delete;
	EventLogQuery &operator=(const EventLogQuery &) = delete;

};

} // namespace Windows::EventLog
