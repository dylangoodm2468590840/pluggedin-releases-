#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_core/juce_core.h>
#include "../UI/RackUnitLookAndFeel.h"
#include "../Utils/PluggedINAutoUpdater.h"
#include "../Utils/InstalledRegistry.h"
#include "../Utils/CloudDownloader.h"
#include "../Utils/ManagerSelfUpdater.h"
#include "../Utils/StudioDiagnostics.h"
#include "AuthModal.h"
#include "SplashScreenComponent.h"

/**
 * @class CentralMainComponent
 * @brief Commercial-grade Audio Plugin Management Hub with Theme Toggle & Resizable Layout.
 */
class CentralMainComponent : public juce::Component, private juce::Timer
{
public:
    enum class NavSection
    {
        MyProducts,
        AllPlugins,
        Updates,
        Diagnostics
    };

    CentralMainComponent()
    {
        setLookAndFeel(&customLookAndFeel);

        // 1. Sidebar Navigation Buttons
        setupNavButton(navMyProducts, "MY PRODUCTS");
        setupNavButton(navAllPlugins, "ALL PLUGINS");
        setupNavButton(navUpdates,    "UPDATES");
        setupNavButton(navSettings,   "STUDIO TOOLS");

        auto refreshCloud = [this] {
            juce::String appVer = (juce::JUCEApplication::getInstance() != nullptr) 
                ? juce::JUCEApplication::getInstance()->getApplicationVersion() 
                : autoUpdater.getCurrentVersion();
            autoUpdater.checkForUpdatesAsync(appVer);
        };

        navMyProducts.onClick = [this, refreshCloud] { currentSection = NavSection::MyProducts; refreshCloud(); repaint(); };
        navAllPlugins.onClick = [this, refreshCloud] { currentSection = NavSection::AllPlugins; refreshCloud(); repaint(); };
        navUpdates.onClick    = [this, refreshCloud] { currentSection = NavSection::Updates; refreshCloud(); repaint(); };
        navSettings.onClick   = [this, refreshCloud] { currentSection = NavSection::Diagnostics; refreshCloud(); repaint(); };

        // 2. Search Box
        addAndMakeVisible(searchInput);
        searchInput.setTextToShowWhenEmpty("Search plugins & audio tools...", juce::Colour(0xff4a5465));
        searchInput.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff12161f));
        searchInput.setColour(juce::TextEditor::textColourId, juce::Colours::white);
        searchInput.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff1e2633));
        searchInput.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0xff00f0ff));

        // 3. Theme Toggle Button (Dark Mode / Light Mode)
        addAndMakeVisible(themeToggleButton);
        themeToggleButton.setButtonText("THEME: DARK");
        themeToggleButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff141820));
        themeToggleButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00f0ff));
        themeToggleButton.onClick = [this]
        {
            isDarkMode = !isDarkMode;
            themeToggleButton.setButtonText(isDarkMode ? "THEME: DARK" : "THEME: LIGHT");
            applyThemeColors();
            repaint();
        };

        // 4. Header Controls
        addAndMakeVisible(accountButton);
        updateAccountButtonState();
        accountButton.onClick = [this] { openAuthDialog(); };

        addAndMakeVisible(statusActionButton);
        statusActionButton.onClick = [this]
        {
            if (autoUpdater.isUpdateAvailable())
            {
                updateManagerApplication();
            }
            else if (autoUpdater.isPluginUpdateAvailable("pluggedin_underground"))
            {
                installUndergroundPlugin();
            }
            else
            {
                statusActionButton.setButtonText("CHECKING CLOUD...");
                autoUpdater.checkForUpdatesAsync(autoUpdater.getCurrentVersion());
            }
        };

        // 5. Quick Action Buttons
        addAndMakeVisible(rescanDawsButton);
        rescanDawsButton.setButtonText("RESCAN DAWS");
        rescanDawsButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff141820));
        rescanDawsButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff7f8b98));
        rescanDawsButton.onClick = [this] { rescanDAWCache(); };

        addAndMakeVisible(diagnosticsButton);
        diagnosticsButton.setButtonText("DSP TEST");
        diagnosticsButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff141820));
        diagnosticsButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00ff66));
        diagnosticsButton.onClick = [this] { runDiagnostics(); };

        // 6. Plugin Action Controls
        addAndMakeVisible(undergroundInstallButton);
        undergroundInstallButton.onClick = [this] { installUndergroundPlugin(); };

        addAndMakeVisible(undergroundOptionsButton);
        undergroundOptionsButton.setButtonText("OPTIONS");
        undergroundOptionsButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff181d26));
        undergroundOptionsButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00f0ff));
        undergroundOptionsButton.onClick = [this] { showPluginOptionsMenu(); };

        addAndMakeVisible(crushButton);
        crushButton.setButtonText("FREE DOWNLOAD");
        crushButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff00f0ff).withAlpha(0.2f));
        crushButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00f0ff));
        crushButton.onClick = [this] { installCrushPlugin(); };

        // Initial background tasks
        detectedDaws = StudioDiagnostics::scanInstalledDAWs();
        StudioDiagnostics::syncFactoryPresets();

        juce::String appVer = (juce::JUCEApplication::getInstance() != nullptr) 
            ? juce::JUCEApplication::getInstance()->getApplicationVersion() 
            : autoUpdater.getCurrentVersion();
        autoUpdater.checkForUpdatesAsync(appVer);

        // Self-healing boot scan: runs in background on every launch on every computer.
        // Finds the highest installed VST3 version, deletes stale ghost copies in all
        // lower-priority directories (including C:\Program Files\Common Files\VST3),
        // and registers the correct authoritative version before the first UI paint.
        juce::Thread::launch([]
        {
            InstalledRegistry::selfHealInstallations("pluggedin_underground");
            InstalledRegistry::selfHealInstallations("pluggedin_crush");
        });

        checkInstallationStatus();

        setSize(920, 600);
        startTimerHz(10);
    }

    ~CentralMainComponent() override
    {
        stopTimer();
        setLookAndFeel(nullptr);
    }

    void applyThemeColors()
    {
        if (isDarkMode)
        {
            searchInput.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff12161f));
            searchInput.setColour(juce::TextEditor::textColourId, juce::Colours::white);
            searchInput.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff1e2633));

            themeToggleButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff141820));
            themeToggleButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00f0ff));

            rescanDawsButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff141820));
            rescanDawsButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff7f8b98));

            diagnosticsButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff141820));
            diagnosticsButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00ff66));

            setupNavButton(navMyProducts, "MY PRODUCTS");
            setupNavButton(navAllPlugins, "ALL PLUGINS");
            setupNavButton(navUpdates,    "UPDATES");
            setupNavButton(navSettings,   "STUDIO TOOLS");
        }
        else
        {
            // Light Theme
            searchInput.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xffffffff));
            searchInput.setColour(juce::TextEditor::textColourId, juce::Colour(0xff10141c));
            searchInput.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xffcbd5e1));

            themeToggleButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffe2e8f0));
            themeToggleButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff0284c7));

            rescanDawsButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffe2e8f0));
            rescanDawsButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff475569));

            diagnosticsButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffe2e8f0));
            diagnosticsButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff059669));

            navMyProducts.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffe2e8f0));
            navMyProducts.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff1e293b));

            navAllPlugins.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffe2e8f0));
            navAllPlugins.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff1e293b));

            navUpdates.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffe2e8f0));
            navUpdates.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff1e293b));

            navSettings.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffe2e8f0));
            navSettings.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff1e293b));
        }
        updateAccountButtonState();
        checkInstallationStatus();
    }

    void updateAccountButtonState()
    {
        if (AuthManager::isLoggedIn())
        {
            accountButton.setButtonText(AuthManager::getCurrentProducerName() + " | PRO");
            accountButton.setColour(juce::TextButton::buttonColourId, isDarkMode ? juce::Colour(0xff15241b) : juce::Colour(0xffdcfce7));
            accountButton.setColour(juce::TextButton::textColourOffId, isDarkMode ? juce::Colour(0xff00ff66) : juce::Colour(0xff16a34a));
        }
        else
        {
            accountButton.setButtonText("SIGN IN / REGISTER");
            accountButton.setColour(juce::TextButton::buttonColourId, isDarkMode ? juce::Colour(0xff00f0ff).withAlpha(0.18f) : juce::Colour(0xffe0f2fe));
            accountButton.setColour(juce::TextButton::textColourOffId, isDarkMode ? juce::Colour(0xff00f0ff) : juce::Colour(0xff0284c7));
        }
    }

    void openAuthDialog()
    {
        if (AuthManager::isLoggedIn())
        {
            juce::PopupMenu menu;
            menu.addItem(1, "Signed in as " + AuthManager::getCurrentProducerName(), false);
            menu.addSeparator();
            menu.addItem(2, "Sign Out / Switch Account");

            menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&accountButton),
                               [this](int result)
                               {
                                   if (result == 2)
                                   {
                                       AuthManager::logoutUser();
                                       updateAccountButtonState();
                                       repaint();
                                   }
                               });
            return;
        }

        auto* authComp = new AuthModalComponent();
        authComp->onAuthSuccess = [this, authComp]
        {
            updateAccountButtonState();
            repaint();
            if (auto* window = authComp->findParentComponentOfClass<juce::DialogWindow>())
                window->exitModalState(1);
        };

        juce::DialogWindow::LaunchOptions options;
        options.content.setOwned(authComp);
        options.dialogTitle                   = "PluggedIN Audio Account";
        options.dialogBackgroundColour        = isDarkMode ? juce::Colour(0xff0e1116) : juce::Colour(0xfff8fafc);
        options.escapeKeyTriggersCloseButton  = true;
        options.useNativeTitleBar             = true;
        options.resizable                     = false;
        options.launchAsync();
    }

    void runDiagnostics()
    {
        juce::String report;
        bool passed = StudioDiagnostics::runAudioEngineDiagnostics(report);

        juce::AlertWindow::showMessageBoxAsync(
            passed ? juce::AlertWindow::InfoIcon : juce::AlertWindow::WarningIcon,
            "PluggedIN DSP Engine Diagnostics",
            report,
            "DONE"
        );
    }

    void rescanDAWCache()
    {
        detectedDaws = StudioDiagnostics::scanInstalledDAWs();
        checkInstallationStatus();

        juce::String dawList = "";
        for (const auto& daw : detectedDaws)
        {
            if (daw.isInstalled)
                dawList += "• " + daw.name + " (" + daw.version + ")\n";
        }
        if (dawList.isEmpty()) dawList = "No default DAW folders detected in Program Files.";

        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::InfoIcon,
            "DAW Scanner & Cache Rescan",
            "Detected Installed DAWs:\n\n" + dawList + "\nVST3 cache timestamps synchronized!",
            "OK"
        );
        repaint();
    }

    void showPluginOptionsMenu()
    {
        juce::PopupMenu menu;
        menu.addItem(1, "Reinstall / Repair UNDERGROUND", isUndergroundInstalled);
        menu.addItem(2, "Uninstall Plugin", isUndergroundInstalled);
        menu.addSeparator();
        menu.addItem(3, "Open VST3 Installation Folder");
        menu.addItem(4, "View Release Changelog");

        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&undergroundOptionsButton),
                           [this](int result)
                           {
                               if (result == 1)
                               {
                                   installUndergroundPlugin();
                               }
                               else if (result == 2)
                               {
                                   CloudDownloader::uninstallPlugin("pluggedin_underground");
                                   checkInstallationStatus();
                                   repaint();
                               }
                               else if (result == 3)
                               {
                                   juce::File userVst3Dir = InstalledRegistry::getUserVst3Directory();
                                   if (!userVst3Dir.exists()) userVst3Dir.createDirectory();
                                   userVst3Dir.startAsProcess();
                               }
                               else if (result == 4)
                               {
                                   juce::AlertWindow::showMessageBoxAsync(
                                       juce::AlertWindow::InfoIcon,
                                       "UNDERGROUND V3.3.0 Changelog",
                                       "• Split-Bus Direct/Parallel Boutique DSP Architecture.\n"
                                       "• 7 Re-Voiced Reference Presets with 4x Polyphase Oversampling.\n"
                                       "• 100% Mono-Safe 3D Spatial Aura (180Hz Side-Cut).\n"
                                       "• Dynamic Transformer Core & Non-Linear Tube Saturation.\n"
                                       "• Real-Time 4-Band Dynamic Resonance & Harshness Suppression.",
                                       "CLOSE"
                                   );
                               }
                           });
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        float sidebarWidth = 210.0f;
        float headerHeight = 60.0f;

        juce::Colour bgColour     = isDarkMode ? juce::Colour(0xff090b10) : juce::Colour(0xfff1f5f9);
        juce::Colour sideGradTop  = isDarkMode ? juce::Colour(0xff10141c) : juce::Colour(0xffe2e8f0);
        juce::Colour sideGradBot  = isDarkMode ? juce::Colour(0xff0a0d13) : juce::Colour(0xffcbd5e1);
        juce::Colour borderColour = isDarkMode ? juce::Colour(0xff18202c) : juce::Colour(0xffcbd5e1);
        juce::Colour textPrimary  = isDarkMode ? juce::Colours::white     : juce::Colour(0xff0f172a);
        juce::Colour textMuted    = isDarkMode ? juce::Colour(0xff8a99ad) : juce::Colour(0xff64748b);
        juce::Colour headerColour = isDarkMode ? juce::Colour(0xff0e1219) : juce::Colour(0xfff8fafc);
        juce::Colour card1Top     = isDarkMode ? juce::Colour(0xff131822) : juce::Colour(0xffffffff);
        juce::Colour card1Bot     = isDarkMode ? juce::Colour(0xff0d1117) : juce::Colour(0xfff8fafc);
        juce::Colour card2Colour  = isDarkMode ? juce::Colour(0xff0f131a) : juce::Colour(0xfff8fafc);

        // 1. Background
        g.fillAll(bgColour);

        // 2. Left Sidebar
        auto sidebarRect = juce::Rectangle<float>(0, 0, sidebarWidth, bounds.getHeight());
        juce::ColourGradient sideGrad(sideGradTop, 0, 0, sideGradBot, sidebarWidth, bounds.getHeight(), false);
        g.setGradientFill(sideGrad);
        g.fillRect(sidebarRect);

        g.setColour(borderColour);
        g.drawVerticalLine((int)sidebarWidth, 0.0f, bounds.getHeight());

        // Sidebar Logo
        g.setColour(isDarkMode ? juce::Colour(0xff00f0ff) : juce::Colour(0xff0284c7));
        g.setFont(juce::Font(18.0f, juce::Font::bold));
        g.drawText("PLUGGEDIN", 20, 18, 170, 22, juce::Justification::left);

        g.setColour(textMuted);
        g.setFont(juce::Font(10.0f, juce::Font::bold));
        g.drawText("STUDIO HUB", 20, 38, 170, 14, juce::Justification::left);

        // Sidebar Section Label
        g.setColour(textMuted);
        g.setFont(juce::Font(9.5f, juce::Font::bold));
        g.drawText("MANAGE", 20, 78, 170, 14, juce::Justification::left);

        // Sidebar Bottom DAW Status
        float sideBotY = bounds.getHeight() - 90.0f;
        g.setColour(borderColour);
        g.drawHorizontalLine((int)sideBotY, 15.0f, sidebarWidth - 15.0f);

        g.setColour(textMuted);
        g.setFont(juce::Font(9.0f, juce::Font::bold));
        g.drawText("DETECTED HOSTS", 20, (int)sideBotY + 8, 170, 12, juce::Justification::left);

        float pillY = sideBotY + 24.0f;
        for (const auto& daw : detectedDaws)
        {
            if (daw.isInstalled && pillY < bounds.getHeight() - 30.0f)
            {
                g.setColour(isDarkMode ? juce::Colour(0xff00ff66) : juce::Colour(0xff16a34a));
                g.setFont(juce::Font(10.0f, juce::Font::plain));
                g.drawText("• " + daw.name, 22, (int)pillY, 160, 14, juce::Justification::left);
                pillY += 16.0f;
            }
        }

        // Sidebar Version Footer
        g.setColour(textMuted);
        g.setFont(juce::Font(9.5f, juce::Font::plain));
        g.drawText("v" + autoUpdater.getCurrentVersion() + " | x64 NATIVE", 20, (int)bounds.getHeight() - 20, 170, 14, juce::Justification::left);

        // 3. Header Bar
        auto contentRect = juce::Rectangle<float>(sidebarWidth, 0, bounds.getWidth() - sidebarWidth, bounds.getHeight());
        auto headerRect = juce::Rectangle<float>(sidebarWidth, 0, contentRect.getWidth(), headerHeight);

        g.setColour(headerColour);
        g.fillRect(headerRect);
        g.setColour(borderColour);
        g.drawHorizontalLine((int)headerHeight, sidebarWidth, bounds.getWidth());

        // 4. Main Body Area
        float mainX = sidebarWidth + 24.0f;
        float mainWidth = bounds.getWidth() - sidebarWidth - 48.0f;
        float startY = headerHeight + 20.0f;

        // Top Banner / Update Alert
        if (autoUpdater.isUpdateAvailable())
        {
            auto updateBanner = juce::Rectangle<float>(mainX, startY, mainWidth, 32.0f);
            g.setColour(juce::Colour(0xffffaa00).withAlpha(0.2f));
            g.fillRoundedRectangle(updateBanner, 4.0f);
            g.setColour(juce::Colour(0xffffaa00));
            g.drawRoundedRectangle(updateBanner, 4.0f, 1.0f);

            g.setFont(juce::Font(11.0f, juce::Font::bold));
            g.setColour(juce::Colour(0xffffaa00));
            g.drawText("PLUGGEDIN CENTRAL APP UPDATE AVAILABLE (" + autoUpdater.getLatestVersion() + ") -- CLICK STATUS TO UPDATE", updateBanner, juce::Justification::centred);
            startY += 42.0f;
        }

        // Section Title
        g.setColour(textPrimary);
        g.setFont(juce::Font(18.0f, juce::Font::bold));
        juce::String sectionTitle = (currentSection == NavSection::MyProducts) ? "Installed Audio Plugins" :
                                    (currentSection == NavSection::AllPlugins) ? "Universal Plugin Catalog" :
                                    (currentSection == NavSection::Updates) ? "Available Cloud Updates" : "Diagnostics & Studio Setup";
        g.drawText(sectionTitle, (int)mainX, (int)startY, (int)mainWidth, 24, juce::Justification::left);

        startY += 32.0f;

        // 5. Card 1: UNDERGROUND V3.3.0
        auto card1 = juce::Rectangle<float>(mainX, startY, mainWidth, 130.0f);
        juce::ColourGradient card1Grad(card1Top, card1.getX(), card1.getY(), card1Bot, card1.getX(), card1.getBottom(), false);
        g.setGradientFill(card1Grad);
        g.fillRoundedRectangle(card1, 6.0f);

        g.setColour(isUndergroundInstalled ? (isDarkMode ? juce::Colour(0xff00ff66).withAlpha(0.6f) : juce::Colour(0xff16a34a).withAlpha(0.6f))
                                           : (isDarkMode ? juce::Colour(0xff00f0ff).withAlpha(0.6f) : juce::Colour(0xff0284c7).withAlpha(0.6f)));
        g.drawRoundedRectangle(card1, 6.0f, 1.2f);

        // Accent strip
        auto accent1 = card1.removeFromLeft(5.0f);
        g.setColour(isDarkMode ? juce::Colour(0xff00f0ff) : juce::Colour(0xff0284c7));
        g.fillRoundedRectangle(accent1, 3.0f);

        // Card 1 Text
        g.setColour(textPrimary);
        g.setFont(juce::Font(16.0f, juce::Font::bold));
        g.drawText("PluggedIN -- UNDERGROUND V3.3.0 (Boutique Vocal Suite)", (int)mainX + 20, (int)startY + 16, (int)mainWidth - 280, 22, juce::Justification::left);

        g.setColour(textMuted);
        g.setFont(juce::Font(11.5f, juce::Font::plain));
        g.drawText("Boutique Multi-FX Vocal Suite with Split-Bus DSP & 7 Re-Voiced Core Presets", (int)mainX + 20, (int)startY + 42, (int)mainWidth - 280, 18, juce::Justification::left);

        g.setColour(isDarkMode ? juce::Colour(0xff00ff66) : juce::Colour(0xff16a34a));
        g.setFont(juce::Font(9.5f, juce::Font::bold));
        g.drawText("VST3 64-BIT  |  AU  |  CLAP  |  STANDALONE  |  ALL DAWS CERTIFIED", (int)mainX + 20, (int)startY + 72, (int)mainWidth - 280, 16, juce::Justification::left);

        // 6. Card 2: CRUSH (Coming Soon)
        float card2Y = startY + 144.0f;
        if (card2Y + 110.0f < bounds.getHeight())
        {
            auto card2 = juce::Rectangle<float>(mainX, card2Y, mainWidth, 110.0f);
            g.setColour(card2Colour);
            g.fillRoundedRectangle(card2, 6.0f);
            g.setColour(borderColour);
            g.drawRoundedRectangle(card2, 6.0f, 1.0f);

            g.setColour(textPrimary.withAlpha(0.6f));
            g.setFont(juce::Font(15.0f, juce::Font::bold));
            g.drawText("PluggedIN -- CRUSH (Coming Soon)", (int)mainX + 20, (int)card2Y + 16, (int)mainWidth - 280, 20, juce::Justification::left);

            g.setColour(textMuted);
            g.setFont(juce::Font(11.5f, juce::Font::plain));
            g.drawText("Analog Tape, Tube Fuzz & Bitcrusher Saturator Module", (int)mainX + 20, (int)card2Y + 42, (int)mainWidth - 280, 18, juce::Justification::left);
        }
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        float sidebarWidth = 210.0f;
        float headerHeight = 60.0f;

        // Sidebar Navigation Buttons
        int navY = 100;
        int navHeight = 36;
        int navSpacing = 6;

        navMyProducts.setBounds(12, navY, (int)sidebarWidth - 24, navHeight);
        navY += navHeight + navSpacing;

        navAllPlugins.setBounds(12, navY, (int)sidebarWidth - 24, navHeight);
        navY += navHeight + navSpacing;

        navUpdates.setBounds(12, navY, (int)sidebarWidth - 24, navHeight);
        navY += navHeight + navSpacing;

        navSettings.setBounds(12, navY, (int)sidebarWidth - 24, navHeight);

        // Header Controls (Right to Left)
        int rightX = bounds.getWidth() - 16;

        // Account Button
        rightX -= 140;
        accountButton.setBounds(rightX, 12, 135, 36);

        // Theme Toggle Button
        rightX -= 125;
        themeToggleButton.setBounds(rightX, 12, 115, 36);

        // Status Button
        rightX -= 185;
        statusActionButton.setBounds(rightX, 12, 175, 36);

        // Diagnostics
        rightX -= 95;
        diagnosticsButton.setBounds(rightX, 14, 85, 32);

        // Rescan DAWs
        rightX -= 115;
        rescanDawsButton.setBounds(rightX, 14, 105, 32);

        // Search Input
        int searchX = (int)sidebarWidth + 20;
        int searchWidth = juce::jmax(120, rightX - searchX - 16);
        searchInput.setBounds(searchX, 14, searchWidth, 32);

        // Main Viewport Cards Layout
        float startY = headerHeight + 20.0f;
        if (autoUpdater.isUpdateAvailable())
            startY += 42.0f;
        startY += 32.0f; // Section title offset

        int card1Y = (int)startY;
        undergroundInstallButton.setBounds(bounds.getWidth() - 240, card1Y + 45, 150, 40);
        undergroundOptionsButton.setBounds(bounds.getWidth() - 80, card1Y + 45, 60, 40);

        int card2Y = card1Y + 144;
        crushButton.setBounds(bounds.getWidth() - 240, card2Y + 36, 220, 38);
    }

