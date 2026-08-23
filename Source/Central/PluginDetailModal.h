#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "CentralDesignSystem.h"
#include "PluginDataModel.h"
#include "../Utils/InstalledRegistry.h"
#include <functional>

/**
 * @class PluginDetailModalComponent
 * @brief Commercial-grade detailed product overview, DSP architecture spec, and changelog viewer.
 */
class PluginDetailModalComponent : public juce::Component
{
public:
    std::function<void()> onPrimaryAction;
    std::function<void()> onRepairAction;
    std::function<void()> onUninstallAction;

    PluginDetailModalComponent(const PluginProduct& product, bool isDarkMode = true)
        : pluginData(product), darkMode(isDarkMode)
    {
        // 1. Primary Action Button (Install / Update / Reinstall)
        addAndMakeVisible(primaryButton);
        updatePrimaryButton();
        primaryButton.onClick = [this]
        {
            if (onPrimaryAction)
                onPrimaryAction();
        };

        // 2. Secondary / Repair Button
        if (pluginData.isInstalled())
        {
            addAndMakeVisible(repairButton);
            repairButton.setButtonText("REPAIR / REINSTALL");
            repairButton.setColour(juce::TextButton::buttonColourId, darkMode ? juce::Colour(0xff18202e) : juce::Colour(0xffe2e8f0));
            repairButton.setColour(juce::TextButton::textColourOffId, CentralDesignSystem::cyan(darkMode));
            repairButton.onClick = [this]
            {
                if (onRepairAction)
                    onRepairAction();
            };

            addAndMakeVisible(uninstallButton);
            uninstallButton.setButtonText("UNINSTALL");
            uninstallButton.setColour(juce::TextButton::buttonColourId, darkMode ? juce::Colour(0xff2a141b) : juce::Colour(0xfffee2e2));
            uninstallButton.setColour(juce::TextButton::textColourOffId, CentralDesignSystem::crimson(darkMode));
            uninstallButton.onClick = [this]
            {
                if (onUninstallAction)
                    onUninstallAction();
            };
        }

        // 3. Open Folder Button
        addAndMakeVisible(openFolderButton);
        openFolderButton.setButtonText("OPEN VST3 FOLDER");
        openFolderButton.setColour(juce::TextButton::buttonColourId, darkMode ? juce::Colour(0xff141822) : juce::Colour(0xffe2e8f0));
        openFolderButton.setColour(juce::TextButton::textColourOffId, CentralDesignSystem::textSecondary(darkMode));
        openFolderButton.onClick = [this]
        {
            juce::File vst3Dir = InstalledRegistry::getUserVst3Directory();
            if (!vst3Dir.exists()) vst3Dir.createDirectory();
            vst3Dir.startAsProcess();
        };

        setSize(560, 480);
    }

