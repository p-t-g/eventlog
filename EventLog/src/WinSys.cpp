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

#include "WinSys.h"

#include "CommonTypes.h"
#include "StringUtils.h"

#include <sddl.h>
#include <AclAPI.h>
#include <strsafe.h>

namespace Windows
{

struct FormatMessageBuffer
{
	void *p = nullptr;

	~FormatMessageBuffer()
	{
		if (p) ::LocalFree(p);
	}

	LPWSTR operator&()
	{
		return reinterpret_cast<LPWSTR>(&p);
	}

	operator LPWSTR()
	{
		return static_cast<LPWSTR>(p);
	}
};

std::string formatMessage(uint32_t errorCode) 
{
	std::string m;

	FormatMessageBuffer buf;
	DWORD result = ::FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		DWORD(errorCode),
		0, // Use the built-in algorithm for matching language
		&buf,
		0, 
		nullptr);
	if (result != 0)
	{
		m = to_utf8(buf);
	}

	return m;
}

Guid::Guid(const wchar_t *wsz)
{
	HRESULT hr = ::CLSIDFromString(wsz, &value);
	if (FAILED(hr))
	{
		THROW_(SystemException, hr);
	}
}

std::string Guid::to_string(const GUID  &g)
{
	return to_utf8(to_wstring(g));
}

std::wstring Guid::to_wstring(const GUID &g)
{
	std::wstring ws;
	LPOLESTR s = nullptr;
	HRESULT hr = ::StringFromCLSID(g, &s);
	if (SUCCEEDED(hr) && s)
	{
		ws = s;
		CoTaskMemFree(s);
	}
	return ws;
}

template<typename T>
struct CoTaskMemOutPtr
{
	void *p = nullptr;

	~CoTaskMemOutPtr()
	{
		if (p)
			::CoTaskMemFree(p);
		p = nullptr;
	}

	template<typename U = T,
		// Help avoid mistakes
		typename = std::enable_if_t< std::is_pointer_v<U> > >
	U *operator &()
	{
		return reinterpret_cast<U*>(&p);
	}

	template<typename U = T,
		// Help avoid mistakes
		typename = std::enable_if_t< std::is_pointer_v<U> > >
	operator U ()
	{
		return static_cast<U>(p);
	}
};

std::string to_string(GUID g)
{
	return Guid::to_string(g);
}

std::string to_string(Guid g)
{
	return g.to_string();
}

std::string Guid::to_string() const
{
	std::string s;

	CoTaskMemOutPtr<LPOLESTR> buf;
	HRESULT hr = ::StringFromCLSID(value, &buf);
	if (FAILED(hr))
	{
		THROW_(SystemException, hr);
	}

	s = to_utf8(buf);

	return s;
}

std::string FileTime::to_string(uint64_t ft)
{
	using Windows::to_string;
	return to_string(to_FILETIME(ft));
}

std::string to_string(const FILETIME &ft)
{	
	SYSTEMTIME st{};
	::FileTimeToSystemTime(&ft, &st);

	TIME_ZONE_INFORMATION tzi{};
	::GetTimeZoneInformation(&tzi); 

	SYSTEMTIME lst{};
	::SystemTimeToTzSpecificLocalTime(&tzi, &st, &lst);

	return SystemTime::format(lst);
}

std::string to_string(const FileTime &ft)
{
	return ft.to_string();
}

std::string FileTime::to_string() const
{
	using Windows::to_string;
	return to_string(ft);
}

SystemTime SystemTime::getSystemTimeUTC()
{
	SYSTEMTIME st{};
	::GetSystemTime(&st);
	return st;
}

SystemTime SystemTime::getSystemTimeLocal()
{
	SYSTEMTIME st{};
	::GetLocalTime(&st);
	return st;
}