private:
    void setupNavButton(juce::TextButton& btn, const juce::String& text)
    {
        addAndMakeVisible(btn);
        btn.setButtonText(text);
        btn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff121620));
        btn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff8a99ad));
    }

    int pollCloudCounter { 0 };

    void timerCallback() override
    {
        // Re-poll cloud manifest every 15 seconds so Central reflects live releases without restart
        if (++pollCloudCounter >= 150)
        {
            pollCloudCounter = 0;
            juce::String appVer = (juce::JUCEApplication::getInstance() != nullptr) 
                ? juce::JUCEApplication::getInstance()->getApplicationVersion() 
                : autoUpdater.getCurrentVersion();
            autoUpdater.checkForUpdatesAsync(appVer);
        }

        checkInstallationStatus();
        repaint();
    }

    void checkInstallationStatus()
    {
        juce::String installedVersion = InstalledRegistry::getInstalledVersion("pluggedin_underground");
        juce::String cloudVersion = autoUpdater.getPluginLatestVersion("pluggedin_underground");

        bool hasUpdate = false;

        if (installedVersion.isNotEmpty())
        {
            isUndergroundInstalled = true;

            // Only compare if the cloud manifest has actually loaded (non-empty)
            if (cloudVersion.isNotEmpty() && InstalledRegistry::compareVersions(installedVersion, cloudVersion) < 0)
            {
                hasUpdate = true;
                undergroundInstallButton.setButtonText("UPDATE (" + cloudVersion + ")");
                undergroundInstallButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffffaa00));
                undergroundInstallButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff000000));
            }
            else
            {
                undergroundInstallButton.setButtonText("INSTALLED (" + installedVersion + ")");
                undergroundInstallButton.setColour(juce::TextButton::buttonColourId, isDarkMode ? juce::Colour(0xff15241b) : juce::Colour(0xffdcfce7));
                undergroundInstallButton.setColour(juce::TextButton::textColourOffId, isDarkMode ? juce::Colour(0xff00ff66) : juce::Colour(0xff16a34a));
            }
        }
        else
        {
            isUndergroundInstalled = false;
            undergroundInstallButton.setButtonText("INSTALL PLUGIN");
            undergroundInstallButton.setColour(juce::TextButton::buttonColourId, isDarkMode ? juce::Colour(0xff00f0ff).withAlpha(0.2f) : juce::Colour(0xffe0f2fe));
            undergroundInstallButton.setColour(juce::TextButton::textColourOffId, isDarkMode ? juce::Colour(0xff00f0ff) : juce::Colour(0xff0284c7));
        }

        undergroundOptionsButton.setVisible(isUndergroundInstalled);

        // System Status Header Badge Updates
        if (autoUpdater.isUpdateAvailable())
        {
            statusActionButton.setButtonText("MANAGER UPDATE (" + autoUpdater.getLatestVersion() + ")");
            statusActionButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffffaa00).withAlpha(0.25f));
            statusActionButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffffaa00));
            navUpdates.setButtonText("UPDATES (1)");
            navUpdates.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffffaa00));
        }
        else if (hasUpdate)
        {
            statusActionButton.setButtonText("1 PLUGIN UPDATE");
            statusActionButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffffaa00).withAlpha(0.25f));
            statusActionButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffffaa00));
            navUpdates.setButtonText("UPDATES (1)");
            navUpdates.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffffaa00));
        }
        else
        {
            statusActionButton.setButtonText("STATUS: UP TO DATE");
            statusActionButton.setColour(juce::TextButton::buttonColourId, isDarkMode ? juce::Colour(0xff15241b) : juce::Colour(0xffdcfce7));
            statusActionButton.setColour(juce::TextButton::textColourOffId, isDarkMode ? juce::Colour(0xff00ff66) : juce::Colour(0xff16a34a));
            navUpdates.setButtonText("UPDATES");
            navUpdates.setColour(juce::TextButton::textColourOffId, isDarkMode ? juce::Colour(0xff8a99ad) : juce::Colour(0xff1e293b));
        }
    }

    void installUndergroundPlugin()
    {
        if (cloudDownloader.isBusy())
            return;

        undergroundInstallButton.setButtonText("INITIALIZING...");
        undergroundInstallButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffffaa00).withAlpha(0.3f));
        undergroundInstallButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffffaa00));
        repaint();

        cloudDownloader.onProgressState = [this](float fraction, const juce::String& stateLabel)
        {
            undergroundInstallButton.setButtonText(stateLabel.toUpperCase());
            repaint();
        };

        cloudDownloader.onComplete = [this](bool success, const juce::String& error)
        {
            if (success)
            {
                // Re-run self-heal after install: this cleans up any stale ghost copies
                // in other directories so the registry reflects the real new version.
                juce::Thread::launch([]
                {
                    InstalledRegistry::selfHealInstallations("pluggedin_underground");
                });

                // Small delay so the self-heal thread has time to update the registry
                // before we re-read it for the UI status check
                juce::Timer::callAfterDelay(2200, [this]
                {
                    autoUpdater.clearUpdateState();
                    checkInstallationStatus();
                    repaint();
                });
            }
            else
            {
                undergroundInstallButton.setButtonText(error.isNotEmpty() && error.contains("DAW") ? "DAW RUNNING!" : "UPDATE FAILED!");
                undergroundInstallButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3f1015));
                undergroundInstallButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffff0055));

                if (error.isNotEmpty())
                {
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::AlertWindow::WarningIcon,
                        "PluggedIN Installation Notice",
                        error,
                        "OK"
                    );
                }
            }
            repaint();
        };

        juce::String cloudUrl = autoUpdater.getPluginDownloadUrl("pluggedin_underground");
        if (cloudUrl.isEmpty()) cloudUrl = autoUpdater.getDownloadUrl();
        juce::String latestVer = autoUpdater.getPluginLatestVersion("pluggedin_underground");
        juce::String sha = autoUpdater.getPluginSha256("pluggedin_underground");
        cloudDownloader.startDownloadAsync(cloudUrl, "pluggedin_underground", latestVer, sha);
    }

    void installCrushPlugin()
    {
        if (cloudDownloader.isBusy())
            return;

        crushButton.setButtonText("INITIALIZING...");
        crushButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffffaa00).withAlpha(0.3f));
        crushButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffffaa00));
        repaint();

        cloudDownloader.onProgressState = [this](float fraction, const juce::String& stateLabel)
        {
            crushButton.setButtonText(stateLabel.toUpperCase());
            repaint();
        };

        cloudDownloader.onComplete = [this](bool success, const juce::String& error)
        {
            if (success)
            {
                crushButton.setButtonText("INSTALLED (1.0.0)");
                crushButton.setColour(juce::TextButton::buttonColourId, isDarkMode ? juce::Colour(0xff15241b) : juce::Colour(0xffdcfce7));
                crushButton.setColour(juce::TextButton::textColourOffId, isDarkMode ? juce::Colour(0xff00ff66) : juce::Colour(0xff16a34a));
                checkInstallationStatus();
            }
            else
            {
                crushButton.setButtonText(error.isNotEmpty() && error.contains("DAW") ? "DAW RUNNING!" : "FAILED");
                crushButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3f1015));
                crushButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffff0055));
            }
            repaint();
        };

        juce::String cloudUrl = autoUpdater.getPluginDownloadUrl("pluggedin_crush");
        if (cloudUrl.isEmpty()) cloudUrl = "https://files.catbox.moe/yuu96z.zip";
        juce::String latestVer = autoUpdater.getPluginLatestVersion("pluggedin_crush");
        if (latestVer.isEmpty()) latestVer = "1.0.0";
        juce::String sha = autoUpdater.getPluginSha256("pluggedin_crush");
        cloudDownloader.startDownloadAsync(cloudUrl, "pluggedin_crush", latestVer, sha);
    }

    void updateManagerApplication()
    {
        if (managerSelfUpdater.isBusy())
            return;

        statusActionButton.setButtonText("UPDATING MANAGER...");
        statusActionButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffffaa00).withAlpha(0.4f));
        repaint();

        managerSelfUpdater.onProgress = [this](float fraction)
        {
            int pct = static_cast<int>(fraction * 100.0f);
            statusActionButton.setButtonText("DOWNLOADING " + juce::String(pct) + "%");
            repaint();
        };

        managerSelfUpdater.onComplete = [this](bool success, const juce::String& error)
        {
            if (!success)
            {
                statusActionButton.setButtonText("UPDATE FAILED");
                statusActionButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3f1015));
                statusActionButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffff0055));

                if (error.isNotEmpty())
                {
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::AlertWindow::WarningIcon,
                        "Manager Update Notice",
                        error,
                        "OK"
                    );
                }
            }
            repaint();
        };

        juce::String downloadUrl = autoUpdater.getDownloadUrl();
        juce::String latestVer = autoUpdater.getLatestVersion();
        juce::String sha = autoUpdater.getManagerSha256();
        managerSelfUpdater.startSelfUpdateAsync(downloadUrl, latestVer, sha);
    }

    RackUnitLookAndFeel customLookAndFeel;
    PluggedINAutoUpdater autoUpdater;
    CloudDownloader cloudDownloader;
    ManagerSelfUpdater managerSelfUpdater;

    NavSection currentSection { NavSection::MyProducts };
    bool isDarkMode { true };

    juce::TextButton navMyProducts;
    juce::TextButton navAllPlugins;
    juce::TextButton navUpdates;
    juce::TextButton navSettings;

    juce::TextEditor searchInput;
    juce::TextButton themeToggleButton;
    juce::TextButton accountButton;
    juce::TextButton statusActionButton;
    juce::TextButton undergroundInstallButton;
    juce::TextButton undergroundOptionsButton;
    juce::TextButton crushButton;

    juce::TextButton rescanDawsButton;
    juce::TextButton diagnosticsButton;

    std::vector<StudioDiagnostics::DAWInfo> detectedDaws;
    bool isUndergroundInstalled { false };
};

