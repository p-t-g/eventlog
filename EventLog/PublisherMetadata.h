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
#include <string>
#include <vector>
#include <optional>
#include <memory>

#include "RefObject.h"
#include "CommonTypes.h"
#include "EventRecord.h"
#include "IPublisherMetadata.h"

namespace Windows::EventLog 
{

class EventRecordHandle;

class PublisherMetadataImpl;
class PublisherMetadata : public IPublisherMetadata
{
public:
	friend class RefObject<PublisherMetadata>; 

	~PublisherMetadata();

	// Can return RefPtr(nullptr).
	static RefPtr<PublisherMetadata> cacheOpenProvider(const std::string &id);

	static Ref<IPublisherMetadata> openProvider(const std::string &id);
	static Ref<IPublisherMetadata> openArchiveLogFile(const std::string &id, const std::string &filePath);

	static Ref<PublisherMetadata> create(const std::string &id);
	static Ref<PublisherMetadata> create(const std::string &id, const std::string &filePath);

	std::optional<GUID> getPublisherGuid() const override;

	// EvtPublisherMetadataResourceFilePath
	std::optional<std::string> getResourceFilePath() const override;

	// EvtPublisherMetadataParameterFilePath
	std::optional<std::string> getParametersFilePath() const override;

	// EvtPublisherMetadataMessageFilePath
	std::optional<std::string> getMessageFilePath() const override;

	// EvtPublisherMetadataHelpLink
	std::optional<std::string> getHelpLink() const override;

	// EvtPublisherMetadataPublisherMessageID
	std::optional<uint32_t> getPublisherMessageId() const override;

	std::string getPublisherMessage() const override;

	// EvtPublisherMetadataChannelReferences
	Ref<IPublisherChannelArray> getChannels() const override;

	// EvtPublisherMetadataLevels
	Ref<IPublisherLevelArray> getLevels() const override;

	// EvtPublisherMetadataTasks
	Ref<IPublisherTaskArray> getTasks() const override;

	// EvtPublisherMetadataOpcodes
	Ref<IPublisherOpcodeArray> getOpcodes() const override;

	// EvtPublisherMetadataKeywords
	Ref<IPublisherKeywordArray> getKeywords() const override;

	Ref<IEventMetadataEnumerator> openEventMetadataEnum() const override;

	std::string formatMessage(uint32_t messageID) const override;

	std::string lookupChannelDisplay(uint32_t channelValue) const override;
	std::string lookupLevelDisplay(uint32_t levelValue) const override;
	std::string lookupTaskDisplay(uint32_t taskValue) const override;
	std::string lookupOpcodesDisplay(uint32_t op_task) const override;
	std::vector<std::string> lookupKeywordsDisplay(uint64_t maskBits) const override;

	FormattedEventRecord format(const EventRecordHandle &recordHandle) const;
	
	static FormattedEventRecord formatEvent(const EventRecordHandle &recordHandle);

private:
	// Hide the messy details behind PImpl.
	std::unique_ptr<PublisherMetadataImpl> d_ptr;

private:
	explicit PublisherMetadata(const std::string &providerId);
	PublisherMetadata(const std::string &providerId, const std::string &filePath);

	// No copy.
	PublisherMetadata(const PublisherMetadata &) = delete;
	PublisherMetadata &operator=(const PublisherMetadata &) = delete;
};


} // Windows::EventLog