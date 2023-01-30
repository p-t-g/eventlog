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

#include "ChannelPathEnumerator.h"
#include "EvtHandle.h"

namespace Windows::EventLog
{

class ChannelPathEnumeratorImpl
{
public:
	ChannelPathEnumeratorImpl()
		: mEnumHandle{ChannelEnumHandle::open()}
		, mCurrentItem{}
	{}

	ChannelEnumHandle mEnumHandle;
	std::string mCurrentItem;
};

Ref<IChannelPathEnumerator> ChannelPathEnumerator::create()
{
	return RefObject<ChannelPathEnumerator>::createRef();
}

ChannelPathEnumerator::ChannelPathEnumerator()
	: d_ptr{std::make_unique<ChannelPathEnumeratorImpl>()}
{
}

ChannelPathEnumerator::~ChannelPathEnumerator()
{}

bool ChannelPathEnumerator::next()
{
	bool haveValue = false;
	auto path = d_ptr->mEnumHandle.nextChannelPath();
	if (path.has_value())
	{
		d_ptr->mCurrentItem = std::move(path.value());
		haveValue = true;
	}
	return haveValue;
}

std::string ChannelPathEnumerator::getCurrent() const
{
	return d_ptr->mCurrentItem;
}

Ref<IChannelPathEnumerator> IChannelPathEnumerator::create()
{
	return RefObject<ChannelPathEnumerator>::createRef();
}

}