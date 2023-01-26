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

#include "Exception.h"

#include <memory>

namespace Windows
{

class ArrayIndexOutOfBoundsError : public Exception
{};

struct NoOp_t
{ 
	template<class...Args>
	constexpr void operator()(Args&&...) const {} 
};

// Generic heap allocated array that is easy to use with C APIs accepting array
// parameters, which std::vector<> often is not. Not intended as a generic 
// collection. Use std::vector for that.
// \tparam T
// Element type. Must be default constructible at a minimum. 
// \tparam Destroyer
// Functor the accepts T param to destroy it. e.g. to close a handle.
// Applied to every element when the array is destroyed.
// TODO: Parameterize memory management.
//        
template<typename T, typename Destroyer = NoOp_t>
class Array
{
	uint32_t mSize;
	std::unique_ptr<T[]> mElements;
public:

	constexpr Array() noexcept
		: mSize(0), mElements{}
	{}

	// Probably this has to be replaced with:
	// constexpr Array(DWORD, T*)
	// The we need make_array<T, F>(DWORD, F alloc) {  }
	// 
	explicit Array(uint32_t size)
		: mSize(size)
		, mElements(std::make_unique<T[]>(size))
	{		
	}

	Array(uint32_t size, std::unique_ptr<T[]> a) noexcept
		: mSize(size), mElements(std::move(a))
	{
	}

	// Move constructor. On return, the movee is empty and can be 
	// reused (e.g. as target of move).
	// \param [in] rhs
	Array(Array &&rhs) noexcept
		: mSize(rhs.mSize)
		, mElements(std::move(rhs.mElements))
	{
		rhs.mElements = 0;
	}

	// Move assignment. On return, the movee is empty and can be 
	// reused (e.g. as target of move).
	// \param [in] rhs
	Array &operator=(Array &&rhs) noexcept
	{
		mSize = rhs.mSize;
		mElements = std::move(rhs.mElements);
		rhs.mSize = 0;
		return *this;
	}

	~Array() noexcept
	{ 
		destroyElements();
	}

	T &operator[](size_t index)
	{
		if (index >= mSize) throw ArrayIndexOutOfBoundsError();
		return mElements[index];
	}

	const T &operator[](size_t index) const
	{
		if (index >= mSize) throw ArrayIndexOutOfBoundsError();
		return mElements[index];
	}

	size_t size() const noexcept
	{
		return mSize;
	}

	explicit operator T *() noexcept
	{
		return mElements.get();
	}

	explicit operator const T *() const noexcept
	{
		return mElements.get();
	}

	// Destroys and default constructs each element.
	// Does not change the size. 
	void sweep()
	{
		sweepElements();
	}

private:

	template<typename U = Destroyer>
	void destroyElements(std::enable_if_t< std::negation_v< std::is_same<U, NoOp_t> > > * = nullptr)
	{
		if (!mElements) 
			return;
		Destroyer destroy{};
		for (uint32_t i = 0; i < mSize; ++i)
		{
			destroy(mElements[i]);
		}
	}

	template<typename U = Destroyer>
	void sweepElements(std::enable_if_t< std::negation_v< std::is_same<U, NoOp_t> > > * = nullptr)
	{
		if (!mElements) 
			return;
		Destroyer destroy{};
		for (uint32_t i = 0; i < mSize; ++i)
		{
			destroy(mElements[i]);
			new (&mElements[i]) T{};
		}
	}

	template<typename U = Destroyer >
	void destroyElements(std::enable_if_t< std::is_same_v<U, NoOp_t> > * = nullptr)
	{
		// Nothing.
	}

	template<typename U = Destroyer >
	void sweepElements(std::enable_if_t< std::is_same_v<U, NoOp_t> > * = nullptr)
	{
		// Nothing.
	}

	// TODO: We could support copy, even if we don't need it ATM ...
	Array(const Array &) = delete;
	Array &operator=(const Array &) = delete;
};

template<typename T, typename D>
T *ptr(Array<T, D> &a) 
{
	return static_cast<T *>(a);
}

template<typename T, typename D>
const T *ptr(const Array<T, D> &a)
{
	return static_cast<const T *>(a);
}

}