    void updatePrimaryButton()
    {
        if (pluginData.needsUpdate())
        {
            primaryButton.setButtonText("UPDATE TO v" + pluginData.latestVersion);
            primaryButton.setColour(juce::TextButton::buttonColourId, CentralDesignSystem::amber(darkMode));
            primaryButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff000000));
        }
        else if (pluginData.isInstalled())
        {
            primaryButton.setButtonText("INSTALLED (v" + pluginData.installedVersion + ")");
            primaryButton.setColour(juce::TextButton::buttonColourId, darkMode ? juce::Colour(0xff15241b) : juce::Colour(0xffdcfce7));
            primaryButton.setColour(juce::TextButton::textColourOffId, CentralDesignSystem::mint(darkMode));
        }
        else
        {
            primaryButton.setButtonText("INSTALL PLUGIN");
            primaryButton.setColour(juce::TextButton::buttonColourId, CentralDesignSystem::cyan(darkMode).withAlpha(0.25f));
            primaryButton.setColour(juce::TextButton::textColourOffId, CentralDesignSystem::cyan(darkMode));
        }
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // 1. Background Modal Fill
        g.fillAll(CentralDesignSystem::bgHeader(darkMode));
        g.setColour(CentralDesignSystem::borderSubtle(darkMode));
        g.drawRect(bounds, 1.0f);

        // 2. Header Banner Area
        auto headerRect = juce::Rectangle<float>(0, 0, bounds.getWidth(), 80);
        juce::ColourGradient headGrad(
            darkMode ? juce::Colour(0xff131a26) : juce::Colour(0xfff1f5f9), 0, 0,
            CentralDesignSystem::bgHeader(darkMode), 0, 80, false);
        g.setGradientFill(headGrad);
        g.fillRect(headerRect);

        g.setColour(CentralDesignSystem::borderSubtle(darkMode));
        g.drawHorizontalLine(80, 0, bounds.getWidth());

        // Header Title & Category
        g.setColour(CentralDesignSystem::cyan(darkMode));
        g.setFont(juce::Font(20.0f, juce::Font::bold));
        g.drawText(pluginData.name, 24, 16, 320, 26, juce::Justification::left);

        g.setColour(CentralDesignSystem::textSecondary(darkMode));
        g.setFont(juce::Font(12.0f, juce::Font::plain));
        g.drawText(pluginData.subtitle, 24, 44, 340, 18, juce::Justification::left);

        // Category Badge in Top Right
        auto badgeRect = juce::Rectangle<float>(bounds.getWidth() - 150, 22, 126, 24);
        CentralDesignSystem::drawPillBadge(g, badgeRect, pluginData.category.toUpperCase(),
                                          darkMode ? juce::Colour(0xff182232) : juce::Colour(0xffe0f2fe),
                                          CentralDesignSystem::cyan(darkMode), 4.0f, 9.5f);

        // 3. Main Info Section
        float y = 96.0f;

        // Description Box
        g.setColour(CentralDesignSystem::textPrimary(darkMode));
        g.setFont(juce::Font(13.0f, juce::Font::bold));
        g.drawText("PRODUCT OVERVIEW & DSP ARCHITECTURE", 24, (int)y, 400, 18, juce::Justification::left);
        y += 22.0f;

        g.setColour(CentralDesignSystem::textSecondary(darkMode));
        g.setFont(juce::Font(11.5f, juce::Font::plain));
        g.drawText(pluginData.description, 24, (int)y, bounds.getWidth() - 48, 36, juce::Justification::left, true);
        y += 42.0f;

        // DSP Features List
        if (!pluginData.dspHighlights.empty())
        {
            g.setColour(CentralDesignSystem::mint(darkMode));
            g.setFont(juce::Font(11.0f, juce::Font::plain));
            for (const auto& feat : pluginData.dspHighlights)
            {
                g.drawText("✓ " + feat, 28, (int)y, bounds.getWidth() - 56, 16, juce::Justification::left);
                y += 18.0f;
            }
            y += 6.0f;
        }

        // 4. Release Notes / Changelog Box
        auto changeRect = juce::Rectangle<float>(24, y, bounds.getWidth() - 48, 110);
        g.setColour(CentralDesignSystem::bgCard(darkMode));
        g.fillRoundedRectangle(changeRect, 6.0f);
        g.setColour(CentralDesignSystem::borderSubtle(darkMode));
        g.drawRoundedRectangle(changeRect, 6.0f, 1.0f);

        g.setColour(CentralDesignSystem::textPrimary(darkMode));
        g.setFont(juce::Font(11.0f, juce::Font::bold));
        g.drawText("RELEASE NOTES (v" + (pluginData.latestVersion.isNotEmpty() ? pluginData.latestVersion : "Latest") + ")",
                   changeRect.reduced(10).removeFromTop(16), juce::Justification::left);

        g.setColour(CentralDesignSystem::textSecondary(darkMode));
        g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 10.5f, juce::Font::plain));
        juce::String noteText = pluginData.changelog.isNotEmpty() ? pluginData.changelog : "• Stable production release verified with zero-latency DSP core.";
        g.drawText(noteText, changeRect.reduced(10).withTrimmedTop(20), juce::Justification::topLeft, true);

        y += 120.0f;

        // 5. Formats & System Compatibility Footer
        g.setColour(CentralDesignSystem::textMuted(darkMode));
        g.setFont(juce::Font(9.5f, juce::Font::bold));
        g.drawText("FORMATS: VST3 (64-BIT)  •  STANDALONE  •  AU (MACOS)  |  WINDOWS 10/11 & MACOS 11+",
                   24, (int)y, bounds.getWidth() - 48, 16, juce::Justification::left);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        int btnY = bounds.getHeight() - 48;

        int rightX = bounds.getWidth() - 24;

        // Primary Action Button
        rightX -= 170;
        primaryButton.setBounds(rightX, btnY, 170, 36);

        // Repair Button
        if (repairButton.isVisible())
        {
            rightX -= 150;
            repairButton.setBounds(rightX, btnY, 140, 36);
        }

        // Uninstall Button
        if (uninstallButton.isVisible())
        {
            rightX -= 110;
            uninstallButton.setBounds(rightX, btnY, 100, 36);
        }

        // Open Folder Button on Left
        openFolderButton.setBounds(24, btnY, 140, 36);
    }

private:
    PluginProduct pluginData;
    bool darkMode { true };

    juce::TextButton primaryButton;
    juce::TextButton repairButton;
    juce::TextButton uninstallButton;
    juce::TextButton openFolderButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginDetailModalComponent)
};
