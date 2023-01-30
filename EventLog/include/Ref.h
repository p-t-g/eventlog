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

#include <memory>
#include <utility>

namespace Windows
{ 

template<typename T> class Ref;
template<typename T> Ref<T> adoptRef(T &r);
template<typename T>
class Ref
{
	T *p;
public:

	friend Ref adoptRef<T>(T &);
	template<typename U> friend class Ref;

	explicit Ref(T &r)
		: p(std::addressof(r))
	{
		p->retain();
	}

	Ref(const Ref &rhs)
		: p(rhs.p)
	{
		p->retain();
	}

	template<typename U>
	Ref(const Ref<U> &rhs)
		: p(rhs.p)
	{
		p->retain();
	}

	Ref(Ref &&rhs)
		: p(rhs.p)
	{
		rhs.p = nullptr;
	}

	template<typename U>
	Ref(Ref<U> &&rhs)
		: p(rhs.p)
	{
		rhs.p = nullptr;
	}

	~Ref()
	{
		auto tmp = std::exchange(p, nullptr);
		if (tmp)
			tmp->release();

	}

	Ref &operator=(T &r)
	{
		Ref<T>{r}.swap(*this);
		return *this;
	}

	Ref &operator=(const Ref &rhs)
	{
		Ref<T>{rhs}.swap(*this);
		return *this;
	}

	template<typename U> Ref &operator=(const Ref<U> &rhs)
	{
		Ref<T>{rhs}.swap(*this);
		return *this;
	}

	Ref &operator=(Ref &&rhs)
	{
		Ref<T>{std::move(rhs)}.swap(*this);
		return *this;
	}

	template<typename U> 
	Ref &operator=(Ref<U> &&rhs)
	{
		Ref<T>{std::move(rhs)}.swap(*this);
		return *this;
	}

	template<typename U>
	void swap(Ref<U> &b)
	{
		U *tmp = b.p;
		b.p = p;
		p = tmp;
	}

	T *operator->() const { return p; }
	T *ptr() const { return p; }
	T &get() const { return *p; }
	operator T &() const { return *p; }

private:
	struct Adopt_t {};
	static constexpr Adopt_t Adopt{}; 

	Ref(T &r, Adopt_t)
		: p(&r)
	{}

};

template<typename T>
void swap(Ref<T> &a, Ref<T> &b)
{
	a.swap(b);
}

template<typename T>
Ref<T> adoptRef(T &r)
{
	return Ref<T>(r, Ref<T>::Adopt);
}

}