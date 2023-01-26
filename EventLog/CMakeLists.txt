cmake_minimum_required(VERSION 3.22)

set(EVENTLOG_HDR 
	Array.h
	ChannelConfig.h
	ChannelPathEnumerator.h
	CommonTypes.h
	EventLogQuery.h
	EventReader.h
	EventRecord.h
	EvtHandle.h
	EvtVariant.h
	Exception.h
	IChannelConfig.h
	IChannelPathEnumerator.h
	IEventLogQuery.h
	IEventMetadata.h
	IEventMetadataEnumerator.h
	IEventReader.h
	IEventRecord.h
	ILogInfo.h
	IPublisherEnumerator.h
	IPublisherMetadata.h
	LogInfo.h
	PublisherEnumerator.h
	PublisherMetadata.h
	PublisherMetadataImpl.h
	Queues.h
	Ref.h
	RefObject.h
	RefPtr.h
	WinSys.h
	StringUtils.h
)

set(EVENTLOG_SRC 
	ChannelConfig.cpp
	ChannelPathEnumerator.cpp
	EventLogQuery.cpp
	EventReader.cpp
	EventRecord.cpp
	EvtHandle.cpp
	EvtVariant.cpp
	Exception.cpp
	LogInfo.cpp
	PublisherEnumerator.cpp
	PublisherMetadata.cpp
	WinSys.cpp
	StringUtils.cpp
)

add_library(eventlog ${EVENTLOG_HDR} ${EVENTLOG_SRC})

# target_link_libraries(tsi tvision)
target_include_directories(eventlog INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(eventlog Wevtapi.lib)

if (MSVC)
    target_compile_options(eventlog PRIVATE /W4)
else()
    target_compile_options(eventlog PRIVATE -Wall -Wextra -pedantic -Werror)
endif()
