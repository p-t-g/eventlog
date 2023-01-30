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

namespace Windows 
{ 

template<typename T> class RefPtr;
template<typename T> RefPtr<T> adopt(T *p);

// Borrowed from WebKit, with some intrusive_ptr and CComPtr.
template<typename T>
class RefPtr
{
	T *p;
public:
	constexpr RefPtr() noexcept 
		: p{nullptr}
	{}

	constexpr RefPtr(std::nullptr_t) noexcept 
		: p{nullptr}
	{}

	template<typename U>
	RefPtr(U *p) noexcept
		: p{p}
	{
		if (p)
			p->retain();
	}

	RefPtr(const RefPtr &rhs) noexcept
		: p(rhs.p)
	{
		if (p) 
			p->retain();
	}

	RefPtr(RefPtr &&rhs) noexcept
		: p(rhs.p)
	{
		rhs.p = nullptr;
	}

	~RefPtr() noexcept
	{
		if (p)
			p->release();
	}

	template<typename U>
	RefPtr(const RefPtr<U> &rhs) noexcept
		: p(rhs.p)
	{
		if (p)
			p->retain();
	}

	template<typename U>
	RefPtr(RefPtr<U> &&rhs) noexcept
		: p(rhs.p)
	{
		rhs.p = nullptr;
	}

	RefPtr &operator=(const RefPtr &rhs) noexcept
	{
		RefPtr(rhs).swap(*this);
		return *this;
	}

	template<typename U>
	RefPtr &operator=(const RefPtr<U> &rhs) noexcept
	{
		RefPtr<T>{rhs}.swap(*this);
		return *this;
	}

	template<typename U>
	RefPtr<T> &operator=(RefPtr<U> &&rhs) noexcept
	{
		RefPtr<T>{std::move(rhs)}.swap(*this);
		return *this;
	}

	RefPtr &operator=(T *rhs) noexcept
	{
		RefPtr<T>(rhs).swap(*this);
		return *this;
	}

	void swap(RefPtr &rhs) noexcept
	{
		T *tmp = rhs.p;
		rhs.p = p;
		p = tmp;
	}

	T *get() const noexcept 
	{
		return p; 
	}

	void reset() noexcept
	{
		RefPtr<T>().swap(*this);
	}

	void reset(T *ptr) noexcept
	{
		RefPtr<T>{ptr}.swap(*this);
	}

	T *detach() noexcept
	{
		T *ptr = p;
		p = nullptr;
		return ptr;
	}

	T &operator*() const noexcept 
	{
		return *p; 
	}

	T *operator->() const noexcept 
	{
		return p; 
	}

	explicit operator bool() const noexcept
	{
		return p != nullptr;
	}

	friend bool operator==(const RefPtr<T> &a, const RefPtr<T> &b) noexcept
	{
		return a.p == b.p;
	}

	friend bool operator!=(const RefPtr<T> &a, const RefPtr<T> &b) noexcept
	{
		return a.p != b.p;
	}

	friend RefPtr adopt<T>(T *);
	template<typename U> friend class RefPtr;

private:
	struct AdoptTag {};
	static constexpr AdoptTag Adopt{};

	RefPtr(T *p, AdoptTag)
		: p(p)
	{}
};

template<typename T>
void swap(RefPtr<T> &a, RefPtr<T> &b) noexcept
{
	a.swap(b);
}

template<typename T>
RefPtr<T> adopt(T *p)
{
	return RefPtr<T>(p, RefPtr<T>::Adopt);
}

}