/**
 * @class CentralWindow
 * @brief Resizable / Expandable DocumentWindow host supporting Splash Sequence and CentralMainComponent.
 */
class CentralWindow : public juce::DocumentWindow
{
public:
    CentralWindow(const juce::String& name)
        : juce::DocumentWindow(name,
                               juce::Colour(0xff090b10),
                               juce::DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar(true);

        // Enable resizing and maximize/restore capability
        setResizable(true, true);
        setResizeLimits(800, 520, 1920, 1080);

        // Host the Hub Container
        rootContainer = std::make_unique<HubRootContainer>();
        setContentNonOwned(rootContainer.get(), true);
        centreWithSize(920, 600);
        setVisible(true);
    }

    ~CentralWindow() override
    {
        clearContentComponent();
        rootContainer = nullptr;
    }

    void closeButtonPressed() override
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }

private:
    class HubRootContainer : public juce::Component
    {
    public:
        HubRootContainer()
        {
            mainComponent = std::make_unique<CentralMainComponent>();
            addAndMakeVisible(mainComponent.get());

            splashOverlay = std::make_unique<SplashScreenComponent>();
            addAndMakeVisible(splashOverlay.get());

            splashOverlay->onSplashComplete = [this]
            {
                juce::MessageManager::callAsync([this]
                {
                    if (splashOverlay != nullptr)
                    {
                        splashOverlay->setVisible(false);
                        removeChildComponent(splashOverlay.get());
                    }
                    if (mainComponent != nullptr)
                    {
                        mainComponent->setVisible(true);
                        mainComponent->repaint();
                    }
                });
            };

            setSize(920, 600);
        }

        void resized() override
        {
            if (mainComponent != nullptr)
                mainComponent->setBounds(getLocalBounds());

            if (splashOverlay != nullptr && splashOverlay->isVisible())
                splashOverlay->setBounds(getLocalBounds());
        }

    private:
        std::unique_ptr<CentralMainComponent> mainComponent;
        std::unique_ptr<SplashScreenComponent> splashOverlay;
    };

    std::unique_ptr<HubRootContainer> rootContainer;
};
