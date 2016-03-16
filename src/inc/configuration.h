// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
// 
// --------------------------------------------------------------------------------------------------
// configuration.h
// 

//
// ZZZZZZZZZZZZZZZZZZZZZZZZZZ write new description!! Unified method of accessing configuration values from environment variables, registry and config file(s).
// This class replaces all GetConfigDWORD and GetConfigString methods in EEConfig and REGUTIL. To define a
// flag, add an entry in the table in file:CLRConfigValues.h.
// 
// 
// 
// 
// --------------------------------------------------------------------------------------------------


#include "clrconfig.h"

#ifndef __configuration_h__
#define __configuration_h__

class Configuration
{
public:

    // Possible types for configuration values
    enum class ZNewConfigValueType
    {
        // Value isn't set
        NotSetType = 0x0,

        // Value is a DWORD
        DwordType = 0x1,

        // Value is a string
        StringType = 0x2
    };

    // Configuration options that we know about
    enum class ZNewConfigId
    {
        // Describe this option
        JitDump = 0,
        // Describe this option
        MaxThreadsForFoo = 1,
        // Describe this option
        ThreadSuspendInjection = 2,
        // Describe this option
        ServerGc = 3,

        ENUM_COUNT = 4
    };

    struct ZNewConfValue 
    {
        // Indicates type of value (and which part of the union is active)
        ZNewConfigValueType typeOfValue;

        union
        {
            DWORD dwordValue;
            LPCWSTR stringValue;
        } configValue;
    };

    // Information about a configuration knob.
    struct ZNewConfInfo
    {
        // ID of the configuration knob
        ZNewConfigId newConfigId;

        // Type of value it can have
        ZNewConfigValueType valuetype;

        // String representing the knob (e.g. "System.GC.Whatever")
        LPCWSTR newConfigName;
    };

    // Information about a configuration knob which can have a DWORD value.
    struct ZNewConfDwordInfo : ZNewConfInfo
    {
        ZNewConfDwordInfo(ZNewConfigId id, LPCWSTR name, const CLRConfig::ConfigDWORDInfo *dwordInfo)
        {
            valuetype = ZNewConfigValueType::DwordType;

            newConfigId = id;
            newConfigName = name;
            legacyDwordInfo = dwordInfo;
        }

        // The ConfigDWORDInfo which enables us to get the value for this
        // configuration knob in the legacy (COMPlus) way.
        const CLRConfig::ConfigDWORDInfo* legacyDwordInfo;
    };

    // Information about a configuration knob which can have a string value.
    struct ZNewConfStringInfo : ZNewConfInfo
    {
        ZNewConfStringInfo(ZNewConfigId id, LPCWSTR name, const CLRConfig::ConfigStringInfo *stringInfo)
        {
            valuetype = ZNewConfigValueType::StringType;

            newConfigId = id;
            newConfigName = name;
            legacyStringInfo = stringInfo;
        }

        // The ConfigStringInfo which enables us to get the value for this
        // configuration knob in the legacy (COMPlus) way.
        const CLRConfig::ConfigStringInfo* legacyStringInfo;
    };

public:
    static void ZInitializeNewConfigurationValues(int numberOfConfigs, LPCWSTR *configNames, LPCWSTR *configValues);

    static DWORD ZGetConfigDWORDValue2(const ZNewConfigId);
    static LPCWSTR ZGetConfigStringValue2(const ZNewConfigId);

private:
    static const ZNewConfInfo* ZGetConfigInfoFromId(const ZNewConfigId desiredId);
    static const ZNewConfInfo* ZGetConfigInfoFromName(LPCWSTR desiredName);

    static void Configuration::ZGetConfigValue2(const ZNewConfigId configId, ZNewConfValue *value);

    // static CrstStatic m_ZConfigValuesCrst;
};

#endif // __configuration_h__
