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

#ifndef TSI_INCLUDED_WINDOWS_H
#define TSI_INCLUDED_WINDOWS_H
#include <Windows.h>
#endif 

#include <cstddef>
#include <memory>
#include <string>

#include <process.h>

#include "Exception.h"
#include "RefObject.h"


namespace Windows
{

struct UseWideChar_t {};

constexpr UseWideChar_t useWideChar{};

class HResult
{
public:
	static HResult fromWin32(DWORD err)

	{
		return HRESULT_FROM_WIN32(err);
	}

	static HResult fromLastError()
	{
		return fromWin32(::GetLastError());
	}

	HResult() = default;

	constexpr HResult(HRESULT hr)
		: mHResult(hr)
	{}

	HResult(const HResult &rhs) = default;

	/* ~HResult() = default; */

	HResult &operator=(const HResult &rhs) = default;

	bool succeeded() const
	{
		return SUCCEEDED(mHResult);
	}

	bool failed() const
	{
		return FAILED(mHResult);
	}

	HRESULT value() const
	{
		return mHResult;
	}

private:
	HRESULT mHResult;
};

// Windows API system error codes.
class SysErr
{
public:
	static SysErr getLast() { return SysErr(::GetLastError()); }

	SysErr() = default;

	constexpr SysErr(DWORD err) noexcept
		: mErr(err)
	{}

	SysErr(const SysErr &rhs) = default;

	SysErr &operator=(const SysErr &rhs) = default;

	SysErr &operator=(DWORD code) noexcept
	{
		mErr = code;
		return *this;
	}

	DWORD getCode() const noexcept
	{
		return mErr;
	}

	bool succeeded() const noexcept
	{
		return mErr == 0;
	}

	bool failed() const noexcept
	{
		return mErr != 0;
	}

	explicit operator bool() const noexcept
	{
		return failed();
	}

	std::string formatMessage() const;

private:
	DWORD mErr;
};

inline bool operator==(const SysErr &lhs, const SysErr &rhs) { return lhs.getCode() == rhs.getCode(); }
inline bool operator!=(const SysErr &lhs, const SysErr &rhs) { return lhs.getCode() != rhs.getCode(); }
inline bool operator==(const SysErr &lhs, DWORD rhs) { return lhs.getCode() == rhs; }
inline bool operator!=(const SysErr &lhs, DWORD rhs) { return lhs.getCode() != rhs; }
inline bool operator==(DWORD lhs, const SysErr &rhs) { return lhs == rhs.getCode(); }
inline bool operator!=(DWORD lhs, const SysErr &rhs) { return lhs != rhs.getCode(); }

class SystemError : public Exception
{
	SysErr mErr{};
public:
	SystemError(const char *file, int line, DWORD err) noexcept
		: Exception(file, line), mErr(err)
	{}

	explicit SystemError(DWORD err) noexcept
		: mErr(err)
	{}

	SystemError(const char *file, int line, SysErr err) noexcept 
		: Exception(file, line), mErr(err)
	{}

	explicit SystemError(SysErr err) noexcept 
		: mErr(err)
	{}

	~SystemError() = default;
	SystemError(const SystemError &rhs) = default;

	SysErr getError() const noexcept { return mErr; }

	std::string formatMessage() const { return mErr.formatMessage(); }

private:
	// Nope.
	SystemError &operator=(const SystemError &) = delete;
};

class HResultError : public Exception
{
	HRESULT mHResult;
public:
	HResultError(HRESULT hr) noexcept : mHResult(hr) {}

	HResultError(const char *file, int line, HRESULT hr) noexcept
		: Exception(file, line), mHResult(hr)
	{}

	HResultError(const HResultError &rhs)
		: Exception(rhs), mHResult(rhs.mHResult)
	{}

	~HResultError() = default;

	HRESULT getResult() const { return mHResult; }

private:
	HResultError &operator=(const HResultError &) = delete;
};

class Guid
{
	GUID value{};
public:

	friend void swap(Guid &, Guid &);
	static std::string to_string(const GUID  &g);
	static std::wstring to_wstring(const GUID &g);

	constexpr Guid() noexcept {}
	Guid(const Guid &) = default;
	Guid &operator=(const Guid &) = default;
	~Guid() = default;

	constexpr Guid(const GUID &g) noexcept 
		: value(g) 
	{}

	Guid &operator=(const GUID &g)
	{
		value = g;
		return *this;
	}

