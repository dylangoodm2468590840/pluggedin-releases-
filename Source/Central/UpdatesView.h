#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "CentralDesignSystem.h"
#include "PluginDataModel.h"
#include "PluginCardComponent.h"
#include <vector>
#include <functional>

/**
 * @class UpdatesViewComponent
 * @brief Dedicated Hub for managing and executing plugin & Central updates.
 */
class UpdatesViewComponent : public juce::Component
{
public:
    std::function<void()> onCheckUpdatesClicked;
    std::function<void()> onUpdateAllClicked;
    std::function<void(const juce::String&)> onInstallClicked;
    std::function<void(const juce::String&)> onRepairClicked;
    std::function<void(const juce::String&)> onUninstallClicked;
    std::function<void(const juce::String&)> onOpenFolderClicked;
    std::function<void(const juce::String&)> onViewDetailsClicked;

    UpdatesViewComponent(bool isDarkMode = true)
        : darkMode(isDarkMode)
    {
        addAndMakeVisible(checkUpdatesButton);
        checkUpdatesButton.setButtonText("CHECK FOR UPDATES NOW");
        checkUpdatesButton.onClick = [this]
        {
            if (onCheckUpdatesClicked)
                onCheckUpdatesClicked();
        };

        addAndMakeVisible(updateAllButton);
        updateAllButton.setButtonText("UPDATE ALL PLUGINS");
        updateAllButton.onClick = [this]
        {
            if (onUpdateAllClicked)
                onUpdateAllClicked();
        };

        updateTheme(isDarkMode);
    }

    void updateTheme(bool isDark)
    {
        darkMode = isDark;
        checkUpdatesButton.setColour(juce::TextButton::buttonColourId, darkMode ? juce::Colour(0xff141c28) : juce::Colour(0xffe2e8f0));
        checkUpdatesButton.setColour(juce::TextButton::textColourOffId, CentralDesignSystem::cyan(darkMode));

        updateAllButton.setColour(juce::TextButton::buttonColourId, CentralDesignSystem::amber(darkMode));
        updateAllButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff000000));
        repaint();
    }

    void setPlugins(const std::vector<PluginProduct>& allPlugins, bool isDark)
    {
        darkMode = isDark;
        updateList.clear();

        for (const auto& p : allPlugins)
        {
            if (p.needsUpdate())
                updateList.push_back(p);
        }

        // Rebuild cards
        cards.clear();
        for (const auto& p : updateList)
        {
            auto card = std::make_unique<PluginCardComponent>(p, darkMode);
            card->onInstallClicked     = [this](const juce::String& id) { if (onInstallClicked) onInstallClicked(id); };
            card->onRepairClicked      = [this](const juce::String& id) { if (onRepairClicked) onRepairClicked(id); };
            card->onUninstallClicked   = [this](const juce::String& id) { if (onUninstallClicked) onUninstallClicked(id); };
            card->onOpenFolderClicked  = [this](const juce::String& id) { if (onOpenFolderClicked) onOpenFolderClicked(id); };
            card->onViewDetailsClicked = [this](const juce::String& id) { if (onViewDetailsClicked) onViewDetailsClicked(id); };

            addAndMakeVisible(card.get());
            cards.push_back(std::move(card));
        }

        updateAllButton.setVisible(!updateList.empty());
        resized();
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        if (updateList.empty())
        {
            // --- Clean, Confidence-Inspiring "Up To Date" Empty State ---
            auto centerBox = juce::Rectangle<float>(bounds.getCentreX() - 240, bounds.getCentreY() - 140, 480, 240);
            g.setColour(CentralDesignSystem::bgCard(darkMode));
            g.fillRoundedRectangle(centerBox, 10.0f);
            g.setColour(CentralDesignSystem::borderSubtle(darkMode));
            g.drawRoundedRectangle(centerBox, 10.0f, 1.0f);

            // Shield / Check icon graphic
            float iconX = centerBox.getCentreX();
            float iconY = centerBox.getY() + 38.0f;
            g.setColour(CentralDesignSystem::mint(darkMode));
            g.fillEllipse(iconX - 22, iconY - 22, 44, 44);
            g.setColour(darkMode ? juce::Colour(0xff090c13) : juce::Colour(0xffffffff));
            g.setFont(juce::Font(22.0f, juce::Font::bold));
            g.drawText("✓", (int)iconX - 22, (int)iconY - 22, 44, 44, juce::Justification::centred);

            // Heading & Subtitle
            g.setColour(CentralDesignSystem::textPrimary(darkMode));
            g.setFont(juce::Font(18.0f, juce::Font::bold));
            g.drawText("Your Audio Suite is Up to Date", centerBox.getX(), iconY + 34.0f, centerBox.getWidth(), 24, juce::Justification::centred);

            g.setColour(CentralDesignSystem::textSecondary(darkMode));
            g.setFont(juce::Font(12.0f, juce::Font::plain));
            g.drawText("All installed plugins and the Central application are running the latest production releases.",
                       centerBox.getX() + 20, iconY + 62.0f, centerBox.getWidth() - 40, 36, juce::Justification::centred, true);
        }
        else
        {
            // Section Title
            g.setColour(CentralDesignSystem::textPrimary(darkMode));
            g.setFont(juce::Font(18.0f, juce::Font::bold));
            g.drawText("Available Cloud Updates (" + juce::String((int)updateList.size()) + ")", 0, 10, bounds.getWidth() - 200, 26, juce::Justification::left);
        }
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        if (updateList.empty())
        {
            int btnW = 220;
            int btnH = 38;
            checkUpdatesButton.setBounds(bounds.getCentreX() - btnW / 2, bounds.getCentreY() + 40, btnW, btnH);
            checkUpdatesButton.setVisible(true);
            updateAllButton.setVisible(false);
        }
        else
        {
            checkUpdatesButton.setVisible(false);
            updateAllButton.setBounds(bounds.getWidth() - 200, 6, 190, 34);

            int cardY = 48;
            int cardH = 120;
            int spacing = 12;

            for (auto& card : cards)
            {
                card->setBounds(0, cardY, bounds.getWidth(), cardH);
                cardY += cardH + spacing;
            }
        }
    }

private:
    bool darkMode { true };
    std::vector<PluginProduct> updateList;
    std::vector<std::unique_ptr<PluginCardComponent>> cards;

    juce::TextButton checkUpdatesButton;
    juce::TextButton updateAllButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UpdatesViewComponent)
};
