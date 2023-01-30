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

#include <cstdint>
#include <optional>
#include <array>

#include "WinSys.h"
// namespace MSWin
namespace Windows 
{

template<typename T, uint32_t N = 16u>
class BoundedQueue
{
public:

	static constexpr uint32_t MaxSize = N;

	BoundedQueue() = default;
	~BoundedQueue() = default;

	bool enqueue(T value)
	{
		bool ok = false;
		auto count = mCount + 1;
		if (count <= MaxSize)
		{ 
			auto index = (mHeadIndex + mCount) % MaxSize;

			mElement[index] = value;

			mCount = count;
			ok = true;
		}
		return ok;
	}

	std::optional<T> dequeue() 
	{
		if (mCount > 0)
		{
			auto index = mHeadIndex;
			mHeadIndex = (mHeadIndex + 1) % MaxSize;
			mCount -= 1;
			return mElement[index];
		}
		return {};
	}

	size_t size() const
	{
		return mCount;
	}

	bool isEmpty() const 
	{
		return size() == 0; 
	}

private:
	std::array<T, MaxSize> mElement{};
	uint32_t mHeadIndex = 0;
	uint32_t mCount = 0;

	BoundedQueue(const BoundedQueue &) = delete;
	BoundedQueue(BoundedQueue &&) = delete;
	BoundedQueue &operator=(const BoundedQueue &) = delete;
	BoundedQueue &operator=(BoundedQueue &&) = delete;
};

template<typename T, uint32_t N = 16>
class BoundedSynchQueue
{
public:
	static constexpr uint32_t MaxSize = N;

	BoundedSynchQueue() = default;

	~BoundedSynchQueue() = default;

	void enqueue(T value)
	{
		// Wait when no available slots.
		auto result = mAvail.wait();
		auto status = result.getStatus();
		if (status == WaitStatus::Object_0)
		{
			{
				CriticalSection::Lock lck(mCritSec);
				mQueue.enqueue(value);
			}

			// Increment used slots
			mOccupied.release();
		}
	}

	std::optional<T> dequeue()
	{
		std::optional<T> tmp;

		auto status = mOccupied.wait();
		if (status.getStatus() == WaitStatus::Object_0)
		{
			{
				CriticalSection::Lock lck(mCritSec);
				tmp = mQueue.dequeue();
			}

			// Increment available slots.
			mAvail.release();
		}

		return tmp;
	}

private:
	BoundedQueue<T, MaxSize> mQueue{};

	CriticalSection mCritSec{};

	// Semaphore on number of available slots
	Semaphore mAvail{LONG(MaxSize), LONG(MaxSize)};

	// Semaphore on number of occupied slots
	Semaphore mOccupied{0, LONG(MaxSize)};

private:
	BoundedSynchQueue(const BoundedSynchQueue &) = delete;
	BoundedSynchQueue(BoundedSynchQueue &&) = delete;
	BoundedSynchQueue &operator=(const BoundedSynchQueue &) = delete;
	BoundedSynchQueue &operator=(BoundedSynchQueue &&) = delete;
};

}
