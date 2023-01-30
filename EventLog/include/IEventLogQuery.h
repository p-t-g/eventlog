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

#include "IEventRecord.h"

namespace Windows::EventLog 
{

// Query batch result.
class IQueryBatchResult : public IRefObject
{
public:
	static Ref<IQueryBatchResult> createEmpty();

	virtual ~IQueryBatchResult() = default;

	// Returns the status of the query. Check this to know how it turned out.
	virtual QueryNextStatus getStatus() const = 0;

	// Returns the number of record handles. Zero when no more.
	virtual uint32_t getCount() const = 0;

	// Returns the record handle at the given index.
	// Throws IndexOutOfBoundsException if index >= size. 
	virtual Ref<IEventRecord> getRecord(uint32_t index) const = 0;
};

// Event log query interface
class IEventLogQuery : public IRefObject
{
public:
	virtual ~IEventLogQuery() = default;

	// Query a channel with XPath
	virtual void queryChannelXPath(const std::string &channel, const std::string &queryXPath, Direction dir) = 0;

	// Query a file with XPath
	virtual void queryFileXPath(const std::string &filePath, const std::string queryXPath, Direction dir) = 0;

	// Query with structured XML
	virtual void queryStructuredXML(const std::string &structuredXML, Direction dir) = 0;

	// Fetches a batch of results from the query. 
	virtual Ref<IQueryBatchResult> getNextBatch(uint32_t batchSize, uint32_t timeout) = 0;

	virtual void seek(int64_t position, SeekOption whence) = 0;

	virtual void close() = 0;
};

}