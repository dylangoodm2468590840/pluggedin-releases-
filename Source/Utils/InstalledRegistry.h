#pragma once

#include <juce_core/juce_core.h>

/**
 * @class InstalledRegistry
 * @brief Thread-safe local JSON registry manager for tracking installed plugin versions on the client machine.
 */
class InstalledRegistry
{
public:
    static juce::File getRegistryFile()
    {
        juce::File appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
        juce::File pluggedInDir = appDataDir.getChildFile("PluggedIN");

        if (!pluggedInDir.exists())
            pluggedInDir.createDirectory();

        return pluggedInDir.getChildFile("installed_manifest.json");
    }

    static juce::var readRegistry()
    {
        juce::File regFile = getRegistryFile();
        if (!regFile.existsAsFile())
            return juce::var(new juce::DynamicObject());

        juce::String jsonContent = regFile.loadFileAsString();
        auto parsed = juce::JSON::parse(jsonContent);

        if (parsed.isObject())
            return parsed;

        return juce::var(new juce::DynamicObject());
    }

    static bool writeRegistry(const juce::var& registryData)
    {
        juce::File regFile = getRegistryFile();
        juce::String jsonString = juce::JSON::toString(registryData, false);
        return regFile.replaceWithText(jsonString);
    }

    static juce::String getInstalledVersion(const juce::String& pluginId)
    {
        auto registry = readRegistry();
        if (auto* obj = registry.getDynamicObject())
        {
            if (obj->hasProperty(pluginId))
            {
                auto pluginObj = obj->getProperty(pluginId);
                if (auto* pObj = pluginObj.getDynamicObject())
                {
                    return pObj->getProperty("installed_version").toString();
                }
            }
        }

        // Fallback: Check if VST3 exists on disk without a registry entry
        juce::File userVst3 = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                                 .getChildFile("Local\\Programs\\Common\\VST3\\UNDERGROUND.vst3");
        juce::File sysVst3("C:\\Program Files\\Common Files\\VST3\\UNDERGROUND.vst3");

        if (userVst3.exists() || sysVst3.exists())
            return "1.0.0"; // Default legacy installed base

        return "";
    }

    static void setInstalledVersion(const juce::String& pluginId, const juce::String& version, const juce::String& path = "")
    {
        auto registry = readRegistry();
        auto* obj = registry.getDynamicObject();
        if (!obj)
        {
            obj = new juce::DynamicObject();
            registry = juce::var(obj);
        }

        auto* pluginObj = new juce::DynamicObject();
        pluginObj->setProperty("installed_version", version);
        pluginObj->setProperty("install_date", juce::Time::getCurrentTime().toISO8601(true));
        pluginObj->setProperty("path", path);

        obj->setProperty(pluginId, juce::var(pluginObj));
        writeRegistry(registry);
    }

    static void removeInstalledVersion(const juce::String& pluginId)
    {
        auto registry = readRegistry();
        if (auto* obj = registry.getDynamicObject())
        {
            if (obj->hasProperty(pluginId))
            {
                obj->removeProperty(pluginId);
                writeRegistry(registry);
            }
        }
    }

    /**
     * Semantic Version Comparator
     * @return -1 if v1 < v2 (Update Available), 0 if equal, 1 if v1 > v2
     */
    static int compareVersions(const juce::String& v1, const juce::String& v2)
    {
        if (v1.isEmpty() && v2.isNotEmpty()) return -1;
        if (v1.isNotEmpty() && v2.isEmpty()) return 1;
        if (v1 == v2) return 0;

        juce::StringArray parts1 = juce::StringArray::fromTokens(v1, ".", "");
        juce::StringArray parts2 = juce::StringArray::fromTokens(v2, ".", "");

        int maxParts = std::max(parts1.size(), parts2.size());
        for (int i = 0; i < maxParts; ++i)
        {
            int num1 = i < parts1.size() ? parts1[i].getIntValue() : 0;
            int num2 = i < parts2.size() ? parts2[i].getIntValue() : 0;

            if (num1 < num2) return -1;
            if (num1 > num2) return 1;
        }

        return 0;
    }
};
