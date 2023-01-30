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

#include "EventLogQuery.h"

#include "EventRecord.h"
#include "EvtHandle.h"
#include "WinSys.h"
#include "Array.h"
#include "Queues.h"
#include "StringUtils.h"

namespace Windows::EventLog
{ 

template<typename T> 
class IMethod: public IRefObject
{
public:
	~IMethod() = default;
	virtual void process(T *)  = 0;
	virtual void complete() = 0;
	virtual WaitResult wait(uint32_t timeout) = 0;

	virtual void captureCurrentException() noexcept = 0;		
	virtual void rethrowCapturedException() = 0;
	virtual bool hasCapturedException() noexcept = 0;
};

class EventLogQueryMethodBase : public IMethod<EventLogQueryImpl>
{
	AutoResetEvent mComplete{FALSE};
	std::exception_ptr mException{nullptr};
public:
	~EventLogQueryMethodBase() = default;

	WaitResult wait(uint32_t timeout) noexcept override;
	void complete() override;
	void captureCurrentException() noexcept override;
	bool hasCapturedException() noexcept override;
	void rethrowCapturedException() override;
};

class QueryChannelXPathMethod : public EventLogQueryMethodBase
{
	// Parameters
	std::string mChannel;
	std::string mXPathQuery;
	Direction mDirection;
public:
	QueryChannelXPathMethod() = default;
	static RefPtr<QueryChannelXPathMethod> create(const std::string &channel, const std::string &xpathQuery, Direction dir);
	QueryChannelXPathMethod(const std::string &channel, const std::string &xpathQuery, Direction dir);
	void process(EventLogQueryImpl *r) override;
};

class QueryFileXPathMethod : public EventLogQueryMethodBase
{
	std::string mFilePath;
	std::string mXPathQuery;
	Direction mDirection;
public:
	static RefPtr<QueryFileXPathMethod> create(const std::string &filePath, const std::string &xpathQuery, Direction dir);
	QueryFileXPathMethod(const std::string &filePath, const std::string &xpathQuery, Direction dir);
	void process(EventLogQueryImpl *r) override;
};

class QueryStructuredXMLMethod : public EventLogQueryMethodBase
{
	std::string mStructuredXML;
	Direction mDirection;
public:
	static RefPtr<QueryStructuredXMLMethod> create(const std::string &structuredXML, Direction dir);
	QueryStructuredXMLMethod(const std::string &structuredXML, Direction dir);
	void process(EventLogQueryImpl *r) override;
};

using EvtHandleArray = Array<EVT_HANDLE, EvtHandleClose>;

class GetNextBatchMethod : public EventLogQueryMethodBase
{
	uint32_t mTimeout;
public:

	static RefPtr<GetNextBatchMethod> create(uint32_t batchSize, uint32_t timeout);

	// Yes, public. 
	EvtHandleArray Events{};
	uint32_t BatchCount{};
	QueryNextStatus Status{};

	GetNextBatchMethod(uint32_t batchSize, uint32_t timeout);
	void process(EventLogQueryImpl *r) override;
};

class SeekMethod : public EventLogQueryMethodBase
{
	int64_t mPosition;
	SeekOption mWhence;
public:
	static RefPtr<SeekMethod> create(int64_t position, SeekOption whence);

	SeekMethod(int64_t position, SeekOption whence);
	void process(EventLogQueryImpl *r) override;

};

class CloseMethod : public EventLogQueryMethodBase
{
public:
	SysErr Err{};
	void process(EventLogQueryImpl *r) override;
};

//
// QueryBatchResult 
//

class QueryBatchResult : public IQueryBatchResult
{
public:
	friend RefObject<QueryBatchResult>;

	static Ref<QueryBatchResult> createTimeout();

	static Ref<QueryBatchResult> createNoMoreItems();

	static Ref<QueryBatchResult> createSuccess(EvtHandleArray events, uint32_t count);

	~QueryBatchResult();

	QueryNextStatus getStatus() const override;
	void setStatus(QueryNextStatus status);

	uint32_t getCount() const override;

