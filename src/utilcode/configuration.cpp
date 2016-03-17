// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
//*****************************************************************************
// CLRConfig.cpp
// 

//
// Unified method of accessing configuration values from environment variables,
// registry and config file. See file:../inc/CLRConfigValues.h for details on how to add config values.
// 
//*****************************************************************************

#include "clrconfig.h"
#include "configuration.h"

static CRITSEC_COOKIE configurationCritSec = nullptr;

const Configuration::DwordConfigurationKnob DwordConfigurationKnobs[] = 
{
    {Configuration::ConfigurationKnobId::ThreadSuspendInjection, W("System.Foo.InjectSuspension"), &CLRConfig::INTERNAL_ThreadSuspendInjection},
    {Configuration::ConfigurationKnobId::ServerGc, W("System.GC.EnableServerGC"), &CLRConfig::UNSUPPORTED_gcServer},
};

const Configuration::StringConfigurationKnob StringConfigurationKnobs[] = 
{
    {Configuration::ConfigurationKnobId::JitDump, W("System.JIT.JitDump"), &CLRConfig::INTERNAL_JitDump},
    {Configuration::ConfigurationKnobId::MaxThreadsForFoo, W("System.Threading.MaxFoo"), &CLRConfig::INTERNAL_Security_TransparencyMethodBreak},
};

static const int NumberOfDwordKnobs = sizeof(DwordConfigurationKnobs) / sizeof(DwordConfigurationKnobs[0]);
static const int NumberOfStringKnobs = sizeof(StringConfigurationKnobs) / sizeof(StringConfigurationKnobs[0]);

static const int TotalNumberOfKnobs = (int)Configuration::ConfigurationKnobId::ENUM_COUNT;

static_assert(NumberOfDwordKnobs + NumberOfStringKnobs == TotalNumberOfKnobs, "Number of knob definitions must match number of possible knobs.");

Configuration::ConfigurationValue configValues[TotalNumberOfKnobs];

const Configuration::ConfigurationKnob* Configuration::GetConfigurationKnobById(const Configuration::ConfigurationKnobId id)
{
    // TODO: Might be worth improving lookup time once there are more elements. For now, it's just linear.
    for (int i = 0; i < NumberOfDwordKnobs; i++)
    {
        if (DwordConfigurationKnobs[i].knobId == id)
        {
            return &DwordConfigurationKnobs[i];
        }
    }

    for (int i = 0; i < NumberOfStringKnobs; i++)
    {
        if (StringConfigurationKnobs[i].knobId == id)
        {
            return &StringConfigurationKnobs[i];
        }
    }

    return nullptr;
}

const Configuration::ConfigurationKnob* Configuration::GetConfigurationKnobByName(LPCWSTR name)
{
    // TODO: Might be worth improving lookup time once there are more elements. For now, it's just linear.
    for (int i = 0; i < NumberOfDwordKnobs; i++)
    {
        if (wcscmp(name, DwordConfigurationKnobs[i].knobName) == 0)
        {
            return &DwordConfigurationKnobs[i];
        }
    }

    for (int i = 0; i < NumberOfStringKnobs; i++)
    {
        if (wcscmp(name, StringConfigurationKnobs[i].knobName) == 0)
        {
            return &StringConfigurationKnobs[i];
        }
    }

    return nullptr;
}

