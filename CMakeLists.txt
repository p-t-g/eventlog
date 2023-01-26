cmake_minimum_required(VERSION  3.20)
project(evtlogview)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(tvision)
add_subdirectory(EventLog)
add_subdirectory(EventLogCtl)
