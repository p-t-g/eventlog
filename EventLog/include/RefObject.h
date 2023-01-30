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

#include <atomic>
#include "RefPtr.h"
#include "Ref.h"

namespace Windows
{

// Common interface for ref counted objects.
class IRefObject
{
public:
	virtual ~IRefObject() = default;
	virtual void retain() const = 0;
	virtual void release() const = 0;
};

// Base implementation of ref counted object.
class RefObjectBase 
{
	mutable std::atomic_uint32_t mRefCount = 1;
public:
	virtual ~RefObjectBase()
	{}

	void retainImpl() const
	{ 
		mRefCount.fetch_add(1u, std::memory_order_relaxed);
	}

	void releaseImpl() const
	{
		auto cnt = mRefCount.fetch_sub(1u, std::memory_order_acq_rel);
		if (cnt == 1 /* was 1, now 0 so delete */)
		{
			delete this;
		}
	}
};

// Provides an implementation of IRefObject via CRTP
// Base must inherit from IRefObject (well, strictly speaking it must provide 
// virtual release() and retain()). 
template<typename Base>
class RefObject : public Base, private RefObjectBase 
{
public:

	template<typename ... Args>
	static RefPtr<Base> create(Args && ... args)
	{
		// If you get a compile error hear about cannot instantiate abstract class,
		// you missed implementing a method.
		Base *pObj = new RefObject<Base>(std::forward<Args>(args)...);
		return adopt(pObj);
	}

	template<typename ... Args>
	static Ref<Base> createRef(Args && ... args)
	{
		// If you get a compile error hear about cannot instantiate abstract class,
		// you missed implementing a method.
		Base *pObj = new RefObject<Base>(std::forward<Args>(args)...);
		return adoptRef(*pObj);
	}

	void retain() const override
	{
		RefObjectBase::retainImpl();
	}

	void release() const override
	{
		RefObjectBase::releaseImpl();
	}

private:
	// If you see a message like:
	// Can't access private member declared in class 'X' where X the type of Base
	// then you probably forgot the friend RefObject<X> declaration in X. 
	template<typename ... Args>
	RefObject(Args && ... args ) 
		: Base(std::forward<Args>(args)...)
	{}

	~RefObject() override 
	{}

private:
	// Nope
	RefObject(const RefObject &) = delete;
	RefObject(RefObject &&) = delete;
	RefObject &operator=(const RefObject &) = delete;
	RefObject &operator=(RefObject &&) = delete;
};

}