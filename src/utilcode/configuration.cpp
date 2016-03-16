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

///////// naming
///////// casing
///////// locks?

const Configuration::ZNewConfDwordInfo m_DwordConfigInfos[] = 
{
    {Configuration::ZNewConfigId::ThreadSuspendInjection, W("System.Foo.InjectSuspension"), &CLRConfig::INTERNAL_ThreadSuspendInjection},
    {Configuration::ZNewConfigId::ServerGc, W("System.GC.EnableServerGC"), &CLRConfig::UNSUPPORTED_gcServer},
};

const Configuration::ZNewConfStringInfo m_StringConfigInfos[] = 
{
    {Configuration::ZNewConfigId::JitDump, W("System.JIT.JitDump"), &CLRConfig::INTERNAL_JitDump},
    {Configuration::ZNewConfigId::MaxThreadsForFoo, W("System.Threading.MaxFoo"), &CLRConfig::INTERNAL_Security_TransparencyMethodBreak},
};

static const int DWORD_INFOS_COUNT = sizeof(m_DwordConfigInfos) / sizeof(m_DwordConfigInfos[0]);
static const int STRING_INFOS_COUNT = sizeof(m_StringConfigInfos) / sizeof(m_StringConfigInfos[0]);
static const int ZCONFIG_COUNT = (int)Configuration::ZNewConfigId::ENUM_COUNT;

static_assert(DWORD_INFOS_COUNT + STRING_INFOS_COUNT == ZCONFIG_COUNT, "There should be one ZZZConfigInfo per configuration option and no more.");

// CrstStatic Configuration::m_ZConfigValuesCrst;

Configuration::ZNewConfValue configValues[ZCONFIG_COUNT];

const Configuration::ZNewConfInfo* Configuration::ZGetConfigInfoFromId(const Configuration::ZNewConfigId desiredId)
{
    //// improve this lookup? for now linear
    for (int i = 0; i < DWORD_INFOS_COUNT; i++)
    {
        if (m_DwordConfigInfos[i].newConfigId == desiredId)
        {
            return &m_DwordConfigInfos[i];
        }
    }

    for (int i = 0; i < STRING_INFOS_COUNT; i++)
    {
        if (m_StringConfigInfos[i].newConfigId == desiredId)
        {
            return &m_StringConfigInfos[i];
        }
    }

    return nullptr;
}

const Configuration::ZNewConfInfo* Configuration::ZGetConfigInfoFromName(LPCWSTR desiredName)
{
    //// improve this lookup? for now linear
    for (int i = 0; i < DWORD_INFOS_COUNT; i++)
    {
        if (wcscmp(desiredName, m_DwordConfigInfos[i].newConfigName) == 0)
        {
            return &m_DwordConfigInfos[i];
        }
    }

    for (int i = 0; i < STRING_INFOS_COUNT; i++)
    {
        if (wcscmp(desiredName, m_StringConfigInfos[i].newConfigName) == 0)
        {
            return &m_StringConfigInfos[i];
        }
    }

    return nullptr;
}

