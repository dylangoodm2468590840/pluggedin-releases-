#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include "InstalledRegistry.h"
#include <atomic>

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

    void clearUpdateState() noexcept
    {
        updateAvailable.store(false);
        undergroundUpdateAvailable.store(false);
    }

    bool isPluginUpdateAvailable(const juce::String& pluginId) const noexcept;
    juce::String getPluginLatestVersion(const juce::String& pluginId) const noexcept;
    juce::String getPluginDownloadUrl(const juce::String& pluginId) const noexcept;
    juce::String getPluginSha256(const juce::String& pluginId) const noexcept;
    juce::String getPluginChangelog(const juce::String& pluginId) const noexcept;

private:
    void run() override;
    void parseManifestJson(const juce::String& jsonText);

    juce::String currentVersion { "2.1.0" };
    std::atomic<bool> updateAvailable { false };

    juce::String latestVersion { "2.1.0" };
    juce::String managerSha256 { "" };
    juce::String changelog { "" };
    juce::String downloadUrl { "" };

    // Plugin Catalog Cache - UNDERGROUND
    juce::String undergroundLatestVersion { "" };
    juce::String undergroundDownloadUrl { "" };
    juce::String undergroundSha256 { "" };
    juce::String undergroundChangelog { "" };
    std::atomic<bool> undergroundUpdateAvailable { false };

    // Plugin Catalog Cache - PLUGGED 1
    juce::String plugged1LatestVersion { "" };
    juce::String plugged1DownloadUrl { "" };
    juce::String plugged1Sha256 { "" };
    juce::String plugged1Changelog { "" };
    std::atomic<bool> plugged1UpdateAvailable { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluggedINAutoUpdater)
};