	explicit Guid(const wchar_t *wsz);

	std::string to_string() const;

	const GUID &get() const { return value; }
	GUID &get() { return value; }
};

std::string to_string(Guid g);

inline void swap(Guid &a, Guid &b)
{
	Guid tmp(a);
	a = b;
	b = tmp;
}

class FileTime
{
	FILETIME ft;
public:
	constexpr FileTime() noexcept
		: ft{} {}

	constexpr FileTime(FILETIME ft) noexcept
		: ft(ft) {}

	constexpr FileTime(ULONGLONG ull) noexcept
		: ft{ DWORD(ull & 0xFFFFFFFFull), DWORD((ull & 0xFFFFFFFF00000000ull) >> 32ull) }
	{}

	FileTime(const FileTime &) = default;
	FileTime(FileTime &&) = default;
	FileTime &operator=(const FileTime &) = default;
	FileTime &operator=(FileTime &&) = default;
	~FileTime() = default;

	static FILETIME to_FILETIME(uint64_t t)
	{
		FILETIME ft{};
		ft.dwLowDateTime  = (t & 0xFFFFFFFF);
		ft.dwHighDateTime = (t >> 32);
		return ft;
	}

	static uint64_t to_uint64(FILETIME ft)
	{
		return (uint64_t(ft.dwHighDateTime) << 32) | 
			(uint64_t(ft.dwLowDateTime));

	}

	uint64_t to_uint64() const noexcept
	{
		return FileTime::to_uint64(ft);
	}

	static std::string to_string(uint64_t ft);
	std::string to_string() const;

	friend bool operator<(const FileTime &, const FileTime &) noexcept;
	friend bool operator>(const FileTime &, const FileTime &) noexcept;
	friend bool operator>=(const FileTime &, const FileTime &) noexcept;
	friend bool operator<=(const FileTime &, const FileTime &) noexcept;
	friend bool operator==(const FileTime &, const FileTime &) noexcept;
	friend bool operator!=(const FileTime &, const FileTime &) noexcept;
};

std::string to_string(const FILETIME &ft);
std::string to_string(const FileTime &ft);


inline bool operator<(const FileTime &a, const FileTime &b) noexcept
{
	return a.to_uint64() < b.to_uint64();
}

inline bool operator>(const FileTime &a, const FileTime &b) noexcept
{
	return a.to_uint64() > b.to_uint64();
}

inline bool operator>=(const FileTime &a, const FileTime &b) noexcept
{
	return a.to_uint64() >= b.to_uint64();
}

inline bool operator<=(const FileTime &a, const FileTime &b) noexcept
{
	return a.to_uint64() <= b.to_uint64();
}

inline bool operator==(const FileTime &a, const FileTime &b) noexcept
{
	return a.to_uint64() == b.to_uint64();
}

inline bool operator!=(const FileTime &a, const FileTime &b) noexcept
{
	return a.to_uint64() != b.to_uint64();
}

class SystemTime
{
	SYSTEMTIME st;
public:
	static SystemTime getSystemTimeUTC();
	static SystemTime getSystemTimeLocal();

	constexpr SystemTime() noexcept
		: st{}
	{}

	constexpr SystemTime(const SYSTEMTIME &st) noexcept
		: st{ st }
	{}

	SystemTime(const SystemTime &) = default;
	SystemTime(SystemTime &&) = default;
	SystemTime &operator=(const SystemTime &) = default;
	SystemTime &operator=(SystemTime &&) = default;
	~SystemTime() = default;

	static std::string format(const SYSTEMTIME &st);

	std::string to_string() const;
};

std::string to_string(const SYSTEMTIME &st);

enum class WaitStatus : DWORD
{
	Abandoned = WAIT_ABANDONED, /**< TODO */
	IoCompletion = WAIT_IO_COMPLETION, /**< TODO */
	Object_0 = WAIT_OBJECT_0, /**< Object signaled */
	Timeout = WAIT_TIMEOUT, /**< Timeout */
	Failed = WAIT_FAILED /**< Error. */
};

WaitStatus to_WaitStatus(DWORD status);

class WaitResult final
{
	WaitStatus mStatus;
	SysErr mLastErr;
public:

	// Create a WaitResult from the status returned by WaitFor* system API.
	static WaitResult make(DWORD status);

	WaitResult &operator=(const WaitResult &) = default;

	// Return the status of the wait operation.
	WaitStatus getStatus() const { return mStatus; }