std::string SystemTime::format(const SYSTEMTIME &st)
{
	DWORD err;

	int totalSize = 0;
	int dateSize = ::GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, 0, &st, nullptr, nullptr, 0, nullptr);
	if (dateSize == 0)
	{
		err = ::GetLastError();
		THROW_(SystemException, err);
	}

	totalSize += dateSize;

	int timeSize = ::GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, 0, &st, nullptr, nullptr, 0);
	if (timeSize == 0)
	{
		err = ::GetLastError();
		THROW_(SystemException, err);
	}

	totalSize += timeSize;

	auto buf = std::make_unique<wchar_t[]>(totalSize);

	int n = ::GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, 0, &st, nullptr, buf.get(), dateSize, nullptr);
	if (n == 0)
	{
		err = ::GetLastError();
		THROW_(SystemException, err);
	}

	buf[size_t(dateSize) - 1] = L' ';
	n = ::GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, 0, &st, nullptr, buf.get() + dateSize, timeSize);
	if (n == 0)
	{
		err = ::GetLastError();
		THROW_(SystemException, err);
	}

	return to_utf8(buf.get());
}

std::string SystemTime::to_string() const
{
	using Windows::to_string;
	return to_string(st);
}

std::string to_string(const SYSTEMTIME &st)
{
	std::unique_ptr<wchar_t []> buffer(std::make_unique<wchar_t[]>(64));
	int result = ::GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, 0, &st, nullptr, buffer.get(), 64);
	if (result == 0)
	{
		THROW_(SystemException, ::GetLastError());
	}

	return to_utf8(buffer.get());
}

WaitStatus to_WaitStatus(DWORD status)
{
	switch (status)
	{
	case WAIT_ABANDONED:
		return WaitStatus::Abandoned;
	case WAIT_IO_COMPLETION:
		return WaitStatus::IoCompletion;
	case WAIT_OBJECT_0:
		return WaitStatus::Object_0;
	case WAIT_TIMEOUT:
		return WaitStatus::Timeout;
	case WAIT_FAILED:
		return WaitStatus::Failed;
	default:
		return WaitStatus::Failed;
	}
}

WaitResult WaitResult::make(DWORD status)
{
	WaitResult r{};
	r.mStatus = to_WaitStatus(status);
	r.mLastErr = (status == WAIT_FAILED) ? ::GetLastError() : ERROR_SUCCESS;
	return r;
}

// Unconditionally throw a SystemError with the contained error code.
[[noreturn]] void WaitResult::throwError() 
{
	THROW_(SystemException, mLastErr.getCode());
}

WaitResult WaitableHandle::wait(HANDLE h, DWORD timeout, BOOL alertable) noexcept
{
	DWORD status = ::WaitForSingleObjectEx(h, timeout, alertable);
	return WaitResult::make(status);
}

//
// ObjectHandle
//

HANDLE ObjectHandle::duplicate(HANDLE hSourceProcess, HANDLE hSourceHandle,
	HANDLE hTargetProcess, DWORD access, BOOL inherit,
	DWORD flags)
{
	HANDLE hResultHandle = nullptr;
	BOOL success = ::DuplicateHandle(
		hSourceProcess,
		hSourceHandle,
		hTargetProcess,
		&hResultHandle,
		access,
		inherit,
		flags);
	if (!success)
	{
		DWORD err = ::GetLastError();
		THROW_(SystemException, err);
	}
	return hResultHandle;
}

HANDLE ObjectHandle::duplicate(HANDLE h, DWORD access , BOOL inherit, DWORD flags)
{
	return duplicate(::GetCurrentProcess(), h, ::GetCurrentProcess(), access, inherit, flags);
}

ObjectHandle::~ObjectHandle()
{
	if (!isNullOrInvalid())
		::CloseHandle(mHandle);
	mHandle = nullptr;
}

ObjectHandle &ObjectHandle::operator=(ObjectHandle &&rhs) noexcept
{
	if (!isNullOrInvalid()) {
		::CloseHandle(mHandle);
	}
	mHandle = rhs.mHandle;
	rhs.mHandle = nullptr;
	return *this;
}

//
// Thread
//

unsigned __stdcall Thread::objectMain_Runnable(void *arg)
{
	// If arg is null, there's nothing to do.
	if (!arg)
		return 0;
	std::unique_ptr<IRunnable> runnable(static_cast<IRunnable *>(arg));

	try
	{
		runnable->run();
	}
	catch (...) 
	{
		return 1;
	}
	return 0;
}

