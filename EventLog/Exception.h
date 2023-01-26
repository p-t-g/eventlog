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

#include <exception>

// throw exception EX 
#define THROW(EX) throw EX (__FILE__, __LINE__)
#define THROW_(EX, ...) throw EX (__FILE__, __LINE__, __VA_ARGS__)

namespace Windows
{

// Base class for all exceptions.
class Exception : public std::exception 
{
public:
	const char *getThrowFile() const;
	int getThrowLine() const;

protected:
	// No changing what it points to, or the pointer! 
	const char *const mFile = nullptr;
	int mLine = 0;

	Exception() = default;
	
	Exception(const char *file, int line) noexcept;
	
	~Exception() = default;
	
	Exception(const Exception &rhs) = default;

private:
	// Nope. 
	Exception &operator=(const Exception &) = delete;
};

// Thrown when an method is called on an object that is in a
// state such that it is not valid to call the method. A common
// example is re-initializing an object that doesn't support it
class InvalidStateException : public Exception
{
public:
	InvalidStateException() = default;

	InvalidStateException(const char *file, int line);

	InvalidStateException(const InvalidStateException &rhs) = default;

private:
	InvalidStateException &operator=(const InvalidStateException &) = delete;
};

// Thrown when an function argument is invalid. 
class InvalidArgumentException : public Exception
{
public:
	InvalidArgumentException() = default;

	InvalidArgumentException(const char *file, int line);

	InvalidArgumentException(const InvalidArgumentException &rhs) = default;

private:
	InvalidArgumentException &operator=(const InvalidArgumentException &) = delete;

};

// Thrown when an index is out of bounds.
class IndexOutOfBoundsException : public Exception
{
public:
	IndexOutOfBoundsException() = default;

	IndexOutOfBoundsException(const char *file, int line);

	IndexOutOfBoundsException(const IndexOutOfBoundsException &rhs) = default;

private:
	IndexOutOfBoundsException &operator=(const IndexOutOfBoundsException &) = delete;
};

// Thrown when a requested or expected data type does not match the actual data type.
class InvalidDataTypeException : public Exception
{
public:
	InvalidDataTypeException() = default;

	InvalidDataTypeException(const char *file, int line);

	InvalidDataTypeException(const InvalidDataTypeException &rhs) = default;

private:
	InvalidDataTypeException &operator=(const InvalidDataTypeException &) = delete;
};

}