	// Return the error, if any, associated with the operation. 
	// Usually only useful if the status is Failed.
	SysErr getError() const { return mLastErr; }

	// Unconditionally throw a SystemError with the contained error code.
	[[noreturn]] void throwError();

};

class WaitableHandle final
{
public:
	static WaitResult wait(HANDLE h, DWORD timeout, BOOL alertable) noexcept;

private:
	WaitableHandle() = delete;
	WaitableHandle(const WaitableHandle &) = delete;
	WaitableHandle(WaitableHandle &&) = delete;
	~WaitableHandle() = delete;
	WaitableHandle &operator=(const WaitableHandle &) = delete;
	WaitableHandle &operator=(WaitableHandle &&) = delete;
};

class ObjectHandle
{
	HANDLE mHandle = nullptr;
public:
	static HANDLE duplicate(HANDLE hSourceProcess, HANDLE hSourceHandle,
		HANDLE hTargetProcess, DWORD access, BOOL inherit,
		DWORD flags);

	static HANDLE duplicate(HANDLE h, DWORD access = 0, BOOL inherit = FALSE, DWORD flags = DUPLICATE_SAME_ACCESS);

	constexpr explicit ObjectHandle(HANDLE h = nullptr)
		: mHandle(h)
	{}

	ObjectHandle(ObjectHandle &&rhs) noexcept
		: mHandle(rhs.mHandle)
	{
		rhs.mHandle = nullptr;
	}

	~ObjectHandle();

	ObjectHandle &operator=(ObjectHandle &&rhs) noexcept;

	bool isNull() const
	{
		return mHandle == nullptr;
	}

	bool isNullOrInvalid() const
	{
		return mHandle == nullptr || mHandle == INVALID_HANDLE_VALUE;
	}

	HANDLE handle() const
	{
		return mHandle;
	}

	explicit operator bool() const
	{
		return !isNullOrInvalid();
	}

	operator HANDLE() const
	{
		return mHandle;
	}

};

class IRunnable : public IRefObject
{
public:
	virtual ~IRunnable() = default;
	virtual void run() = 0;
};

// A more flexible thread class that let's us use Windows specific goodies.
// The thread object is a proxy for a thing, not the thing itself, so 
// we are a little looser than std::thread about some semantics. 
// For example, move assignment of a thread into one currently representing 
// a running thread is just fine, if that's what you *really* want to do you can. 
// It amounts to closing the one thread handle overwriting the variable with
// another thread handle. Meh. Not a reason to terminate!
class Thread
{
public:

	// Thread object has a real handle, not pseudo-handle.
	static Thread current();

	// Spin-up a new thread.
	static Thread begin(unsigned(__stdcall *pfn)(void *), void *arg);

	// Default constructed thread is empty. 
	Thread() = default;

	// Move construct. From rhs, which becomes empty.
	Thread(Thread &&rhs) noexcept;

	// Assign from rhs, which becomes empty. 
	Thread &operator=(Thread &&rhs) noexcept;

	~Thread() = default;

	// Create an independent thread object representing the same thread as *this.
	Thread duplicate() const;

	// Wait for this thread to exit.
	WaitResult wait(DWORD timeout = INFINITE, BOOL alertable = FALSE) noexcept;

	// Wait forever for this thread to exit.
	void join();

private:

	Thread(HANDLE hThread, DWORD dwThreadId)
		: mhThread(hThread), mThreadId(dwThreadId)
	{}

	// Nope. duplicate provides similar functionality.
	Thread(const Thread &) = delete;
	Thread &operator=(const Thread &) = delete;

private:

	ObjectHandle mhThread{};
	DWORD mThreadId{};

	static unsigned __stdcall objectMain_Runnable(void *arg);
	static HANDLE beginThreadEx(void *_Security, unsigned _StackSize, _beginthreadex_proc_type _StartAddress, void *_ArgList, unsigned _InitFlag, unsigned *_ThrdAddr);

};

class Semaphore
{
	ObjectHandle mhSemaphore;
	static HANDLE create(LONG initial, LONG max, LPCWSTR name = nullptr);

public:

	// \param [in] inital
	// The inital count
	// \param [in] max
	// The maximum count.
	// \param [in] name
	// The optional name for the semaphore.
	Semaphore(LONG initial, LONG max, LPCWSTR name = nullptr);

	~Semaphore() = default;

