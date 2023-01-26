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

#ifndef TSI_INCLUDED_WINDOWS_H
#define TSI_INCLUDED_WINDOWS_H
#include <Windows.h>
#endif 

#ifndef TSI_INCLUDED_WINEVT_H
#define TSI_INCLUDED_WINEVT_H
#include <winevt.h>
#endif

#include "CommonTypes.h"

#include <memory>
#include <optional>

namespace Windows::EventLog
{

struct CMemFree 
{ 
	void operator()(void *p);
};

template<typename U>
using CMemPtr = std::unique_ptr<U, CMemFree>;

using EvtVariantPtr = CMemPtr<EVT_VARIANT>;

EvtVariantPtr allocEvtVariant(size_t n);

namespace Variant 
{

inline bool isNull(const EVT_VARIANT &v) { return v.Type == EvtVarTypeNull; }

EvtVariantPtr allocStringVariant(const std::string &s);

std::string getString(const EVT_VARIANT & v);
std::optional<std::string> getMaybeString(const EVT_VARIANT &v);
EvtVariantPtr createString(const std::string &s);

std::string getAnsiString(const EVT_VARIANT &v);

int8_t getSByte(const EVT_VARIANT &v);
void setSByte(EVT_VARIANT &v, int8_t);

uint8_t getByte(const EVT_VARIANT &v);
std::optional<uint8_t> getMaybeByte(const EVT_VARIANT &v);
void setByte(EVT_VARIANT &v, uint8_t);

int16_t getInt16(const EVT_VARIANT &v);
void setInt16(EVT_VARIANT &v, int16_t);

uint16_t getUInt16(const EVT_VARIANT &v);
std::optional<uint16_t> getMaybeUInt16(const EVT_VARIANT &v);
void setUInt16(EVT_VARIANT &v, uint16_t);

int32_t getInt32(const EVT_VARIANT &v);
void setInt32(EVT_VARIANT &v, int32_t);

uint32_t getUInt32(const EVT_VARIANT &v);
std::optional<uint32_t> getMaybeUInt32(const EVT_VARIANT &v);
void setUInt32(EVT_VARIANT &v, uint32_t);

int64_t getInt64(const EVT_VARIANT &v);
std::optional<int64_t> getMaybeInt64(const EVT_VARIANT &v);
void setInt64(EVT_VARIANT &v, int64_t);

uint64_t getUInt64(const EVT_VARIANT &v);
std::optional<uint64_t> getMaybeUInt64(const EVT_VARIANT &v);
void setUInt64(EVT_VARIANT &v, uint64_t);

float getSingle(const EVT_VARIANT &v);
void setSingle(EVT_VARIANT &v, float);

double getDouble(const EVT_VARIANT &v);
void setDouble(EVT_VARIANT &v, double);

GUID getGuid(const EVT_VARIANT &v);
std::optional<GUID> getMaybeGuid(const EVT_VARIANT &v);

size_t getSizeT(const EVT_VARIANT &v);
void setSizeT(EVT_VARIANT &v, size_t s);

FILETIME getFileTime(const EVT_VARIANT &v);
std::optional<FILETIME> getMaybeFileTime(const EVT_VARIANT &v);

std::optional<Timestamp> getMaybeTimestamp(const EVT_VARIANT &v);
uint64_t getRawFileTime(const EVT_VARIANT &v);

void setFileTime(EVT_VARIANT &v, FILETIME ft);

SYSTEMTIME getSystemTime(const EVT_VARIANT &v);

EVT_HANDLE getEvtHandle(const EVT_VARIANT &v);

std::vector<std::string> getStringArray(const EVT_VARIANT &v);

bool getBool(const EVT_VARIANT &v);
void setBool(EVT_VARIANT &v, bool value);

std::string getTypeName(const EVT_VARIANT &v);
std::string to_string(const EVT_VARIANT &v);

int64_t getHexInt64(const EVT_VARIANT &v);
int32_t getHexInt32(const EVT_VARIANT &v);

}

} // namespace 