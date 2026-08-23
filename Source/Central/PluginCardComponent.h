#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "CentralDesignSystem.h"
#include "PluginDataModel.h"
#include "PluginDetailModal.h"
#include <functional>

/**
 * @class PluginCardComponent
 * @brief Interactive commercial card representing an individual audio plugin in Central.
 */
class PluginCardComponent : public juce::Component
{
public:
    std::function<void(const juce::String&)> onInstallClicked;
    std::function<void(const juce::String&)> onRepairClicked;
    std::function<void(const juce::String&)> onUninstallClicked;
    std::function<void(const juce::String&)> onOpenFolderClicked;
    std::function<void(const juce::String&)> onViewDetailsClicked;

    PluginCardComponent(const PluginProduct& product, bool isDarkMode = true)
        : pluginData(product), darkMode(isDarkMode)
    {
        addAndMakeVisible(actionButton);
        actionButton.onClick = [this]
        {
            if (pluginData.needsUpdate() || !pluginData.isInstalled() || pluginData.state == PluginInstallState::Failed)
            {
                if (onInstallClicked)
                    onInstallClicked(pluginData.id);
            }
            else if (pluginData.isInstalled())
            {
                if (onViewDetailsClicked)
                    onViewDetailsClicked(pluginData.id);
            }
        };

        addAndMakeVisible(optionsButton);
        optionsButton.setButtonText("OPTIONS");
        optionsButton.onClick = [this] { showOptionsMenu(); };

        updateButtonState();
    }

    void updateData(const PluginProduct& updatedProduct, bool isDark)
    {
        pluginData = updatedProduct;
        darkMode = isDark;
        updateButtonState();
        repaint();
    }

