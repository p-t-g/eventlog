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

#include "StringUtils.h"

#include "CommonTypes.h"
#include "WinSys.h"
#include <strsafe.h>

namespace Windows
{

std::string to_utf8(const std::wstring &ws)
{
	return to_utf8(ws.c_str(), ws.length());
}

std::string to_utf8(const wchar_t * wsz)
{
	if (!wsz)
		return {};

	size_t length = 0;
	HRESULT hr = StringCchLengthW(wsz, STRSAFE_MAX_CCH, &length);
	if (FAILED(hr))
	{
		THROW_(SystemException, hr);
	}

	return to_utf8(wsz, length);
}

std::string to_utf8(const wchar_t *wsz, size_t length)
{
	if (length == 0)
		return {};

	std::string u;

	int requiredLength = ::WideCharToMultiByte(CP_UTF8, 0, wsz, int(length), nullptr, 0, nullptr, nullptr);
	if (requiredLength > 0)
	{
		u.resize(requiredLength);

		requiredLength = ::WideCharToMultiByte(CP_UTF8, 0, wsz, int(length), &u[0], int(requiredLength), nullptr, nullptr);
		if (requiredLength == 0)
		{
			THROW_(SystemException, ::GetLastError());
		}
	}
	else
	{
		THROW_(SystemException, ::GetLastError());
	}

	return u;
}

std::wstring to_utf16(const char *sz, size_t length)
{
	if (!sz || length == 0)
		return {};

	std::wstring ws;

	int n = ::MultiByteToWideChar(CP_UTF8, 0, sz, int(length), nullptr, 0);
	if (n > 0)
	{
		ws.resize(n);
		n = ::MultiByteToWideChar(CP_UTF8, 0, sz, int(length), &ws[0], int(length));
		if (n == 0)
		{
			THROW_(SystemException, ::GetLastError());
		}
	}
	else
	{
		THROW_(SystemException, ::GetLastError());
	}
	return ws;
}

std::wstring to_utf16(const char *sz)
{
	if (!sz)
		return {};

	size_t length = 0;
	HRESULT hr = StringCchLengthA(sz, STRSAFE_MAX_CCH, &length);
	if (FAILED(hr))
	{
		THROW_(SystemException, hr);
	}
	return to_utf16(sz, length);
}

std::wstring to_utf16(const std::string &s)
{
	return to_utf16(s.c_str(), s.length());
}

} // namespace Windows