void Configuration::ZInitializeNewConfigurationValues(int numberOfConfigs, LPCWSTR *names, LPCWSTR *values)
{
    // m_ZConfigValuesCrst.Init(CrstLeafLock, CrstFlags(CRST_UNSAFE_ANYMODE));

    // Initialize everything to NotSetType
    for (int i = 0; i < ZCONFIG_COUNT; ++i)
    {
        configValues[i].typeOfValue = ZNewConfigValueType::NotSetType;
    }

    // CrstHolder ch(&m_ZConfigValuesCrst);

    // For any configuration values that are passed in, set their values correctly. This may
    // mean either using the value passed in or using the legacy (COMPlus) equivalent if it
    // is set.
    for (int i = 0; i < numberOfConfigs; ++i)
    {
        const ZNewConfInfo *configInfo = ZGetConfigInfoFromName(names[i]);

        // This isn't a configuration option that we recognize. Ignore it.
        if (configInfo == nullptr)
            continue;

        ZNewConfigId id = configInfo->newConfigId;

        // Set the type of the config value. If we got to this point, we're going to set the value.
        configValues[static_cast<int>(id)].typeOfValue = configInfo->valuetype;

        _ASSERTE(configInfo->valuetype == ZNewConfigValueType::DwordType || configInfo->valuetype == ZNewConfigValueType::StringType);

        if (configInfo->valuetype == ZNewConfigValueType::DwordType)
        {
            const CLRConfig::ConfigDWORDInfo* const legacyInfo = static_cast<const ZNewConfDwordInfo* const>(configInfo)->legacyDwordInfo;

            DWORD value;
            bool legacyConfigSet = CLRConfig::GetConfigValue(*legacyInfo, false /* useDefaultValue */, &value);

            if (legacyConfigSet)
            {
                configValues[static_cast<int>(id)].configValue.dwordValue = value;
            }
            else
            {
                configValues[static_cast<int>(id)].configValue.dwordValue = wcstoul(values[i], nullptr, 0);
            }

        }
        else if (configInfo->valuetype == ZNewConfigValueType::StringType)
        {
            const CLRConfig::ConfigStringInfo* const legacyInfo = static_cast<const ZNewConfStringInfo* const>(configInfo)->legacyStringInfo;

            LPWSTR value = CLRConfig::GetConfigValue(*legacyInfo);

            if (value != nullptr)
            {
                // If the value is set in the legacy way, use that.
                configValues[static_cast<int>(id)].configValue.stringValue = value; ///// no copy here. we own the string that's returned
            }
            else
            {
                configValues[static_cast<int>(id)].configValue.stringValue = values[i]; //// SHOULD WE MAKE A COPY OF THIS?
            }
        }
    }
}

void Configuration::ZGetConfigValue2(const ZNewConfigId configId, ZNewConfValue *value)
{
    _ASSERT(value != nullptr);

    // Look up the ID in the table.
    // If the value is set, use that. Otherwise, set it in the table using the old way.
    if (configValues[static_cast<int>(configId)].typeOfValue == Configuration::ZNewConfigValueType::NotSetType)
    {
        const ZNewConfInfo * const configInfo = ZGetConfigInfoFromId(configId);
        if (configInfo != nullptr)
        {
            configValues[static_cast<int>(configId)].typeOfValue = configInfo->valuetype;
            if (configInfo->valuetype == ZNewConfigValueType::DwordType)
            {
                DWORD valueToSet = CLRConfig::GetConfigValue(*(static_cast<const ZNewConfDwordInfo* const>(configInfo)->legacyDwordInfo));
                configValues[static_cast<int>(configId)].configValue.dwordValue = valueToSet;
            }
            else if (configInfo->valuetype == ZNewConfigValueType::StringType)
            {
                LPWSTR valueToSet = CLRConfig::GetConfigValue(*(static_cast<const ZNewConfStringInfo* const>(configInfo)->legacyStringInfo));
                configValues[static_cast<int>(configId)].configValue.stringValue = valueToSet; //// SHOULD WE MAKE A COPY OF THIS?
            }
            else
            {
                // Should never get here. All configs should be either DWORD or string type.
                _ASSERT(false);
            }
        }
    }

    *value = configValues[static_cast<int>(configId)];
}

DWORD Configuration::ZGetConfigDWORDValue2(const ZNewConfigId configId)
{
    ZNewConfValue confValue;
    ZGetConfigValue2(configId, &confValue);

    _ASSERT(confValue.typeOfValue == ZNewConfigValueType::DwordType);
    return confValue.configValue.dwordValue;
}


LPCWSTR Configuration::ZGetConfigStringValue2(const ZNewConfigId configId)
{
    ZNewConfValue confValue;
    ZGetConfigValue2(configId, &confValue);

    _ASSERT(confValue.typeOfValue == ZNewConfigValueType::StringType);
    return confValue.configValue.stringValue;
}