	Ref<IEventRecord> getRecord(uint32_t index) const override;

private:
	QueryNextStatus mStatus{QueryNextStatus::Success};
	EvtHandleArray mEvents{};
	uint32_t mCount{0};

private:
	QueryBatchResult(QueryNextStatus status, EvtHandleArray events, uint32_t count);
	QueryBatchResult(QueryNextStatus status);

};

// Null avoidance sentinel
class EmptyQueryBatchResult : public IQueryBatchResult
{
public:
	static Ref<EmptyQueryBatchResult> create()
	{
		return RefObject<EmptyQueryBatchResult>::createRef();
	}

	QueryNextStatus getStatus() const override { return QueryNextStatus::NoMoreItems; }
	uint32_t getCount() const override { return 0u; }
	Ref<IEventRecord> getRecord(uint32_t /* index */) const override { return IEventRecord::createEmpty(); }
};

Ref<QueryBatchResult> QueryBatchResult::createTimeout()
{
	return RefObject<QueryBatchResult>::createRef(QueryNextStatus::Timeout);
}

Ref<QueryBatchResult> QueryBatchResult::createNoMoreItems()
{
	return RefObject<QueryBatchResult>::createRef(QueryNextStatus::NoMoreItems);
}

Ref<QueryBatchResult> QueryBatchResult::QueryBatchResult::createSuccess(EvtHandleArray events, uint32_t count)
{
	return RefObject<QueryBatchResult>::createRef(QueryNextStatus::Success, std::move(events), count);
}

QueryBatchResult::QueryBatchResult(QueryNextStatus status, EvtHandleArray events, uint32_t count)
	: mStatus{ status }
	, mEvents{ std::move(events) }
	, mCount(count)
{
}

QueryBatchResult::QueryBatchResult(QueryNextStatus status)
	: mStatus{ status }
{}

QueryBatchResult::~QueryBatchResult()
{}

QueryNextStatus QueryBatchResult::getStatus() const 
{
	return mStatus;
}

void QueryBatchResult::setStatus(QueryNextStatus status)
{
	mStatus = status;
}

uint32_t QueryBatchResult::getCount() const 
{
	return mCount;
}

Ref<IEventRecord> QueryBatchResult::getRecord(uint32_t index) const 
{
	if (index >= mCount)
	{
		THROW(IndexOutOfBoundsException);
	}

	return EventRecord::create(EventRecordHandle(mEvents[index]));
}

//
// EventLogQueryImpl
//

class EventLogQueryImpl
{
public:

	friend void QueryChannelXPathMethod::process(EventLogQueryImpl *);
	friend void QueryFileXPathMethod::process(EventLogQueryImpl *);
	friend void QueryStructuredXMLMethod::process(EventLogQueryImpl *);
	friend void SeekMethod::process(EventLogQueryImpl *);
	friend void GetNextBatchMethod::process(EventLogQueryImpl *);
	friend void CloseMethod::process(EventLogQueryImpl *);

	EventLogQueryImpl();
	~EventLogQueryImpl();

	void queryChannelXPath(const std::string &channel, const std::string &xpathQuery, Direction dir);
	void queryFileXPath(const std::string &filePath, const std::string &xpathQuery, Direction dir);
	void queryStructuredXML(const std::string &structuredXML, Direction dir);

	Ref<IQueryBatchResult> getNextBatch(uint32_t batchSize, uint32_t timeout);

	void seek(int64_t position, SeekOption whence);

	void close();

private:

	void execQueryChannelXPath(const std::string &channel, const std::string &xpathQuery, Direction dir);
	void execQueryFileXPath(const std::string &filePath, const std::string &xpathQuery, Direction dir);
	void execQueryStructuredXML(const std::string &structuredXML, Direction dir);
	QueryNextStatus execGetNextBatch(EvtHandleArray &a, uint32_t timeout, uint32_t *count);
	void execSeek(int64_t position, SeekOption whence);
	SysErr execClose();

	// Intended to be called from the dtor, so musn't throw. 
	// Triggers and waits for thread shutdown.
	void terminate() noexcept;

private:
	static unsigned objectMain(void *arg);
	unsigned objectThisMain();

	// All void returns handled the same way.

	void enqueueVoidReturnAndWait(RefPtr<IMethod<EventLogQueryImpl> > pVoidReturnMethod);

private:
	// Order is important. The queue must exist before the thread.
	BoundedSynchQueue< RefPtr<IMethod<EventLogQueryImpl> > > mQ;
	Thread mThread;
	QueryHandle mQueryHandle;

