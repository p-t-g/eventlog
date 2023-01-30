# EventLog
EventLog is a C++ library wrapper around the Windows Event Log API for 
consumers. 

The Windows C API is not very convenient and the hope is that
this library provides a much more usable programming interface, as well
as some useful utilities. In particular, a full screen text based event
viewer (think 90's style DOS program) is planned. Why? Because I got sick
of debugging and diagnosing Windows systems over ssh connections with only
powershell or wevtutil for viewing event logs.

# EventLogCtl
EventLogCtl is a test harness that one day hopes to grow-up into a real program.
As the library is a wrapper, it didn't make sense to have extensive unit tests 
around mostly trivial unit classes, especially for a hobby project. Instead, 
eventlogctl is used to exercise the code and inspect the output.

It also serves as example code for now.

# Building
The build uses CMake. Obviously, the only possible target is Windows. There are
no options. 

To build, create a directory out of the source tree, cd into it then do `cmake ..\path\to\code` 
followed by `cmake --build .` 

# TODO
There are many things to do:
- Find and fix bugs 
- Code Design
	- Low-level exception type for Windows API return codes
	  can escape. Need to tidy this up.
- Features: 
	- Remote sessions.
- Documentation
- Build enhancements:
	- install
	- ci build for g++ 
- Better command line option handling in EventLogCtl
- ncurses or tvision (or similar) text client for viewing events.
