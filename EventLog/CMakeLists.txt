cmake_minimum_required(VERSION 3.22)

set(EVENTLOG_PUBLIC_HDR
	include/CommonTypes.h
	include/IChannelConfig.h
	include/IChannelPathEnumerator.h
	include/IEventLogQuery.h
	include/IEventMetadata.h
	include/IEventMetadataEnumerator.h
	include/IEventReader.h
	include/IEventRecord.h
	include/ILogInfo.h
	include/IPublisherEnumerator.h
	include/IPublisherMetadata.h
	include/Ref.h
	include/RefObject.h
	include/RefPtr.h
)

set(EVENTLOG_IMPL_HDR 
	src/Array.h
	src/ChannelConfig.h
	src/ChannelPathEnumerator.h
	src/EventLogQuery.h
	src/EventReader.h
	src/EventRecord.h
	src/EvtHandle.h
	src/EvtVariant.h
	src/Exception.h
	src/LogInfo.h
	src/PublisherEnumerator.h
	src/PublisherMetadata.h
	src/PublisherMetadataImpl.h
	src/Queues.h
	src/StringUtils.h
	src/WinSys.h
)

set(EVENTLOG_SRC 
	src/ChannelConfig.cpp
	src/ChannelPathEnumerator.cpp
	src/EventLogQuery.cpp
	src/EventReader.cpp
	src/EventRecord.cpp
	src/EvtHandle.cpp
	src/EvtVariant.cpp
	src/Exception.cpp
	src/LogInfo.cpp
	src/PublisherEnumerator.cpp
	src/PublisherMetadata.cpp
	src/StringUtils.cpp
	src/WinSys.cpp
)

# Organize the headers files a little in the VS IDE.
if (CMAKE_GENERATOR MATCHES "Visual Studio")
	source_group("Header Files/Public" FILES ${EVENTLOG_PUBLIC_HDR} )
	source_group("Header Files/Implementation" FILES ${EVENTLOG_IMPL_HDR})
endif()

add_library(eventlog ${EVENTLOG_PUBLIC_HDR} ${EVENTLOG_IMPL_HDR} ${EVENTLOG_SRC})

# target_link_libraries(tsi tvision)
target_include_directories(eventlog 
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> )

target_link_libraries(eventlog Wevtapi.lib)

if (MSVC)
    target_compile_options(eventlog PRIVATE /W4 /WX)
else()
    target_compile_options(eventlog PRIVATE -Wall -Wextra -pedantic -Werror)
endif()
