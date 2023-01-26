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

#include <optional>
#include <string>

#include "CommonTypes.h"
#include "EvtHandle.h"
#include "PublisherMetadata.h"

namespace Windows::EventLog
{ 

//
// PublisherMetadataImpl
//

class PublisherMetadataImpl
{
public:
	explicit PublisherMetadataImpl(PublisherMetadataHandle publisherMetaHandle);
	~PublisherMetadataImpl() = default;
	FormattedEventRecord format(const EventRecordHandle &h) const;

	std::optional<GUID> mPublisherGuid{};
	std::optional<std::string> mResourceFilePath{};
	std::optional<std::string> mParametersFilePath{};
	std::optional<std::string> mMessageFilePath{};
	std::optional<std::string> mHelpLink{};
	std::optional<uint32_t> mMessageId{};
	std::string mMessage{};

	Ref<IPublisherChannelArray> mChannels;
	Ref<IPublisherLevelArray> mLevels;
	Ref<IPublisherTaskArray> mTasks;
	Ref<IPublisherOpcodeArray> mOpcodes;
	Ref<IPublisherKeywordArray> mKeywords;

	PublisherMetadataHandle mPublisherMetadataHandle{};

private:
	PublisherMetadataImpl(const PublisherMetadataImpl &) = delete;
	PublisherMetadataImpl(PublisherMetadataImpl &&) = delete;
	PublisherMetadataImpl &operator=(const PublisherMetadataImpl &) = delete;
	PublisherMetadataImpl &operator=(PublisherMetadataImpl &&) = delete;
};

}