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

#include "PublisherEnumerator.h"

#include "EvtHandle.h"
#include "StringUtils.h"

namespace Windows::EventLog
{

class PublisherEnumeratorImpl
{
public:
	PublisherEnumHandle mhPublisherEnum = PublisherEnumHandle::open();
	std::wstring mCurrentItem;
};

RefPtr<IPublisherEnumerator> PublisherEnumerator::create()
{
	return RefObject<PublisherEnumerator>::create();
}

PublisherEnumerator::PublisherEnumerator()
	: d_ptr(std::make_unique<PublisherEnumeratorImpl>())
{}

PublisherEnumerator::~PublisherEnumerator()
{}

bool PublisherEnumerator::next()
{
	bool hasCurrent = false;
	auto publisher = d_ptr->mhPublisherEnum.nextPublisherId(0);
	if (publisher.has_value())
	{
		hasCurrent = true;
		d_ptr->mCurrentItem = std::move(publisher.value());
	}
	return hasCurrent;
}

std::string PublisherEnumerator::getCurrent() const
{
	return to_utf8(d_ptr->mCurrentItem);
}

std::wstring PublisherEnumerator::getCurrent(int) const
{
	return d_ptr->mCurrentItem;
}

Ref<IPublisherEnumerator> IPublisherEnumerator::create()
{
	return RefObject<PublisherEnumerator>::createRef();
}


}