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

#include "IEventReader.h"
#include "IEventRecord.h"

#include <memory>

namespace Windows::EventLog
{

class EventReaderImpl;
class EventReader : public IEventReader
{
public:
	friend class RefObject<EventReader> ;

	static Ref<EventReader> openChannel(const std::string &channel, 
		const std::string &queryText, Direction direction);

	static Ref<EventReader> openFile(const std::string &channel, 
		const std::string &queryText, Direction direction);

	static Ref<EventReader> openStructuredXML(const std::string &structuredXML, 
		Direction direction);

	~EventReader();

	uint32_t getTimeout() const override;
	void setTimeout(uint32_t timeout) override;

	bool next() override;

	Ref<IEventRecord> getRecord() const override;

	void seek(int64_t position, SeekOption whence) override;

private:
	struct OpenChannel {};
	
	EventReader(OpenChannel, const std::string &channel,
		const std::string &queryText, Direction direction);

	struct OpenFile {};

	EventReader(OpenFile, const std::string &channel, 
		const std::string queryText, Direction direction);

	EventReader(const std::string &structuredQueryText, Direction direction);

	std::unique_ptr<EventReaderImpl> d_ptr;

private:
	EventReader(const EventReader &) = delete;
	EventReader &operator=(const EventReader &) = delete;
};

}