    void updateButtonState()
    {
        if (pluginData.isBusy())
        {
            actionButton.setButtonText(pluginData.statusMessage.isNotEmpty() ? pluginData.statusMessage.toUpperCase() : "PROCESSING...");
            actionButton.setColour(juce::TextButton::buttonColourId, CentralDesignSystem::amber(darkMode).withAlpha(0.3f));
            actionButton.setColour(juce::TextButton::textColourOffId, CentralDesignSystem::amber(darkMode));
            actionButton.setEnabled(false);
        }
        else if (pluginData.needsUpdate())
        {
            actionButton.setButtonText("UPDATE (" + pluginData.latestVersion + ")");
            actionButton.setColour(juce::TextButton::buttonColourId, CentralDesignSystem::amber(darkMode));
            actionButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff000000));
            actionButton.setEnabled(true);
        }
        else if (pluginData.isInstalled())
        {
            actionButton.setButtonText("INSTALLED (" + pluginData.installedVersion + ")");
            actionButton.setColour(juce::TextButton::buttonColourId, darkMode ? juce::Colour(0xff15241b) : juce::Colour(0xffdcfce7));
            actionButton.setColour(juce::TextButton::textColourOffId, CentralDesignSystem::mint(darkMode));
            actionButton.setEnabled(true);
        }
        else
        {
            actionButton.setButtonText("INSTALL");
            actionButton.setColour(juce::TextButton::buttonColourId, CentralDesignSystem::cyan(darkMode).withAlpha(0.2f));
            actionButton.setColour(juce::TextButton::textColourOffId, CentralDesignSystem::cyan(darkMode));
            actionButton.setEnabled(true);
        }

        optionsButton.setColour(juce::TextButton::buttonColourId, darkMode ? juce::Colour(0xff161c28) : juce::Colour(0xffe2e8f0));
        optionsButton.setColour(juce::TextButton::textColourOffId, CentralDesignSystem::textSecondary(darkMode));
        optionsButton.setVisible(pluginData.isInstalled() && !pluginData.isBusy());
    }

    void showOptionsMenu()
    {
        juce::PopupMenu menu;
        menu.addItem(1, "View Details & Changelog");
        menu.addItem(2, "Reinstall / Repair " + pluginData.name, pluginData.isInstalled());
        menu.addSeparator();
        menu.addItem(3, "Open VST3 Installation Folder");
        menu.addItem(4, "Copy Plugin Diagnostics");
        menu.addSeparator();
        menu.addItem(5, "Uninstall " + pluginData.name, pluginData.isInstalled());

        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&optionsButton),
                           [this](int result)
                           {
                               if (result == 1 && onViewDetailsClicked) onViewDetailsClicked(pluginData.id);
                               else if (result == 2 && onRepairClicked) onRepairClicked(pluginData.id);
                               else if (result == 3 && onOpenFolderClicked) onOpenFolderClicked(pluginData.id);
                               else if (result == 4)
                               {
                                   juce::String diag = "Plugin: " + pluginData.name + "\nID: " + pluginData.id +
                                                       "\nInstalled Version: " + pluginData.installedVersion +
                                                       "\nLatest Version: " + pluginData.latestVersion +
                                                       "\nState: " + juce::String((int)pluginData.state);
                                   juce::SystemClipboard::copyTextToClipboard(diag);
                                   juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                       "Diagnostics Copied",
                                       "Diagnostic details for " + pluginData.name + " copied to clipboard!",
                                       "OK");
                               }
                               else if (result == 5 && onUninstallClicked) onUninstallClicked(pluginData.id);
                           });
    }

    void mouseEnter(const juce::MouseEvent&) override { isHovered = true; repaint(); }
    void mouseExit(const juce::MouseEvent&) override  { isHovered = false; repaint(); }

    void mouseDoubleClick(const juce::MouseEvent&) override
    {
        if (onViewDetailsClicked)
            onViewDetailsClicked(pluginData.id);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);

        // 1. Card Surface Fill & Gradient
        juce::Colour cardBg = isHovered ? CentralDesignSystem::bgCardHover(darkMode) : CentralDesignSystem::bgCard(darkMode);
        juce::Colour cardBorder = pluginData.needsUpdate()
                                      ? CentralDesignSystem::amber(darkMode).withAlpha(0.6f)
                                      : (pluginData.isInstalled() ? CentralDesignSystem::mint(darkMode).withAlpha(0.4f)
                                                                  : CentralDesignSystem::borderSubtle(darkMode));

        g.setColour(cardBg);
        g.fillRoundedRectangle(bounds, 8.0f);
        g.setColour(cardBorder);
        g.drawRoundedRectangle(bounds, 8.0f, isHovered ? 1.4f : 1.0f);

        // 2. Left Accent Indicator Strip
        auto accentStrip = bounds.removeFromLeft(5.0f);
        juce::Colour accentCol = pluginData.needsUpdate()
                                     ? CentralDesignSystem::amber(darkMode)
                                     : (pluginData.isInstalled() ? CentralDesignSystem::mint(darkMode)
                                                                 : CentralDesignSystem::cyan(darkMode));
        g.setColour(accentCol);
        g.fillRoundedRectangle(accentStrip, 3.0f);

        // 3. Card Typography & Information
        float textX = bounds.getX() + 18.0f;
        float textW = bounds.getWidth() - 250.0f;

        // Title + Version
        g.setColour(CentralDesignSystem::textPrimary(darkMode));
        g.setFont(juce::Font(16.0f, juce::Font::bold));
        juce::String displayTitle = pluginData.name + " — " + pluginData.subtitle;
        g.drawText(displayTitle, (int)textX, 16, (int)textW, 22, juce::Justification::left, true);

        // Description
        g.setColour(CentralDesignSystem::textSecondary(darkMode));
        g.setFont(juce::Font(11.5f, juce::Font::plain));
        g.drawText(pluginData.description, (int)textX, 42, (int)textW, 36, juce::Justification::left, true);

        // Format and Category Pills
        float pillX = textX;
        float pillY = 86.0f;

        // Category Badge
        auto catRect = juce::Rectangle<float>(pillX, pillY, 110, 18);
        CentralDesignSystem::drawPillBadge(g, catRect, pluginData.category.toUpperCase(),
                                          darkMode ? juce::Colour(0xff182230) : juce::Colour(0xffe0f2fe),
                                          CentralDesignSystem::cyan(darkMode), 3.0f, 9.0f);
        pillX += 118.0f;

        // Formats Badge
        juce::String formatStr = "VST3 64-BIT  •  STANDALONE  •  ALL DAWS";
        auto fmtRect = juce::Rectangle<float>(pillX, pillY, 210, 18);
        CentralDesignSystem::drawPillBadge(g, fmtRect, formatStr,
                                          darkMode ? juce::Colour(0xff15201b) : juce::Colour(0xffdcfce7),
                                          CentralDesignSystem::mint(darkMode), 3.0f, 9.0f);

        // 4. In-Progress Download / Extraction Bar
        if (pluginData.isBusy() && pluginData.progress > 0.0f)
        {
            auto progressRect = juce::Rectangle<float>(bounds.getWidth() - 230, 84, 220, 18);
            CentralDesignSystem::drawProgressBar(g, progressRect, pluginData.progress,
                                                pluginData.statusMessage, CentralDesignSystem::amber(darkMode), darkMode);
        }
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        int btnW = 145;
        int btnH = 38;
        int optW = 75;

        int rightX = bounds.getWidth() - 16;

        // Options Button
        rightX -= optW;
        optionsButton.setBounds(rightX, 22, optW, btnH);

        // Primary Action Button
        rightX -= (btnW + 8);
        actionButton.setBounds(rightX, 22, btnW, btnH);
    }

    const PluginProduct& getPluginData() const noexcept { return pluginData; }

private:
    PluginProduct pluginData;
    bool darkMode { true };
    bool isHovered { false };

    juce::TextButton actionButton;
    juce::TextButton optionsButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginCardComponent)
};
