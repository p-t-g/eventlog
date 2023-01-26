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

#include <string>

namespace Windows
{

std::string to_utf8(const wchar_t *wsz, size_t length);
std::string to_utf8(const wchar_t *wsz);
std::string to_utf8(const std::wstring &ws);

std::wstring to_utf16(const char *s, size_t length);
std::wstring to_utf16(const char *s);
std::wstring to_utf16(const std::string &s);

}

