cmake_minimum_required(VERSION 3.22)

set(EVENTLOGCTL_SRC
	eventlogctl.cpp)

add_executable(eventlogctl ${EVENTLOGCTL_SRC}) 
target_link_libraries(eventlogctl eventlog)

if (MSVC)
    target_compile_options(eventlogctl PRIVATE /W4)
else()
    target_compile_options(eventlogctl PRIVATE -Wall -Wextra -pedantic -Werror)
endif()
