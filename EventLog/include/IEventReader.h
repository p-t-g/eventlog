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

#include "RefObject.h"
#include "IEventRecord.h"

namespace Windows::EventLog
{
// TODO: dumb name. Change to IEventLogReader
class IEventReader : public IRefObject
{
public:

	// Opens the channel with the given xpath query.
	static Ref<IEventReader> openChannel(const std::string &channel, 
		const std::string &queryText, Direction direction);

	// Opens using the given strutured query
	static Ref<IEventReader> openStructuredXML(const std::string &structuredQueryText, Direction direction);

	// Opens the given logfile with the given xpath query.
	static Ref<IEventReader> openFile(const std::string &filePath, 
		const std::string &queryText, Direction direction);

	virtual ~IEventReader() = default;
	
	// Returns the timeout in milliseconds for retrieving events. Default is INFINITE.
	virtual uint32_t getTimeout() const = 0;

	// Set the timeout in milliseconds for retrieving events. Default is INFINITE.
	virtual void setTimeout(uint32_t) = 0;

	// Moves to the next record and returns true if there is a record, or false
	// if the last record has been reached. e.g: 
	//     while (reader->next()) { ... }
	// returns true if a current record is available, false otherwise.
	virtual bool next() = 0;

	virtual Ref<IEventRecord> getRecord() const = 0;

	virtual void seek(int64_t position, SeekOption whence) = 0;
};

}