HANDLE Thread::beginThreadEx(void *_Security, unsigned _StackSize, _beginthreadex_proc_type _StartAddress, void *_ArgList, unsigned _InitFlag, unsigned *_ThrdAddr)
{
	return reinterpret_cast<HANDLE>(
		_beginthreadex(
			_Security,
			_StackSize,
			_StartAddress,
			_ArgList,
			_InitFlag,
			_ThrdAddr));
}

Thread Thread::current()
{
	return Thread(ObjectHandle::duplicate(::GetCurrentThread()), ::GetCurrentThreadId());
}

Thread::Thread(Thread &&rhs) noexcept
	: mhThread(std::move(rhs.mhThread))
	, mThreadId(rhs.mThreadId)
{
	rhs.mThreadId = DWORD(-1);
}

Thread &Thread::operator=(Thread &&rhs) noexcept
{
	mhThread = std::move(rhs.mhThread);

	mThreadId = rhs.mThreadId;
	rhs.mThreadId = DWORD(-1);

	return *this;
}

Thread Thread::begin(unsigned(__stdcall *pfn)(void *), void *arg)
{
	unsigned int tid = 0;
	HANDLE hThread = beginThreadEx(nullptr, 0, pfn, arg, 0, &tid);
	if (!hThread)
	{
		THROW_(SystemException, static_cast<DWORD>(_doserrno));
	}
	return Thread(hThread, static_cast<DWORD>(tid));
}

// Create an independent thread object representing the same thread as *this.
Thread Thread::duplicate() const
{
	return Thread(ObjectHandle::duplicate(mhThread.handle()), mThreadId);
}

WaitResult Thread::wait(DWORD timeout, BOOL alertable) noexcept
{
	return WaitableHandle::wait(mhThread.handle(), timeout, alertable);
}

void Thread::join()
{
	wait(INFINITE);
}

//
// Semaphore
//

HANDLE Semaphore::create(LONG initial, LONG max, LPCWSTR name)
{
	HANDLE hSemaphore = ::CreateSemaphoreW(nullptr, initial, max, name);
	if (!hSemaphore)
	{
		THROW_(SystemException, ::GetLastError());
	}
	return hSemaphore;
}

Semaphore::Semaphore(LONG initial, LONG max, LPCWSTR name)
	: mhSemaphore(Semaphore::create(initial, max, name))
{
}

SysErr Semaphore::release(LONG releaseCount, LONG *previousCount)
{
	BOOL success = ::ReleaseSemaphore(mhSemaphore.handle(), releaseCount, previousCount);
	if (!success)
	{
		return SysErr::getLast();
	}
	return {};
}

WaitResult Semaphore::wait(DWORD timeout, BOOL alertable)
{
	return WaitableHandle::wait(mhSemaphore.handle(), timeout, alertable);
}

//
// CriticalSection
//

CriticalSection::CriticalSection()
{
	::InitializeCriticalSection(&mCS);
}

CriticalSection::CriticalSection(LONG spinCount, DWORD flags)
{
	::InitializeCriticalSectionEx(&mCS, spinCount, flags);
}

CriticalSection::~CriticalSection()
{
	::DeleteCriticalSection(&mCS);
}

void CriticalSection::enter()
{
	::EnterCriticalSection(&mCS);
}

bool CriticalSection::tryEnter()
{
	return ::TryEnterCriticalSection(&mCS) ? true : false;
}

void CriticalSection::leave()
{
	::LeaveCriticalSection(&mCS);
}

// 
// Event
// 

HANDLE Event::create(BOOL bManualReset, BOOL bInitialState, LPCWSTR name)
{
	HANDLE hEvent = ::CreateEventW(nullptr, bManualReset, bInitialState, name);
	if (!hEvent)
	{
		DWORD err = ::GetLastError();
		THROW_(SystemException, err);
	}
	return hEvent;
}

HANDLE Event::create(BOOL bManualReset, BOOL bInitialState, LPCSTR name)
{
	HANDLE hEvent = ::CreateEventA(nullptr, bManualReset, bInitialState, name);
	if (!hEvent)
	{
		DWORD err = ::GetLastError();
		THROW_(SystemException, err);
	}
	return hEvent;
}

