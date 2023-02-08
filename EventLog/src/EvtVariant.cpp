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

#include "EvtVariant.h"

#include "WinSys.h"
#include "StringUtils.h"

namespace Windows::EventLog
{

static DWORD allocStringVariantImpl(const std::string &s, EVT_VARIANT **ppv) noexcept
{
	//
	// This converts s into a wide char string directly into the buffer
	// allocated for the EVT_VARIANT. It seems the easiest way to 
	// deal with correctness was to do it old-school. 
	//

	if (!ppv)
		return ERROR_INVALID_PARAMETER;
	
	*ppv = nullptr;

	DWORD err = ERROR_SUCCESS;
	if (!s.empty())
	{
		// How many character for the conversion? 
		int charCount = ::MultiByteToWideChar(CP_UTF8, 0, s.c_str(), int(s.length()), nullptr, 0);
		if (charCount > 0) // something to convert?
		{
			// The size, in bytes, of the buffer required to hold the variant 
			// and the string data.
			size_t size = sizeof(EVT_VARIANT) + ((size_t(charCount) + 1) * sizeof(wchar_t));

			uint8_t *buffer = static_cast<uint8_t *>(::malloc(size));
			EVT_VARIANT *v = reinterpret_cast<EVT_VARIANT *>(buffer);
			if (buffer) // malloc success? 
			{
				v->Type = EvtVarTypeString;
				v->Count = 0;

				// Pointer to the start of the string in the buffer.
				wchar_t *wsz = reinterpret_cast<wchar_t *>(buffer + sizeof(EVT_VARIANT));
				int convertedCharCount = ::MultiByteToWideChar(CP_UTF8, 0, s.c_str(), int(s.length()), wsz, charCount);
				if (convertedCharCount > 0)
				{
					// Boundary check. Arguably unnecessary, as we asked for 
					// the size already, but do it anyway. 
					if (convertedCharCount <= charCount)
					{ 
						wsz[convertedCharCount] = 0;

						v->Type = EvtVarTypeString;
						v->Count = 0;
						v->StringVal = wsz;

						*ppv = v;
					}
					else // Would be weird, but better than crashing.
					{
						err = ERROR_INVALID_PARAMETER;
						free(buffer);
					}
				}
				else // MBtoWC fail
				{
					err = ::GetLastError();
					::free(buffer);
				}
			}
			else // malloc fail
			{
				err = ERROR_OUTOFMEMORY;
			}
		}
		else // MBtoWC failed.
		{
			err = ::GetLastError();
		}
	}
	else
	{		
		size_t size = sizeof(EVT_VARIANT) + sizeof(wchar_t);

		uint8_t *buffer = static_cast<uint8_t *>(::malloc(size));
		EVT_VARIANT *v = reinterpret_cast<EVT_VARIANT *>(buffer);
		if (buffer)
		{
			v->Type = EvtVarTypeString;
			v->Count = 0;
			v->StringVal = reinterpret_cast<LPCWSTR>(buffer + sizeof(EVT_VARIANT));

			*ppv = v;
		}
		else
		{
			err = ERROR_OUTOFMEMORY;
		}
	}
	return err;
}

void CMemFree::operator()(void *p)
{ 
	::free(p); 
} 

template<typename T> CMemPtr<T> allocCMem(size_t n)
{
	auto p = ::calloc(1, n);
	if (!p) throw std::bad_alloc();
	return CMemPtr<T>(static_cast<T*>(p));
}

EvtVariantPtr allocEvtVariant(size_t n)
{
	return allocCMem<EVT_VARIANT>(n);
}

namespace Variant
{

EvtVariantPtr allocStringVariant(const std::string &s)
{
	EVT_VARIANT *pv = nullptr;
	DWORD err = allocStringVariantImpl(s, &pv);
	if (err) // never any allocation if error occurs.
	{
		if (err == ERROR_OUTOFMEMORY)
			throw std::bad_alloc();
		else
			THROW_(SystemException, err);
	}
	return EvtVariantPtr{pv};
}

std::optional<std::string> getMaybeString(const EVT_VARIANT &v)
{
	if (v.Type == EvtVarTypeString)
		return to_utf8(v.StringVal);
	if (v.Type == EvtVarTypeNull)
		return {};
	THROW(InvalidDataTypeException);
}

std::string getString(const EVT_VARIANT &v)
{
	bool expectedType = (v.Type == EvtVarTypeString);
	if (!expectedType)
	{
		THROW(InvalidDataTypeException);
	}
	return to_utf8(v.StringVal);
}

EvtVariantPtr createString(const std::string &s)
{
	return allocStringVariant(s);
}

std::string getAnsiString(const EVT_VARIANT &v)
{
	bool expectedType = (v.Type == EvtVarTypeAnsiString);
	if (!expectedType)
	{
		THROW(InvalidDataTypeException);
	}

	return v.AnsiStringVal ? std::string(v.AnsiStringVal) : std::string{};
}

// EvtVariantPtr createAnsiString(const std::string &s);

int8_t getSByte(const EVT_VARIANT &v)	
{
	bool expectedType = (v.Type == EvtVarTypeSByte);
	if (!expectedType)
	{
		THROW(InvalidDataTypeException);
	}
	return v.SByteVal;
}

void setSByte(EVT_VARIANT &v, int8_t value)
{
	if (v.Type != EvtVarTypeNull) 
	{
		THROW(InvalidArgumentException);
	}
	v.SByteVal = value;
	v.Count = 0;
	v.Type = EvtVarTypeSByte;
}

uint8_t getByte(const EVT_VARIANT &v)
{
	bool expectedType = (v.Type == EvtVarTypeByte);
	if (!expectedType)
	{
		THROW(InvalidDataTypeException);
	}
	return v.ByteVal;
}

std::optional<uint8_t> getMaybeByte(const EVT_VARIANT &v)
{
	if (v.Type == EvtVarTypeByte)
		return v.ByteVal;
	if (v.Type == EvtVarTypeNull)
		return {};
	THROW(InvalidDataTypeException);
}

void setByte(EVT_VARIANT &v, uint8_t value)
{
	if (v.Type != EvtVarTypeNull) 
	{
		THROW(InvalidArgumentException);
	}
	v.ByteVal = value;
	v.Count = 0;
	v.Type = EvtVarTypeByte;
}

int16_t getInt16(const EVT_VARIANT &v)
{
	bool expectedType = (v.Type == EvtVarTypeInt16);
	if (!expectedType)
	{
		THROW(InvalidDataTypeException);
	}
	return v.Int16Val;
}

void setInt16(EVT_VARIANT &v, int16_t value)
{
	if (v.Type != EvtVarTypeNull) 
	{
		THROW(InvalidArgumentException);
	}
	v.Int16Val = value;
	v.Count = 0;
	v.Type = EvtVarTypeInt16;
}

uint16_t getUInt16(const EVT_VARIANT &v)
{
	bool expectedType = (v.Type == EvtVarTypeUInt16);
	if (!expectedType)
	{
		THROW(InvalidDataTypeException);
	}
	return v.UInt16Val;
}

std::optional<uint16_t> getMaybeUInt16(const EVT_VARIANT &v)
{
	if (v.Type == EvtVarTypeUInt16)
		return v.UInt16Val;
	if (v.Type == EvtVarTypeNull)
		return {};
	THROW(InvalidDataTypeException);
}

void setUInt16(EVT_VARIANT &v, uint16_t value)
{
	if (v.Type != EvtVarTypeNull) 
	{
		THROW(InvalidArgumentException);
	}
	v.UInt16Val = value;
	v.Type = EvtVarTypeUInt16;
	v.Count = 0;
}

int32_t getInt32(const EVT_VARIANT &v)
{
	bool expectedType = (v.Type == EvtVarTypeInt32);
	if (!expectedType)
	{
		THROW(InvalidDataTypeException);
	}
	return v.Int32Val;
}

void setInt32(EVT_VARIANT &v, int32_t value)
{
	if (v.Type != EvtVarTypeNull) 
	{
		THROW(InvalidArgumentException);
	}
	v.Int32Val = value;
	v.Type = EvtVarTypeInt32;
	v.Count = 0;
}

uint32_t getUInt32(const EVT_VARIANT &v)
{
	bool expectedType = (v.Type == EvtVarTypeUInt32);
	if (!expectedType)
	{
		THROW(InvalidDataTypeException);
	}
	return v.UInt32Val;
}

std::optional<uint32_t> getMaybeUInt32(const EVT_VARIANT &v)
{
	if (v.Type == EvtVarTypeUInt32)
		return v.UInt32Val;
	if (v.Type == EvtVarTypeNull)
		return {};
	THROW(InvalidDataTypeException);
}

void setUInt32(EVT_VARIANT &v, uint32_t value)
{
	if (v.Type != EvtVarTypeNull) 
	{
		THROW(InvalidArgumentException);
	}
	v.UInt32Val = value;
	v.Type = EvtVarTypeUInt32;
	v.Count = 0;
}

int64_t getInt64(const EVT_VARIANT &v)
{
	bool expectedType = v.Type == EvtVarTypeInt64;
	if (!expectedType)
	{
		THROW(InvalidDataTypeException);
	}
	return v.Int64Val;
}

std::optional<int64_t> getMaybeInt64(const EVT_VARIANT &v)
{
	if (v.Type == EvtVarTypeInt64)
		return v.Int64Val;
	if (v.Type == EvtVarTypeNull)
		return {};
	THROW(InvalidDataTypeException);
}

void setInt64(EVT_VARIANT &v, int64_t value)
{
	if (v.Type != EvtVarTypeNull) 
	{
		THROW(InvalidArgumentException);
	}
	v.Int64Val = value;
	v.Type = EvtVarTypeInt64;
	v.Count = 0;
}

uint64_t getUInt64(const EVT_VARIANT &v)
{
	bool expectedType = v.Type == EvtVarTypeUInt64;
	if (!expectedType)
	{
		THROW(InvalidDataTypeException);
	}
	return v.UInt64Val;
}

std::optional<uint64_t> getMaybeUInt64(const EVT_VARIANT &v)
{
	if (v.Type == EvtVarTypeUInt64)
		return v.UInt64Val;
	if (v.Type == EvtVarTypeNull)
		return {};;
	THROW(InvalidDataTypeException);
}

void setUInt64(EVT_VARIANT &v, uint64_t value)
{
	if (v.Type != EvtVarTypeNull) 
	{
		THROW(InvalidArgumentException);
	}
	v.UInt64Val = value;
	v.Type = EvtVarTypeUInt64;
	v.Count = 0;
}

float getSingle(const EVT_VARIANT &v)
{
	bool expectedType = (v.Type == EvtVarTypeSingle);
	if (!expectedType)
	{
		THROW(InvalidDataTypeException);
	}
	return v.SingleVal;
}

void setSingle(EVT_VARIANT &v, float value)
{
	if (v.Type != EvtVarTypeNull) 
	{
		THROW(InvalidArgumentException);
	}
	v.SingleVal = value;
	v.Type = EvtVarTypeSingle;
	v.Count = 0;
}

double getDouble(const EVT_VARIANT &v)
{
	bool expectedType = (v.Type == EvtVarTypeDouble);
	if (!expectedType)
	{
		THROW(InvalidDataTypeException);
	}
	return v.DoubleVal;
}

void setDouble(EVT_VARIANT &v, double value)
{
	if (v.Type != EvtVarTypeNull) 
	{
		THROW(InvalidArgumentException);
	}
	v.DoubleVal = value;
	v.Type = EvtVarTypeDouble;
	v.Count = 0;
}

bool getBool(const EVT_VARIANT &v)
{
	bool expectedType = (v.Type == EvtVarTypeBoolean);
	if (!expectedType)
	{
		THROW(InvalidDataTypeException);
	}
	return v.BooleanVal == FALSE ? false : true;
}

void setBool(EVT_VARIANT &v, bool value)
{
	if (v.Type != EvtVarTypeNull) 
	{
		THROW(InvalidArgumentException);
	}
	v.BooleanVal = value == false ? FALSE : TRUE;
	v.Type = EvtVarTypeBoolean;
	v.Count = 0;
}

GUID getGuid(const EVT_VARIANT &v)
{
	if (v.Type != EvtVarTypeGuid)
	{
		THROW(InvalidDataTypeException);
	}
	return *v.GuidVal;
}

std::optional<GUID> getMaybeGuid(const EVT_VARIANT &v)
{
	if (v.Type == EvtVarTypeGuid)
		return *v.GuidVal;
	if (v.Type == EvtVarTypeNull)
		return {};
	THROW(InvalidDataTypeException);
}

size_t getSizeT(const EVT_VARIANT &v)
{
	if (v.Type != EvtVarTypeSizeT)
	{
		THROW(InvalidDataTypeException);
	}
	return v.SizeTVal;
}

void setSizeT(EVT_VARIANT &v, size_t value)
{
	if (v.Type != EvtVarTypeNull) 
	{
		THROW(InvalidArgumentException);
	}
	v.Type = EvtVarTypeSizeT;
	v.Count = 0;
	v.SizeTVal = value;
}

uint64_t getRawFileTime(const EVT_VARIANT &v)
{
	if (v.Type != EvtVarTypeFileTime)
	{
		THROW(InvalidDataTypeException);
	}

	return v.FileTimeVal;
}

FILETIME getFileTime(const EVT_VARIANT &v)
{
	if (v.Type != EvtVarTypeFileTime)
	{
		THROW(InvalidDataTypeException);
	}

	FILETIME ft;
	ft.dwLowDateTime = v.FileTimeVal & 0xffffffff;
	ft.dwHighDateTime = v.FileTimeVal >> 32;

	return ft;
}

std::optional<FILETIME> getMaybeFileTime(const EVT_VARIANT &v)
{
	if (v.Type == EvtVarTypeFileTime)
	{
		FILETIME ft{};
		ft.dwLowDateTime = v.FileTimeVal & 0xffffffff;
		ft.dwHighDateTime = v.FileTimeVal >> 32;
		return ft;
	}
	
	if (v.Type == EvtVarTypeNull)
		return {};
	
	THROW(InvalidDataTypeException);

}

std::optional<Timestamp> getMaybeTimestamp(const EVT_VARIANT &v)
{
	if (v.Type == EvtVarTypeFileTime)
		return {{v.FileTimeVal}};

	if (v.Type == EvtVarTypeNull)
		return {};

	THROW(InvalidDataTypeException);
}

void setFileTime(EVT_VARIANT &v, FILETIME value)
{
	ULONGLONG t = value.dwHighDateTime;
	t <<= 32;
	t += value.dwLowDateTime;

	v.Type = EvtVarTypeFileTime;
	v.Count = 0;
	v.FileTimeVal = t;
}

SYSTEMTIME getSystemTime(const EVT_VARIANT &v)
{
	if (v.Type != EvtVarTypeSysTime)
	{
		THROW(InvalidDataTypeException);
	}
	return *v.SysTimeVal;
}

EVT_HANDLE getEvtHandle(const EVT_VARIANT &v)
{
	if (v.Type != EvtVarTypeEvtHandle)
	{
		THROW(InvalidDataTypeException);
	}
	return v.EvtHandleVal;
}

std::vector<std::string> getStringArray(const EVT_VARIANT &v)
{
	std::vector<std::string> a;
	if (v.Type != (EvtVarTypeString | EVT_VARIANT_TYPE_ARRAY))
	{
		THROW(InvalidDataTypeException);
	}

	for (DWORD i = 0; i < v.Count; ++i)
	{
		a.push_back(to_utf8(v.StringArr[i]));
	}
	return a;
}

std::string getTypeName(const EVT_VARIANT &v)
{
	auto type = v.Type & EVT_VARIANT_TYPE_MASK;

	auto isArray = (v.Type & EVT_VARIANT_TYPE_ARRAY) == EVT_VARIANT_TYPE_ARRAY;

	std::string s;

	if (isArray)
		s += "Array of ";

	switch (type)
	{
	case EvtVarTypeNull:
		s += "Null";
		break;
	case EvtVarTypeString:
		s += "String";
		break;
	case EvtVarTypeAnsiString:
		s += "AnsiString";
		break;
	case EvtVarTypeSByte:
		s += "SByte";
		break;
	case EvtVarTypeByte:
		s += "Byte";
		break;
	case EvtVarTypeInt16:
		s += "Int16";
		break;
	case EvtVarTypeUInt16:
		s += "UInt16";
		break;
	case EvtVarTypeInt32:
		s += "Int32";
		break;
	case EvtVarTypeUInt32:
		s += "UInt32";
		break;
	case EvtVarTypeInt64:
		s += "Int64";
		break;
	case EvtVarTypeUInt64:
		s += "UInt64";
		break;
	case EvtVarTypeSingle:
		s += "Single";
		break;
	case EvtVarTypeDouble:
		s += "Double";
		break;
	case EvtVarTypeBoolean:
		s += "Boolean";
		break;
	case EvtVarTypeBinary:
		s += "Binary";
		break;
	case EvtVarTypeGuid:
		s += "Guid";
		break;
	case EvtVarTypeSizeT:
		s += "SizeT";
		break;
	case EvtVarTypeFileTime:
		s += "FileTime";
		break;
	case EvtVarTypeSysTime:
		s += "SysTime";
		break;
	case EvtVarTypeSid:
		s += "Sid";
		break;
	case EvtVarTypeHexInt32:
		s += "HexInt32";
		break;
	case EvtVarTypeHexInt64:
		s += "HexInt64";
		break;
	case EvtVarTypeEvtHandle:
		s += "EvtHandle";
		break;
	case EvtVarTypeEvtXml:
		s += "Xml";
		break;
	default:
		s += "Unknown";
		break;
	}
	return s;
}

std::string to_string(const EVT_VARIANT &v)
{
	using std::to_string;
	using Windows::to_string;

	DWORD type = v.Type & EVT_VARIANT_TYPE_MASK;
	// auto isArray = (v.Type & EVT_VARIANT_TYPE_ARRAY) == EVT_VARIANT_TYPE_ARRAY;

	switch (type)
	{
	case EvtVarTypeNull:
		return std::string("Null");		
	case EvtVarTypeString:
		return to_utf8(v.StringVal);
	case EvtVarTypeAnsiString:
		return std::string(v.AnsiStringVal);
	case EvtVarTypeSByte:
		return to_string(v.SByteVal);
	case EvtVarTypeByte:
		return to_string(v.ByteVal);
	case EvtVarTypeInt16:
		return to_string(v.Int16Val);
	case EvtVarTypeUInt16:
		return to_string(v.UInt16Val);
	case EvtVarTypeInt32:
		return to_string(v.Int32Val);
	case EvtVarTypeUInt32:
		return to_string(v.UInt32Val);
	case EvtVarTypeInt64:
		return to_string(v.Int64Val);
	case EvtVarTypeUInt64:
		return to_string(v.UInt64Val);
	case EvtVarTypeSingle:
		return to_string(v.SingleVal);
	case EvtVarTypeDouble:
		return to_string(v.DoubleVal);
	case EvtVarTypeBoolean:
		return to_string(v.BooleanVal);
	case EvtVarTypeBinary:
		return {}; // TODO
	case EvtVarTypeGuid:
		return to_string(*v.GuidVal);
	case EvtVarTypeSizeT:
		return to_string(v.SizeTVal);
	case EvtVarTypeFileTime:
		return to_string(v.FileTimeVal);
		break;
	case EvtVarTypeSysTime:
		return to_string(*v.SysTimeVal);
	case EvtVarTypeSid:
		return "Sid";
	case EvtVarTypeHexInt32:
		return "HexInt32";
	case EvtVarTypeHexInt64:
		return "HexInt64";
	case EvtVarTypeEvtHandle:
		return "EvtHandle";
	case EvtVarTypeEvtXml:
		return "Xml";
	default:
		return "Unknown";
	}
}

int64_t getHexInt64(const EVT_VARIANT &v)
{
	bool expectedType = (v.Type == EvtVarTypeHexInt64);
	if (!expectedType)
	{
		THROW(InvalidDataTypeException);
	}
	return v.Int64Val;
}

int32_t getHexInt32(const EVT_VARIANT &v)
{
	bool expectedType = (v.Type == EvtVarTypeHexInt32);
	if (!expectedType)
	{
		THROW(InvalidDataTypeException);
	}
	return v.Int32Val;
}

} // namespace
} // namespace