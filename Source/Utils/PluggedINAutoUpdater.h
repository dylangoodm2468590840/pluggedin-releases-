#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include "InstalledRegistry.h"
#include <atomic>
#include <map>
#include <vector>

struct CloudPluginEntry
{
    juce::String id;
    juce::String name;
    juce::String subtitle;
    juce::String category;
    juce::String description;
    juce::String latestVersion { "1.0.0" };
    juce::String downloadUrl;
    juce::String sha256;
    juce::String changelog;
    bool updateAvailable { false };
};

/**
 * @class PluggedINAutoUpdater
 * @brief Non-blocking background thread cloud version checker for PluggedIN Audio suite.
 */
class PluggedINAutoUpdater : private juce::Thread
{
public:
    PluggedINAutoUpdater();
    ~PluggedINAutoUpdater() override;

    void checkForUpdatesAsync(const juce::String& currentVersionStr) noexcept;

    bool isUpdateAvailable() const noexcept { return updateAvailable.load(); }
    juce::String getCurrentVersion() const noexcept { return currentVersion; }
    juce::String getLatestVersion() const noexcept { return latestVersion; }
    juce::String getManagerSha256() const noexcept { return managerSha256; }
    juce::String getChangelog() const noexcept { return changelog; }
    juce::String getDownloadUrl() const noexcept { return downloadUrl; }

    void clearUpdateState() noexcept;

    bool isPluginUpdateAvailable(const juce::String& pluginId) const noexcept;
    juce::String getPluginLatestVersion(const juce::String& pluginId) const noexcept;
    juce::String getPluginDownloadUrl(const juce::String& pluginId) const noexcept;
    juce::String getPluginSha256(const juce::String& pluginId) const noexcept;
    juce::String getPluginChangelog(const juce::String& pluginId) const noexcept;
    juce::String getPluginName(const juce::String& pluginId) const noexcept;
    std::vector<juce::String> getDiscoveredPluginIds() const noexcept;
    std::vector<CloudPluginEntry> getAllDiscoveredPlugins() const noexcept;

private:
    void run() override;
    void parseManifestJson(const juce::String& jsonText);

    juce::String currentVersion { "2.2.0" };
    std::atomic<bool> updateAvailable { false };

    juce::String latestVersion { "2.2.0" };
    juce::String managerSha256 { "" };
    juce::String changelog { "" };
    juce::String downloadUrl { "" };

    mutable juce::CriticalSection cacheLock;
    std::map<juce::String, CloudPluginEntry> pluginCatalogCache;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluggedINAutoUpdater)
};