Event::Event(BOOL bManualReset, BOOL bInitialState, LPCWSTR name)
	: mhEvent(Event::create(bManualReset, bInitialState, name))
{}

Event::Event(BOOL bManualReset, BOOL bInitialState, LPCSTR name)
	: mhEvent(Event::create(bManualReset, bInitialState, name))
{}

void Event::reset()
{
	BOOL success = ::ResetEvent(mhEvent.handle());
	if (!success)
	{
		DWORD err = ::GetLastError();
		THROW_(SystemException, err);
	}
}

void Event::set()
{
	BOOL success = ::SetEvent(mhEvent.handle());
	if (!success)
	{
		DWORD err = ::GetLastError();
		THROW_(SystemException, err);
	}
}

WaitResult Event::wait(DWORD timeout, BOOL alertable) noexcept
{
	return WaitableHandle::wait(mhEvent.handle(), timeout, alertable);
}

Event Event::duplicate() const
{
	return Event(ObjectHandle::duplicate(mhEvent.handle()));
}

//
// ManualResetEvent
//

ManualResetEvent::ManualResetEvent(BOOL initialState, LPCSTR name)
	: mEvent(TRUE, initialState, name)
{}

ManualResetEvent::ManualResetEvent(BOOL initialState, LPCWSTR name)
	: mEvent(TRUE, initialState, name)
{}

ManualResetEvent ManualResetEvent::duplicate() const
{
	return ManualResetEvent(mEvent.duplicate());
}

void ManualResetEvent::set()
{
	mEvent.set();
}

void ManualResetEvent::reset()
{
	mEvent.reset();
}

WaitResult ManualResetEvent::wait(DWORD timeout, BOOL alertable ) noexcept
{
	return mEvent.wait(timeout, alertable);
}

ManualResetEvent::ManualResetEvent(Event ev)
	: mEvent(ev.duplicate()) 
{}

//
// AutoResetEvent
//

AutoResetEvent::AutoResetEvent(BOOL initialState, LPCWSTR name)
	: mEvent(FALSE, initialState, name)
{}

AutoResetEvent::AutoResetEvent(BOOL initialState, LPCSTR name)
	: mEvent(FALSE, initialState, name)
{}

AutoResetEvent AutoResetEvent::duplicate() const
{
	return AutoResetEvent(mEvent.duplicate());
}

void AutoResetEvent::set()
{
	return mEvent.set();
}

WaitResult AutoResetEvent::wait(DWORD timeout, BOOL alertable) noexcept
{
	return mEvent.wait(timeout, alertable);
}

AutoResetEvent::AutoResetEvent(Event ev)
	: mEvent(std::move(ev))
{}

// Retrieves the account name for the given SID 
std::string lookupAccount(PSID pSid)
{
	std::string user{};
	DWORD NameLength = 0;
	DWORD DomainLength = 0;
	SID_NAME_USE NameAndUse;
	BOOL success = ::LookupAccountSidW(nullptr, pSid, nullptr, &NameLength, nullptr, &DomainLength, &NameAndUse);
	if (!success)
	{
		DWORD err = ::GetLastError();
		if (err == ERROR_INSUFFICIENT_BUFFER)
		{
			std::unique_ptr<wchar_t []> Name(std::make_unique<wchar_t[]>(NameLength));
			std::unique_ptr<wchar_t []> Domain(std::make_unique<wchar_t[]>(DomainLength));

			success = ::LookupAccountSidW(nullptr, pSid, Name.get(), &NameLength, Domain.get(), &DomainLength, &NameAndUse);
			if (success)
			{
				user.append(to_utf8(Domain.get())).append("\\").append(to_utf8(Name.get()));
			}
			else
			{
				err = ::GetLastError();
				THROW_(SystemException, err);
			}
		}
	}
	// else: should never happen 
	return user;
}

std::string to_string(const Timestamp &ts) 
{ 
	FILETIME ft;
	ft.dwHighDateTime = (ts.timestamp & uint64_t(0xffffffff00000000)) >> 32;
	ft.dwLowDateTime = ts.timestamp & 0xffffffff;
	return to_string(ft);
}

} // namespace Windows