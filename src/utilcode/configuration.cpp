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

const Configuration::DwordConfigurationKnob DwordConfigurationKnobs[] = 
{
    {Configuration::ConfigurationKnobId::System_GC_Concurrent, W("System.GC.Concurrent"), &CLRConfig::UNSUPPORTED_gcConcurrent, true},
    {Configuration::ConfigurationKnobId::System_GC_Server, W("System.GC.Server"), &CLRConfig::UNSUPPORTED_gcServer, true},
    {Configuration::ConfigurationKnobId::System_Threading_ThreadPool_MinThreads, W("System.Threading.ThreadPool.MinThreads"), &CLRConfig::INTERNAL_ThreadPool_ForceMinWorkerThreads, false},
    {Configuration::ConfigurationKnobId::System_Threading_ThreadPool_MaxThreads, W("System.Threading.ThreadPool.MaxThreads"), &CLRConfig::INTERNAL_ThreadPool_ForceMaxWorkerThreads, false},
};

const Configuration::StringConfigurationKnob StringConfigurationKnobs[] = 
{
    // String configuration options go here
};

static CRITSEC_COOKIE configurationCritSec = nullptr;

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

        if (knob->valueType == ConfigurationValueType::DwordType || knob->valueType == ConfigurationValueType::BooleanType)
        {
            const CLRConfig::ConfigDWORDInfo* const legacyInfo = static_cast<const DwordConfigurationKnob* const>(knob)->legacyDwordInfo;

            DWORD value;
            bool legacyConfigSet = CLRConfig::GetConfigValue(*legacyInfo, false /* useDefaultIfNotSet */, true /* acceptExplicitDefaultFromRegutil */, &value);
            if (legacyConfigSet)
            {
                if (knob->valueType == ConfigurationValueType::DwordType)
                {
                    configValues[static_cast<int>(id)].value.dword = value;
                }
                else
                {
                    configValues[static_cast<int>(id)].value.boolean = (value == 1);
                }
            }
            else
            {
                if (knob->valueType == ConfigurationValueType::DwordType)
                {
                    configValues[static_cast<int>(id)].value.dword = wcstoul(values[i], nullptr, 0);
                }
                else
                {
                    configValues[static_cast<int>(id)].value.boolean = (wcscmp(values[i], W("true")) == 0);
                }
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
        else
        {
            // There should be no other types of values
            _ASSERT(false);
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
                else if (knob->valueType == ConfigurationValueType::BooleanType)
                {
                    const CLRConfig::ConfigDWORDInfo* const legacyInfo = static_cast<const DwordConfigurationKnob* const>(knob)->legacyDwordInfo;
                    DWORD valueAsDword = CLRConfig::GetConfigValue(*legacyInfo);
                    configValues[static_cast<int>(id)].value.boolean = (valueAsDword == 1);
                }
                else
                {
                    // Should never get here. All configurations should be DWORD, string, or boolean type.
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

bool Configuration::GetKnobBooleanValue(const ConfigurationKnobId id)
{
    ConfigurationValue confValue;
    GetConfigurationValue(id, &confValue);

    _ASSERT(confValue.typeOfValue == ConfigurationValueType::BooleanType);
    return confValue.value.boolean;
}