	// Increments the semaphore.
	// \param [in] releaseCount
	// \param [in] previousCount
	SysErr release(LONG releaseCount = 1, LONG *previousCount = nullptr);

	// Throws on error.
	WaitResult wait(DWORD timeout = INFINITE, BOOL alertable = FALSE);

private:

	// Nope. 
	Semaphore() = delete;
	Semaphore(const Semaphore &) = delete;

};

// Wrapper around CRITICAL_SECTION.
class CriticalSection
{
	CRITICAL_SECTION mCS;
public:

	// Scoped lock. 
	class Lock
	{
		CriticalSection &cs;
	public:
		// Enters the critical section
		// \param [in] cs
		// Reference to the critical section to enter.
		explicit Lock(CriticalSection &cs) noexcept : cs(cs) { cs.enter(); }

		// Leaves the critical section
		~Lock() noexcept { cs.leave(); }
	};

	// Initializes the critical section. 
	CriticalSection();

	// @see InitializeCriticalSectionEx
	explicit CriticalSection(LONG spinCount, DWORD flags = 0);

	// @see DeleteCriticalSection
	~CriticalSection();

	// Enter the critical section.
	// Use Lock instead.
	void enter();

	// Try to enter the critical section,
	// @returns true if successful, false otherwise.
	bool tryEnter();

	// Leave the critical section. 
	// Use Lock instead.
	void leave();

private:
	CriticalSection(const CriticalSection &) = delete;
	CriticalSection &operator=(const CriticalSection &) = delete;
};

class Event
{
	ObjectHandle mhEvent;

	static HANDLE create(BOOL bManualReset, BOOL bInitialState, LPCWSTR name);

	static HANDLE create(BOOL bManualReset, BOOL bInitialState, LPCSTR name);

public:
	Event(BOOL bManualReset, BOOL bInitialState, LPCWSTR name = nullptr);

	Event(BOOL bManualReset, BOOL bInitialState, LPCSTR name);

	Event(Event &&rhs) = default;
	Event &operator=(Event &&rhs) = default;
	~Event() = default;

	void reset();

	void set();

	// Waits for the event to become signaled. Does not throw. 
	// \return 
	// Wait result. 
	WaitResult wait(DWORD timeout = INFINITE, BOOL alertable = FALSE) noexcept;

	// Duplicates the handle, but refers to the same kernel object. This is useful for
	// managing handles shared between threads. Each thread can have it's own
	// handle to the object (in this case an event) refering to the same kernel 
	// object, which keep the kernel object alive as long as necessary. 
	Event duplicate() const;

private:
	explicit Event(HANDLE h) noexcept
		: mhEvent(h)
	{}

	Event(const Event &) = delete;
	Event &operator=(const Event &) = delete;
};

class ManualResetEvent
{
	Event mEvent;
public:
	explicit ManualResetEvent(BOOL initialState, LPCSTR name = nullptr);

	explicit ManualResetEvent(BOOL initialState, LPCWSTR name = nullptr);

	ManualResetEvent(ManualResetEvent &&rhs) = default;
	ManualResetEvent &operator=(ManualResetEvent &&rhs) = default;
	~ManualResetEvent() = default;

	ManualResetEvent duplicate() const;

	void set();

	void reset();

	WaitResult wait(DWORD timeout = INFINITE, BOOL alertable = FALSE) noexcept;

private:
	explicit ManualResetEvent(Event ev);
};

class AutoResetEvent
{
	Event mEvent;
public:
	explicit AutoResetEvent(BOOL initialState, LPCWSTR name = nullptr);

	explicit AutoResetEvent(BOOL initialState, LPCSTR name);

	AutoResetEvent(AutoResetEvent &&rhs) = default;

	AutoResetEvent &operator=(AutoResetEvent &&rhs) = default;

	~AutoResetEvent() = default;

	AutoResetEvent duplicate() const;

	void set();

	WaitResult wait(DWORD timeout = INFINITE, BOOL alertable = FALSE) noexcept;

private:
	explicit AutoResetEvent(Event ev);
};

class SystemInfo : public SYSTEM_INFO
{
public:
	SystemInfo()
	{
		::GetSystemInfo(this);
	}
	~SystemInfo() = default;
	SystemInfo(const SystemInfo &) = default;
	SystemInfo(SystemInfo &&) = default;
	SystemInfo &operator=(const SystemInfo &) = default;
	SystemInfo &operator=(SystemInfo &&) = default;
};

std::string lookupAccount(PSID pSid);

} // namespace Windows