	EventLogQueryImpl(const EventLogQueryImpl &) = delete;
	EventLogQueryImpl &operator=(const EventLogQueryImpl &) = delete;

};

//
// EventLogQueryMethodBase implementation
//

WaitResult EventLogQueryMethodBase::wait(uint32_t timeout) noexcept 
{
	return mComplete.wait(timeout);
}

void EventLogQueryMethodBase::complete() 
{
	mComplete.set();
}

void EventLogQueryMethodBase::captureCurrentException() noexcept 
{
	mException = std::current_exception();
}

bool EventLogQueryMethodBase::hasCapturedException() noexcept 
{
	return mException ? true : false;
}

void EventLogQueryMethodBase::rethrowCapturedException() 
{
	if (mException)
		std::rethrow_exception(mException);
}

//
// QueryChannelXPathCall
//

RefPtr<QueryChannelXPathMethod> QueryChannelXPathMethod::create(const std::string &channel, const std::string &xpathQuery, Direction dir)
{
	return RefObject<QueryChannelXPathMethod>::create(channel, xpathQuery, dir);
}

QueryChannelXPathMethod::QueryChannelXPathMethod(const std::string &channel, const std::string &xpathQuery, Direction dir)
	: mChannel(channel)
	, mXPathQuery(xpathQuery)
	, mDirection(dir)
{}

void QueryChannelXPathMethod::process(EventLogQueryImpl *r) 
{
	r->execQueryChannelXPath(mChannel, mXPathQuery, mDirection);
}

//
// QueryFileXPathCall
//

RefPtr<QueryFileXPathMethod> QueryFileXPathMethod::create(const std::string &channel, const std::string &xpathQuery, Direction dir)
{
	return RefObject<QueryFileXPathMethod>::create(channel, xpathQuery, dir);
}

QueryFileXPathMethod::QueryFileXPathMethod(const std::string &filePath, const std::string &xpathQuery, Direction dir)
	: mFilePath(filePath)
	, mXPathQuery(xpathQuery)
	, mDirection(dir)
{}

void QueryFileXPathMethod::process(EventLogQueryImpl *r) 
{
	r->execQueryChannelXPath(mFilePath, mXPathQuery, mDirection);
}

//
// QueryStructuredXMLCall
//

RefPtr<QueryStructuredXMLMethod> QueryStructuredXMLMethod::create(const std::string &structuredXML, Direction dir)
{
	return RefObject<QueryStructuredXMLMethod>::create(structuredXML, dir);
}


QueryStructuredXMLMethod::QueryStructuredXMLMethod(const std::string &structuredXML, Direction dir)
	: mStructuredXML(structuredXML)
	, mDirection(dir)
{}

void QueryStructuredXMLMethod::process(EventLogQueryImpl *r) 
{
	r->execQueryStructuredXML(mStructuredXML, mDirection);
}

//
// NextCall
//

RefPtr<GetNextBatchMethod> GetNextBatchMethod::create(uint32_t batchSize, uint32_t timeout)
{
	return RefObject<GetNextBatchMethod>::create(batchSize, timeout);
}

GetNextBatchMethod::GetNextBatchMethod(uint32_t batchSize, uint32_t timeout)
	: mTimeout(timeout)
	, Events(batchSize)
	, BatchCount(0)
	, Status{}
{}

void GetNextBatchMethod::process(EventLogQueryImpl *r) 
{
	Status = r->execGetNextBatch(Events, mTimeout, &BatchCount);
}

//
// CloseCall
//

void CloseMethod::process(EventLogQueryImpl *r)  
{
	Err = r->execClose();
}

//
// SeekMethod
//

RefPtr<SeekMethod> SeekMethod::create(int64_t position, SeekOption whence)
{
	return RefObject<SeekMethod>::create(position, whence);
}

SeekMethod::SeekMethod(int64_t position, SeekOption whence)
	: mPosition(position)
	, mWhence(whence)
{}

void SeekMethod::process(EventLogQueryImpl *r)
{
	r->execSeek(mPosition, mWhence);
}

//
// EventQueryImpl
//

// 1 minute. 
static constexpr DWORD CALL_FAILSAFE_TIMEOUT = 1000 * 60;

EventLogQueryImpl::EventLogQueryImpl()
	: mQ{}
	, mThread(Thread::begin(&EventLogQueryImpl::objectMain, this))
{
}

EventLogQueryImpl::~EventLogQueryImpl()
{
	terminate();
}

void EventLogQueryImpl::enqueueVoidReturnAndWait(RefPtr<IMethod<EventLogQueryImpl>> pMethod)
{
	mQ.enqueue(pMethod);

	WaitResult result = pMethod->wait(CALL_FAILSAFE_TIMEOUT);

	WaitStatus status = result.getStatus();
	switch (status)
	{
	case WaitStatus::Object_0:

		// Does nothing if no captured exception.
		pMethod->rethrowCapturedException();

		break;
	case WaitStatus::Timeout:
		// This is the failsafe timeout. It's NOT expected so we throw.
		THROW_(SystemError, ERROR_TIMEOUT);

	case WaitStatus::Failed:
		result.throwError(); // Does not return

	default:
		// TODO: Should never get here. Is doing nothing reasonable? 
		// but it isn't easy to solve this sort of thing ... 
		break;
	}

}

void EventLogQueryImpl::queryChannelXPath(const std::string &channel, const std::string &xpathQuery, Direction dir)
{
	RefPtr<QueryChannelXPathMethod> pQueryChannelXPath = QueryChannelXPathMethod::create(channel, xpathQuery, dir);
	enqueueVoidReturnAndWait(pQueryChannelXPath);
}

void EventLogQueryImpl::queryFileXPath(const std::string &filePath, const std::string &xpathQuery, Direction dir)
{
	RefPtr<QueryFileXPathMethod> pQueryFileXPath = QueryFileXPathMethod::create(filePath, xpathQuery, dir);
	enqueueVoidReturnAndWait(pQueryFileXPath);
}

void EventLogQueryImpl::queryStructuredXML(const std::string &structuredXML, Direction dir)
{
	RefPtr<QueryStructuredXMLMethod> pQuery = QueryStructuredXMLMethod::create(structuredXML, dir);
	enqueueVoidReturnAndWait(pQuery);
}

Ref<IQueryBatchResult> EventLogQueryImpl::getNextBatch(uint32_t batchSize, uint32_t timeout)
{
	RefPtr<GetNextBatchMethod> pNextCall = GetNextBatchMethod::create(batchSize, timeout);

	mQ.enqueue(pNextCall);

	WaitResult result = pNextCall->wait(CALL_FAILSAFE_TIMEOUT);

	WaitStatus status = result.getStatus();
	switch (status)
	{
	case WaitStatus::Object_0:

		// Does nothing if no captured exception.
		pNextCall->rethrowCapturedException();

		switch (pNextCall->Status)
		{
		case QueryNextStatus::Success:
			return QueryBatchResult::createSuccess(std::move(pNextCall->Events), pNextCall->BatchCount);
		case QueryNextStatus::NoMoreItems:
			return QueryBatchResult::createNoMoreItems();
		case QueryNextStatus::Timeout: // Specified timeout. Expected.
			return QueryBatchResult::createTimeout();
		default: // Never happens.
			return QueryBatchResult::createNoMoreItems();
		}

		break; // Never get here.
	case WaitStatus::Timeout: // Failsafe timeout. Not expected.
		THROW_(SystemError, ERROR_TIMEOUT);

	case WaitStatus::Failed:
		result.throwError(); // Does not return

	default: // Never happens.
		return QueryBatchResult::createNoMoreItems();
	}
}

void EventLogQueryImpl::close()
{
	RefPtr<CloseMethod> pCloseMethod(RefObject<CloseMethod>::create());
	enqueueVoidReturnAndWait(pCloseMethod);
}

void EventLogQueryImpl::seek(int64_t position, SeekOption whence)
{
	RefPtr<SeekMethod> pSeekMethod(RefObject<SeekMethod>::create(position, whence));
	enqueueVoidReturnAndWait(pSeekMethod);
}

unsigned EventLogQueryImpl::objectMain(void *arg)
{
	try
	{
		return static_cast<EventLogQueryImpl*>(arg)->objectThisMain();
	}
	catch (...) // This is the top of thread stack, so swallow everything.
	{
		return 1;
	}
}

unsigned EventLogQueryImpl::objectThisMain()
{
	bool done = false;
	while (!done)
	{
		auto p = mQ.dequeue();
		if (p.has_value())
		{
			RefPtr<IMethod<EventLogQueryImpl>> c = p.value();
			if (c)
			{
				try
				{
					c->process(this);
				}
				catch (...)
				{
					c->captureCurrentException();
				}

				c->complete();
			}
			else
			{
				done = true;
			}
		}
		else
		{ 
			done = true;
		}
	}

	return 0;
}

void EventLogQueryImpl::terminate() noexcept
{
	mQ.enqueue({});
	mThread.wait(CALL_FAILSAFE_TIMEOUT);
	// TODO: Do we care about the result?
	// If it fails, there's nothing we can do.
}

void EventLogQueryImpl::execQueryChannelXPath(const std::string &channel, const std::string &xpathQuery, Direction dir)
{
	if (mQueryHandle)
	{
		SysErr err = execClose();
		if (err.failed()) 
		{
			THROW_(SystemError, err.getCode());
		}
	}
	uint32_t flags = EvtQueryChannelPath | 
		(dir == Direction::Forward ? EvtQueryForwardDirection : EvtQueryReverseDirection);

	mQueryHandle = QueryHandle::query(to_utf16(channel).c_str(), to_utf16(xpathQuery).c_str(), flags);
}

void EventLogQueryImpl::execQueryFileXPath(const std::string &filePath, const std::string &xpathQuery, Direction dir)
{
	if (mQueryHandle)
	{
		SysErr err = execClose();
		if (err.failed()) 
		{
			THROW_(SystemError, err.getCode());
		}
	}

	uint32_t flags = EvtQueryFilePath | 
		(dir == Direction::Forward ? EvtQueryForwardDirection : EvtQueryReverseDirection);

	mQueryHandle = QueryHandle::query(to_utf16(filePath).c_str(), to_utf16(xpathQuery).c_str(), flags);
}

void EventLogQueryImpl::execQueryStructuredXML(const std::string &structuredXML, Direction dir)
{
	if (mQueryHandle)
	{
		SysErr err = execClose();
		if (err.failed()) 
		{
			THROW_(SystemError, err.getCode());
		}
	}
	uint32_t flags = (dir == Direction::Forward ? EvtQueryForwardDirection : EvtQueryReverseDirection);

	mQueryHandle = QueryHandle::query(nullptr, to_utf16(structuredXML).c_str(), flags);
}

QueryNextStatus EventLogQueryImpl::execGetNextBatch(EvtHandleArray &a, uint32_t timeout, uint32_t *count)
{
	uint32_t size = uint32_t(a.size());
	QueryNextStatus status = mQueryHandle.next(size, ptr(a), timeout, 0, count);
	return status;
}

void EventLogQueryImpl::execSeek(int64_t position, SeekOption whence)
{
	mQueryHandle.seek(position, whence);
}

SysErr EventLogQueryImpl::execClose()
{
	return mQueryHandle.close();
}

//
// EventLogQuery
//

Ref<IEventLogQuery> EventLogQuery::create()
{
	return RefObject<EventLogQuery>::createRef();
}

EventLogQuery::EventLogQuery()
	: d_ptr{ new EventLogQueryImpl{ } }
{
}

EventLogQuery::~EventLogQuery()
{
	// Nothing.
}

void EventLogQuery::queryChannelXPath(const std::string &channel, const std::string &queryXPath, Direction dir)
{
	d_ptr->queryChannelXPath(channel, queryXPath, dir);
}

void EventLogQuery::queryFileXPath(const std::string &filePath, const std::string queryXPath, Direction dir)
{
	d_ptr->queryFileXPath(filePath, queryXPath, dir);
}

void EventLogQuery::queryStructuredXML(const std::string &structuredXML, Direction dir)
{
	d_ptr->queryStructuredXML(structuredXML, dir);
}

Ref<IQueryBatchResult> EventLogQuery::getNextBatch(uint32_t batchSize, uint32_t timeout)
{
	return d_ptr->getNextBatch(batchSize, timeout);
}

void EventLogQuery::seek(int64_t position, SeekOption flags)
{
	return d_ptr->seek(position, flags);
}

// Closes the query handle.
void EventLogQuery::close() 
{
	d_ptr->close();
}

Ref<IQueryBatchResult> IQueryBatchResult::createEmpty()
{
	return EmptyQueryBatchResult::create();
}

}
