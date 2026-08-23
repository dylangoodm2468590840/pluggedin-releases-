#pragma once

#include <juce_core/juce_core.h>
#include <vector>

/**
 * @enum PluginInstallState
 * @brief Formal state machine for plugin installation & lifecycle.
 */
enum class PluginInstallState
{
    NotInstalled,
    Downloading,
    Verifying,
    Installing,
    Installed,
    UpdateAvailable,
    Updating,
    Repairing,
    Failed
};

/**
 * @struct PluginProduct
 * @brief Authoritative ecosystem metadata for an audio plugin or virtual instrument.
 */
struct PluginProduct
{
    juce::String id;
    juce::String name;
    juce::String subtitle;
    juce::String category;
    juce::String description;
    std::vector<juce::String> dspHighlights;
    std::vector<juce::String> supportedFormats;
    juce::String minOsVersion;

    juce::String installedVersion;
    juce::String latestVersion;
    juce::String changelog;
    juce::String downloadUrl;
    juce::String sha256;

    PluginInstallState state { PluginInstallState::NotInstalled };
    float progress { 0.0f };
    juce::String statusMessage { "" };
    juce::String errorMessage { "" };
    bool isAvailableInCloud { true };

    bool isInstalled() const noexcept
    {
        return state == PluginInstallState::Installed || state == PluginInstallState::UpdateAvailable;
    }

    bool needsUpdate() const noexcept
    {
        return state == PluginInstallState::UpdateAvailable;
    }

    bool isBusy() const noexcept
    {
        return state == PluginInstallState::Downloading ||
               state == PluginInstallState::Verifying ||
               state == PluginInstallState::Installing ||
               state == PluginInstallState::Updating ||
               state == PluginInstallState::Repairing;
    }
};
