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

#include "EventReader.h"

#include "EventLogQuery.h"
#include "EventRecord.h"
#include "PublisherMetadata.h"

// TODO: make this configurable? 
static constexpr uint32_t BatchSize = 16;

namespace Windows::EventLog
{

class EventReaderImpl
{
public:
	struct ChannelReader {};
	EventReaderImpl(const ChannelReader &, const std::string &channel,
		const std::string &queryText, Direction direction);

	struct FileReader {};
	EventReaderImpl(const FileReader &, const std::string &filePath, 
		const std::string &queryText, Direction direction);

	EventReaderImpl(const std::string &queryText, Direction direction);

	uint32_t getTimeout() const { return mTimeout; }	
	void setTimeout(uint32_t timeout) { mTimeout = timeout; }
	
	bool next();
	
	Ref<IEventRecord> getCurrent() const { return mCurrentRecord; }

	void seek(int64_t position, SeekOption option);

private:
	Ref<IEventLogQuery> mQuery;
	Ref<IQueryBatchResult> mQueryBatch;

	uint32_t mCurrent = 0;
	uint32_t mEventCount = 0;
	uint32_t mTimeout = UINT32_MAX;

	Ref<IEventRecord> mCurrentRecord{IEventRecord::createEmpty()};
};

//
// EventReaderImpl
//

EventReaderImpl::EventReaderImpl(const ChannelReader &, const std::string &channelPath, const std::string &queryText, Direction direction)
	: mQuery{ EventLogQuery::create() }
	, mQueryBatch{ IQueryBatchResult::createEmpty() }
	, mCurrentRecord{ IEventRecord::createEmpty() }
{
	mQuery->queryChannelXPath(channelPath, queryText, direction);
}

EventReaderImpl::EventReaderImpl(const FileReader &, const std::string &filePath, const std::string &queryText, Direction direction)
	: mQuery{EventLogQuery::create()}
	, mQueryBatch { IQueryBatchResult::createEmpty() }
	, mCurrentRecord{ IEventRecord::createEmpty() }
{
	mQuery->queryFileXPath(filePath, queryText, direction);
}

EventReaderImpl::EventReaderImpl(const std::string &structuredXML, Direction dir)
	: mQuery{EventLogQuery::create()}
	, mQueryBatch { IQueryBatchResult::createEmpty() }
	, mCurrentRecord{ IEventRecord::createEmpty() }
{
	mQuery->queryStructuredXML(structuredXML, dir);
}

bool EventReaderImpl::next()
{
	bool hasNext = false;

	// If there are event records to retrieve
	if ((mEventCount > 0) && (mCurrent < (mEventCount - 1)))
	{
		mCurrentRecord = mQueryBatch->getRecord(mCurrent);
		mCurrent += 1;		
		hasNext = true;
	}
	else // -> mEventCount == 0 || mCurrent == (mEventCount - 1) 
	{
		// We need events (either have none or need more) so fetch the next batch.
		mQueryBatch = mQuery->getNextBatch(BatchSize, getTimeout());
		mEventCount = mQueryBatch->getCount();
		
		if (mQueryBatch->getStatus() == QueryNextStatus::Success)
		{
			mCurrent = 0;
			mCurrentRecord = mQueryBatch->getRecord(mCurrent);
			hasNext = true;
		}
		else
		{
			// Ruh-roh. 
			mCurrentRecord = IEventRecord::createEmpty();
			hasNext = false;
		}
	}

	return hasNext;
}

void EventReaderImpl::seek(int64_t position, SeekOption option)
{
	this->mQuery->seek(position, option);
}

//
// EventReader
//

Ref<EventReader> EventReader::openChannel(const std::string &channel,
	const std::string &queryText, Direction direction)
{
	return RefObject<EventReader>::createRef(OpenChannel{}, channel, queryText, direction);
}

Ref<EventReader> EventReader::openFile(const std::string &channel,
	const std::string &queryText, Direction direction)
{
	return RefObject<EventReader>::createRef(OpenFile{}, channel, queryText, direction);
}

Ref<EventReader> EventReader::openStructuredXML(const std::string &structuredQueryXML,
	Direction direction)
{
	return RefObject<EventReader>::createRef(structuredQueryXML, direction);
}

EventReader::EventReader(OpenChannel, const std::string &channel,
	const std::string &queryText, Direction direction)
	: d_ptr{std::make_unique<EventReaderImpl>(EventReaderImpl::ChannelReader{}, channel, queryText, direction)}
{}

EventReader::EventReader(OpenFile, const std::string &channel, 
	const std::string queryText, Direction direction)
	: d_ptr{std::make_unique<EventReaderImpl>(EventReaderImpl::FileReader{}, channel, queryText, direction)}
{}

EventReader::EventReader(const std::string &structuredQueryText, Direction direction)
	: d_ptr{std::make_unique<EventReaderImpl>(structuredQueryText, direction)}
{}

EventReader::~EventReader()
{}

bool EventReader::next()
{
	return d_ptr->next();
}

uint32_t EventReader::getTimeout() const 
{
	return d_ptr->getTimeout(); 
}

void EventReader::setTimeout(uint32_t timeout) 
{ 
	d_ptr->setTimeout(timeout);
}

Ref<IEventRecord> EventReader::getRecord() const
{
	return d_ptr->getCurrent();
}

void EventReader::seek(int64_t position, SeekOption whence)
{
	d_ptr->seek(position, whence);
}

//
// IEventReader
//

Ref<IEventReader> IEventReader::openChannel(const std::string &channel,
	const std::string &queryText, Direction direction)
{
	return EventReader::openChannel(channel, queryText, direction);
}

Ref<IEventReader> IEventReader::openFile(const std::string &filePath, const std::string &queryText, Direction direction)
{
	return EventReader::openFile(filePath, queryText, direction);
}

Ref<IEventReader> IEventReader::openStructuredXML(const std::string &structuredQueryText, Direction direction)
{
	return EventReader::openStructuredXML(structuredQueryText, direction);
}

}