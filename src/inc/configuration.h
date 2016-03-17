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

    // Configuration options that we know about
    enum class ConfigurationKnobId
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

    // Possible types for configuration values
    enum class ConfigurationValueType
    {
        // Value isn't set
        NotSetType = 0x0,

        // Value is a DWORD
        DwordType = 0x1,

        // Value is a string
        StringType = 0x2
    };

    struct ConfigurationValue
    {
        // Indicates type of value (and which part of the union is active)
        ConfigurationValueType typeOfValue;

        union
        {
            DWORD dword;
            LPCWSTR str;
        } value;
    };

    // Information about a configuration knob.
    struct ConfigurationKnob
    {
        ConfigurationKnob(ConfigurationKnobId id, ConfigurationValueType type, LPCWSTR name)
            : knobId(id)
            , valueType(type)
            , knobName(name)
        { }

        // ID of the configuration knob
        ConfigurationKnobId knobId;

        // Type of value it can have
        ConfigurationValueType valueType;

        // String representing the knob (e.g. "System.GC.EnableFooGc")
        LPCWSTR knobName;
    };

    // Information about a configuration knob which can have a DWORD value.
    struct DwordConfigurationKnob : ConfigurationKnob
    {
        DwordConfigurationKnob(ConfigurationKnobId id, LPCWSTR name, const CLRConfig::ConfigDWORDInfo *dwordInfo)
            : ConfigurationKnob(id, ConfigurationValueType::DwordType, name)
            , legacyDwordInfo(dwordInfo)
        { }

        // The ConfigDWORDInfo which enables us to get the value for this
        // configuration knob in the legacy (COMPlus) way.
        const CLRConfig::ConfigDWORDInfo* legacyDwordInfo;
    };

    // Information about a configuration knob which can have a string value.
    struct StringConfigurationKnob : ConfigurationKnob
    {
        StringConfigurationKnob(ConfigurationKnobId id, LPCWSTR name, const CLRConfig::ConfigStringInfo *stringInfo)
            : ConfigurationKnob(id, ConfigurationValueType::StringType, name)
            , legacyStringInfo(stringInfo)
        { }

        // The ConfigStringInfo which enables us to get the value for this
        // configuration knob in the legacy (COMPlus) way.
        const CLRConfig::ConfigStringInfo* legacyStringInfo;
    };

public:
    static void InitializeConfigurationKnobs(int numberOfConfigs, LPCWSTR *configNames, LPCWSTR *configValues);

    static DWORD GetKnobDWORDValue(const ConfigurationKnobId id);
    static LPCWSTR GetKnobStringValue(const ConfigurationKnobId id);

private:
    static const ConfigurationKnob* GetConfigurationKnobById(const ConfigurationKnobId id);
    static const ConfigurationKnob* GetConfigurationKnobByName(LPCWSTR name);

    static void Configuration::GetConfigurationValue(const ConfigurationKnobId id, ConfigurationValue *value);

    // static CrstStatic m_ZConfigValuesCrst;
};

#endif // __configuration_h__
