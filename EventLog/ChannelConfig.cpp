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

#include "ChannelConfig.h"

#include "EvtHandle.h"
#include "EvtVariant.h"

namespace Windows::EventLog
{

class ChannelConfigImpl
{
public:
	explicit ChannelConfigImpl(const std::string &path)
		: mConfigHandle{ChannelConfigHandle::open(path)}
	{}

	ChannelConfigHandle mConfigHandle;
};

struct VariantGuid
{
	EVT_VARIANT value;
	GUID g;
};

struct VariantSystemTime
{
	EVT_VARIANT value;
	SYSTEMTIME st;
};


static std::string getString(const ChannelConfigHandle &h, EVT_CHANNEL_CONFIG_PROPERTY_ID Id)
{
	EvtVariantPtr pv = h.getPropertyValue(Id);
	return Variant::getString(*pv);
}


static void setString(const ChannelConfigHandle &h, EVT_CHANNEL_CONFIG_PROPERTY_ID Id, const std::string &s)
{
	auto p = Variant::allocStringVariant(s);
	h.setPropertyValue(Id, p.get());
}

static UINT32 getUInt32(const ChannelConfigHandle &h, EVT_CHANNEL_CONFIG_PROPERTY_ID Id)
{
	EVT_VARIANT v{};
	h.getPropertyValue(Id, &v);
	return Variant::getUInt32(v);
}

static
std::optional<UINT32> getNullableUInt32(const ChannelConfigHandle &h,
	EVT_CHANNEL_CONFIG_PROPERTY_ID Id)
{
	EVT_VARIANT v{};
	h.getPropertyValue(Id, &v);
	if (v.Type == EvtVarTypeNull)
		return std::optional<INT32>{};
	return std::make_optional<INT32>(Variant::getUInt32(v));
}

static void setUInt32(const ChannelConfigHandle &h, EVT_CHANNEL_CONFIG_PROPERTY_ID Id, UINT32 value)
{
	EVT_VARIANT v{};
	Variant::setUInt32(v, value);
	h.setPropertyValue(Id, &v);
}

static void setNullableUInt32(const ChannelConfigHandle &h, EVT_CHANNEL_CONFIG_PROPERTY_ID Id, std::optional<UINT32> value)
{
	if (value.has_value())
	{
		setUInt32(h, Id, value.value());
	}
	else
	{
		EVT_VARIANT v{};
		v.Type = EvtVarTypeNull;
		h.setPropertyValue(Id, &v);
	}
}

void setInt64(const ChannelConfigHandle &h, EVT_CHANNEL_CONFIG_PROPERTY_ID Id, INT64 value)
{
	EVT_VARIANT v{};
	Variant::setInt64(v, value);
	h.setPropertyValue(Id, &v);
}

static UINT64 getUInt64(const ChannelConfigHandle &h, EVT_CHANNEL_CONFIG_PROPERTY_ID Id)
{
	EVT_VARIANT v{};
	h.getPropertyValue(Id, &v);
	return Variant::getUInt64(v);
}

static std::optional<UINT64> getNullableUInt64(const ChannelConfigHandle &h, EVT_CHANNEL_CONFIG_PROPERTY_ID Id)
{
	EVT_VARIANT v{};
	h.getPropertyValue(Id, &v);
	if (v.Type == EvtVarTypeNull)
		return std::optional<UINT64>{};
	return std::make_optional<UINT64>(Variant::getUInt64(v));
}

static void setUInt64(const ChannelConfigHandle &h, EVT_CHANNEL_CONFIG_PROPERTY_ID Id, UINT64 value)
{
	EVT_VARIANT v{};
	Variant::setUInt64(v, value);
	h.setPropertyValue(Id, &v);
}

static void setNullableUInt64(const ChannelConfigHandle &h, EVT_CHANNEL_CONFIG_PROPERTY_ID Id, std::optional<UINT64> value)
{
	if (value.has_value())
	{
		setUInt64(h, Id, value.value());
	}
	else
	{
		EVT_VARIANT v{};
		v.Type = EvtVarTypeNull;
		h.setPropertyValue(Id, &v);
	}
}

static bool getBoolean(const ChannelConfigHandle &h, EVT_CHANNEL_CONFIG_PROPERTY_ID Id)
{
	EVT_VARIANT v{};
	h.getPropertyValue(Id, &v);
	return Variant::getBool(v);
}

static void setBoolean(const ChannelConfigHandle &h, EVT_CHANNEL_CONFIG_PROPERTY_ID Id, bool value)
{
	EVT_VARIANT v{};
	Variant::setBool(v, value);
	h.setPropertyValue(Id, &v);
}

static std::optional<GUID> getNullableGuid(const ChannelConfigHandle &h, EVT_CHANNEL_CONFIG_PROPERTY_ID Id)
{
	EVT_VARIANT v{};
	h.getPropertyValue(Id, &v);
	if (v.Type == EvtVarTypeNull)
		return std::optional<GUID>{};
	return std::make_optional<GUID>(Variant::getGuid(v));
}

static std::vector<std::string> getStringArray(const ChannelConfigHandle &h, EVT_CHANNEL_CONFIG_PROPERTY_ID Id)
{
	EvtVariantPtr pv = h.getPropertyValue(Id);
	return Variant::getStringArray(*pv);
}

Ref<IChannelConfig> ChannelConfig::create(const std::string &path)
{
	return RefObject<ChannelConfig>::createRef(path);
}

ChannelConfig::~ChannelConfig()
{}

ChannelConfig::ChannelConfig(const std::string &path)
	: d_ptr(std::make_unique<ChannelConfigImpl>(path))
{}

bool ChannelConfig::getConfigEnabled() const
{
	return getBoolean(d_ptr->mConfigHandle, EvtChannelConfigEnabled);
}

void ChannelConfig::setConfigEnabled(bool isEnabled)
{
	setBoolean(d_ptr->mConfigHandle, EvtChannelConfigEnabled, isEnabled);
}

ChannelIsolation ChannelConfig::getConfigIsolation() const
{
	auto value = getUInt32(d_ptr->mConfigHandle, EvtChannelConfigIsolation);
	return static_cast<ChannelIsolation>(value);
}

ChannelType ChannelConfig::getConfigType() const
{
	auto value = getUInt32(d_ptr->mConfigHandle, EvtChannelConfigType);
	return static_cast<ChannelType>(value);
}

std::string ChannelConfig::getConfigOwningPublisher() const
{
	return getString(d_ptr->mConfigHandle, EvtChannelConfigOwningPublisher);
}

bool ChannelConfig::getConfigClassicEventLog() const
{
	return getBoolean(d_ptr->mConfigHandle, EvtChannelConfigClassicEventlog);
}

std::string ChannelConfig::getConfigAccess() const
{
	return getString(d_ptr->mConfigHandle, EvtChannelConfigAccess);
}

void ChannelConfig::setConfigAccess(const std::string &access)
{
	setString(d_ptr->mConfigHandle, EvtChannelConfigAccess, access);
}

bool ChannelConfig::getLoggingConfigRetention() const
{
	return getBoolean(d_ptr->mConfigHandle, EvtChannelLoggingConfigRetention);
}

void ChannelConfig::setLoggingConfigRetention(bool retention)
{
	setBoolean(d_ptr->mConfigHandle, EvtChannelLoggingConfigRetention, retention);
}

bool ChannelConfig::getLoggingConfigAutoBackup() const
{
	return getBoolean(d_ptr->mConfigHandle, EvtChannelLoggingConfigAutoBackup);
}

void ChannelConfig::setLoggingConfigAutoBackup(bool autobackup)
{
	setBoolean(d_ptr->mConfigHandle, EvtChannelLoggingConfigAutoBackup, autobackup);
}

UINT64 ChannelConfig::getLoggingConfigMaxSize() const
{
	return getUInt64(d_ptr->mConfigHandle, EvtChannelLoggingConfigMaxSize);
}

void ChannelConfig::setLoggingConfigMaxSize(uint64_t value)
{
	setUInt64(d_ptr->mConfigHandle, EvtChannelLoggingConfigMaxSize, value);
}

std::string ChannelConfig::getLoggingConfigLogFilePath() const
{
	return getString(d_ptr->mConfigHandle, EvtChannelLoggingConfigLogFilePath);
}

void ChannelConfig::setLoggingConfigLogFilePath(const std::string &path)
{
	return setString(d_ptr->mConfigHandle, EvtChannelLoggingConfigLogFilePath, path);
}

std::optional<UINT32> ChannelConfig::getPublishingConfigLevel() const
{
	return getNullableUInt32(d_ptr->mConfigHandle, EvtChannelPublishingConfigLevel);
}

// Disable the debug or analytic channel first
void ChannelConfig::setPublishingConfigLevel(std::optional<UINT32> level)
{
	setNullableUInt32(d_ptr->mConfigHandle, EvtChannelPublishingConfigLevel, level);
}

std::optional<uint64_t> ChannelConfig::getPublishingConfigKeywords() const
{
	auto val = getNullableUInt64(d_ptr->mConfigHandle, EvtChannelPublishingConfigKeywords);
	if (val.has_value())
	{
		// Top word is reserved. Mask it off.
		val.value() = val.value() & 0x0000FFFFFFFFFFFFull;
	}
	return val;
}

// Disable the debug or analytic channel first
void ChannelConfig::setPublishingConfigKeywords(std::optional<uint64_t> value)
{
	setNullableUInt64(d_ptr->mConfigHandle, EvtChannelPublishingConfigKeywords, value);
}

std::optional<GUID> ChannelConfig::getPublishingConfigControlGuid() const
{
	return getNullableGuid(d_ptr->mConfigHandle, EvtChannelPublishingConfigControlGuid);
}

std::optional<UINT32> ChannelConfig::getPublishingConfigBufferSize() const
{
	return getNullableUInt32(d_ptr->mConfigHandle, EvtChannelPublishingConfigBufferSize);
}

std::optional<UINT32> ChannelConfig::getPublishingConfigMinBuffers() const
{
	return getNullableUInt32(d_ptr->mConfigHandle, EvtChannelPublishingConfigMinBuffers);
}

std::optional<UINT32> ChannelConfig::getPublishingConfigMaxBuffers() const
{
	return getNullableUInt32(d_ptr->mConfigHandle, EvtChannelPublishingConfigMaxBuffers);
}

std::optional<UINT32> ChannelConfig::getPublishingConfigLatency() const
{
	return getNullableUInt32(d_ptr->mConfigHandle, EvtChannelPublishingConfigLatency);
}

std::optional<ChannelClockType> ChannelConfig::getPublishingConfigClockType() const
{
	auto value = getNullableUInt32(d_ptr->mConfigHandle, EvtChannelPublishingConfigClockType);
	if (value.has_value())
		return std::optional<ChannelClockType>(static_cast<ChannelClockType>(value.value()));
	return {};
}

std::optional<ChannelSIDType> ChannelConfig::getPublishingConfigSidType() const
{
	auto value = getNullableUInt32(d_ptr->mConfigHandle, EvtChannelPublishingConfigSidType);
	if (value.has_value())
		return std::optional<ChannelSIDType>(static_cast<ChannelSIDType>(value.value()));
	return {};
}

std::vector<std::string> ChannelConfig::getPublisherList() const
{
	return getStringArray(d_ptr->mConfigHandle, EvtChannelPublisherList);
}

std::optional<uint32_t> ChannelConfig::getPublishingConfigFileMax() const
{
	return getNullableUInt32(d_ptr->mConfigHandle, EvtChannelPublishingConfigFileMax);
}

void ChannelConfig::setPublishingConfigFileMax(std::optional<uint32_t> value)
{
	return setNullableUInt32(d_ptr->mConfigHandle, EvtChannelPublishingConfigFileMax, value);
}

void ChannelConfig::save()
{
	d_ptr->mConfigHandle.save();
}

Ref<IChannelConfig> IChannelConfig::create(const std::string &channel)
{
	return ChannelConfig::create(channel);
}

}