void Configuration::InitializeConfigurationKnobs(int numberOfConfigs, LPCWSTR *names, LPCWSTR *values)
{
    CRITSEC_COOKIE lock = ClrCreateCriticalSection(CrstLeafLock, CRST_UNSAFE_ANYMODE);
    if (InterlockedCompareExchangeT<CRITSEC_COOKIE>(&configurationCritSec, lock, nullptr) != nullptr)
    {
        ClrDeleteCriticalSection(lock);
    }

    CRITSEC_Holder csecHolder(configurationCritSec);

    // Initialize everything to NotSetType
    for (int i = 0; i < TotalNumberOfKnobs; ++i)
    {
        configValues[i].typeOfValue = ConfigurationValueType::NotSetType;
    }

    // For any configuration values that are passed in, set their values correctly. This may mean
    // either using the value passed in or using the legacy (COMPlus) equivalent if it is set.
    for (int i = 0; i < numberOfConfigs; ++i)
    {
        const ConfigurationKnob *knob = GetConfigurationKnobByName(names[i]);

        // This isn't a configuration option that we recognize. Ignore it.
        if (knob == nullptr)
            continue;

        ConfigurationKnobId id = knob->knobId;

        // Set the type of the config value. If we got to this point, we're going to set the value.
        configValues[static_cast<int>(id)].typeOfValue = knob->valueType;

        _ASSERTE(knob->valueType == ConfigurationValueType::DwordType || knob->valueType == ConfigurationValueType::StringType);

        if (knob->valueType == ConfigurationValueType::DwordType)
        {
            const CLRConfig::ConfigDWORDInfo* const legacyInfo = static_cast<const DwordConfigurationKnob* const>(knob)->legacyDwordInfo;

            DWORD value;
            bool legacyConfigSet = CLRConfig::GetConfigValue(*legacyInfo, false /* useDefaultValue */, &value);

            if (legacyConfigSet)
            {
                configValues[static_cast<int>(id)].value.dword = value;
            }
            else
            {
                configValues[static_cast<int>(id)].value.dword = wcstoul(values[i], nullptr, 0);
            }

        }
        else if (knob->valueType == ConfigurationValueType::StringType)
        {
            const CLRConfig::ConfigStringInfo* const legacyInfo = static_cast<const StringConfigurationKnob* const>(knob)->legacyStringInfo;

            LPWSTR value = CLRConfig::GetConfigValue(*legacyInfo);

            if (value != nullptr)
            {
                // If the value is set in the legacy way, use that.
                // We don't make a copy of value because we own it.
                configValues[static_cast<int>(id)].value.str = value;
            }
            else
            {
                // Make a copy of the string passed in.
                int bufferLength = wcslen(values[i]) + 1;
                LPWSTR buffer = new WCHAR[bufferLength];
                wcscpy_s(buffer, bufferLength, values[i]);

                configValues[static_cast<int>(id)].value.str = buffer;
            }
        }
    }
}

void Configuration::GetConfigurationValue(const ConfigurationKnobId id, ConfigurationValue *value)
{
    _ASSERT(value != nullptr);

    // Look up the ID in the table.
    // If the value is set, use that. Otherwise, set it in the table using the old way.
    if (configValues[static_cast<int>(id)].typeOfValue == Configuration::ConfigurationValueType::NotSetType)
    {
        CRITSEC_Holder csecHolder(configurationCritSec);

        // Check if someone has already updated the value.
        if (configValues[static_cast<int>(id)].typeOfValue == Configuration::ConfigurationValueType::NotSetType)
        {
            const ConfigurationKnob * const knob = GetConfigurationKnobById(id);
            if (knob != nullptr)
            {
                configValues[static_cast<int>(id)].typeOfValue = knob->valueType;
                if (knob->valueType == ConfigurationValueType::DwordType)
                {
                    const CLRConfig::ConfigDWORDInfo* const legacyInfo = static_cast<const DwordConfigurationKnob* const>(knob)->legacyDwordInfo;
                    DWORD valueToSet = CLRConfig::GetConfigValue(*legacyInfo);
                    configValues[static_cast<int>(id)].value.dword = valueToSet;
                }
                else if (knob->valueType == ConfigurationValueType::StringType)
                {
                    const CLRConfig::ConfigStringInfo* const legacyInfo = static_cast<const StringConfigurationKnob* const>(knob)->legacyStringInfo;
                    LPWSTR valueToSet = CLRConfig::GetConfigValue(*legacyInfo);
                    configValues[static_cast<int>(id)].value.str = valueToSet;
                }
                else
                {
                    // Should never get here. All configurations should be either DWORD or string type.
                    _ASSERT(false);
                }
            }
        }
    }

    *value = configValues[static_cast<int>(id)];
}

DWORD Configuration::GetKnobDWORDValue(const ConfigurationKnobId id)
{
    ConfigurationValue confValue;
    GetConfigurationValue(id, &confValue);

    _ASSERT(confValue.typeOfValue == ConfigurationValueType::DwordType);
    return confValue.value.dword;
}

LPCWSTR Configuration::GetKnobStringValue(const ConfigurationKnobId id)
{
    ConfigurationValue confValue;
    GetConfigurationValue(id, &confValue);

    _ASSERT(confValue.typeOfValue == ConfigurationValueType::StringType);
    return confValue.value.str;
}
