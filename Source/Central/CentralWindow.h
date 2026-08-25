#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_core/juce_core.h>
#include "../UI/RackUnitLookAndFeel.h"
#include "../Utils/PluggedINAutoUpdater.h"
#include "../Utils/InstalledRegistry.h"
#include "../Utils/CloudDownloader.h"
#include "../Utils/ManagerSelfUpdater.h"
#include "../Utils/StudioDiagnostics.h"
#include "CentralDesignSystem.h"
#include "PluginDataModel.h"
#include "PluginCardComponent.h"
#include "PluginDetailModal.h"
#include "UpdatesView.h"
#include "StudioToolsView.h"
#include "AuthModal.h"
#include "SplashScreenComponent.h"
#include <vector>
#include <memory>

/**
 * @class CentralMainComponent
 * @brief Commercial-grade Audio Plugin Management Hub with dynamic catalog, view switching, and safe transactions.
 */
class CentralMainComponent : public juce::Component, private juce::Timer
{
public:
    enum class NavSection
    {
        MyProducts,
        AllPlugins,
        Updates,
        StudioTools
    };

    CentralMainComponent()
    {
        setLookAndFeel(&customLookAndFeel);

        // 1. Initialize Product Catalog Metadata
        initProductCatalog();

        // 2. Sidebar Navigation Buttons
        setupNavButton(navMyProducts,  "MY PLUGINS");
        setupNavButton(navAllPlugins,  "ALL PLUGINS");
        setupNavButton(navUpdates,     "UPDATES");
        setupNavButton(navStudioTools, "STUDIO TOOLS");

        auto refreshCloud = [this] {
            juce::String appVer = (juce::JUCEApplication::getInstance() != nullptr) 
                ? juce::JUCEApplication::getInstance()->getApplicationVersion() 
                : autoUpdater.getCurrentVersion();
            autoUpdater.checkForUpdatesAsync(appVer);
        };

        navMyProducts.onClick  = [this, refreshCloud] { currentSection = NavSection::MyProducts;  refreshCloud(); updateActiveViews(); };
        navAllPlugins.onClick  = [this, refreshCloud] { currentSection = NavSection::AllPlugins;  refreshCloud(); updateActiveViews(); };
        navUpdates.onClick     = [this, refreshCloud] { currentSection = NavSection::Updates;     refreshCloud(); updateActiveViews(); };
        navStudioTools.onClick = [this, refreshCloud] { currentSection = NavSection::StudioTools; refreshCloud(); updateActiveViews(); };

        // 3. Search Box with Instant Live Filtering
        addAndMakeVisible(searchInput);
        searchInput.setTextToShowWhenEmpty("Search plugins, instruments & audio tools...", juce::Colour(0xff4a5465));
        searchInput.onTextChange = [this] { filterAndRefreshCards(); };

        // 4. Theme Toggle Button (Dark / Light)
        addAndMakeVisible(themeToggleButton);
        themeToggleButton.setButtonText("THEME: DARK");
        themeToggleButton.onClick = [this]
        {
            isDarkMode = !isDarkMode;
            themeToggleButton.setButtonText(isDarkMode ? "THEME: DARK" : "THEME: LIGHT");
            applyThemeColors();
        };

        // 5. Header Account Button
        addAndMakeVisible(accountButton);
        updateAccountButtonState();
        accountButton.onClick = [this] { openAuthDialog(); };

        // 6. Header Global Status / Update Badge
        addAndMakeVisible(statusActionButton);
        statusActionButton.onClick = [this]
        {
            if (autoUpdater.isUpdateAvailable())
            {
                updateManagerApplication();
            }
            else
            {
                currentSection = NavSection::Updates;
                updateActiveViews();
            }
        };

        // 7. Scrollable Card Viewport Container
        addAndMakeVisible(cardsViewport);
        cardsViewport.setViewedComponent(&cardsListContainer, false);
        cardsViewport.setScrollBarsShown(true, false);
        cardsViewport.setScrollBarThickness(8);

        // 8. Dedicated Sub-Views
        updatesView = std::make_unique<UpdatesViewComponent>(isDarkMode);
        updatesView->onCheckUpdatesClicked = [this, refreshCloud] { refreshCloud(); checkInstallationStatus(); };
        updatesView->onUpdateAllClicked    = [this] { updateAllPendingPlugins(); };
        updatesView->onInstallClicked      = [this](const juce::String& id) { handleInstallPlugin(id); };
        updatesView->onRepairClicked       = [this](const juce::String& id) { handleRepairPlugin(id); };
        updatesView->onUninstallClicked    = [this](const juce::String& id) { handleUninstallPlugin(id); };
        updatesView->onOpenFolderClicked   = [this](const juce::String& id) { handleOpenFolder(id); };
        updatesView->onViewDetailsClicked  = [this](const juce::String& id) { handleViewDetails(id); };
        addChildComponent(updatesView.get());

        studioToolsView = std::make_unique<StudioToolsViewComponent>(isDarkMode);
        addChildComponent(studioToolsView.get());

        // 9. Empty State Button for "My Plugins"
        addChildComponent(browseAllButton);
        browseAllButton.setButtonText("BROWSE UNIVERSAL CATALOG");
        browseAllButton.onClick = [this] { currentSection = NavSection::AllPlugins; updateActiveViews(); };

        // 10. Initial Background Cloud Sync & Self-Healing
        detectedDaws = StudioDiagnostics::scanInstalledDAWs();
        StudioDiagnostics::syncFactoryPresets();

        juce::String appVer = (juce::JUCEApplication::getInstance() != nullptr) 
            ? juce::JUCEApplication::getInstance()->getApplicationVersion() 
            : autoUpdater.getCurrentVersion();
        autoUpdater.checkForUpdatesAsync(appVer);

        juce::Thread::launch([]
        {
            InstalledRegistry::selfHealInstallations("pluggedin_underground");
            InstalledRegistry::selfHealInstallations("pluggedin_plugged1");
            InstalledRegistry::selfHealInstallations("pluggedin_plugtune");
        });

        applyThemeColors();
        checkInstallationStatus();

        setSize(940, 620);
        startTimerHz(10);
    }

    ~CentralMainComponent() override
    {
        stopTimer();
        setLookAndFeel(nullptr);
    }

    void initProductCatalog()
    {
        products.clear();

        // 1. UNDERGROUND
        PluginProduct ug;
        ug.id = "pluggedin_underground";
        ug.name = "UNDERGROUND";
        ug.subtitle = "Boutique Vocal Multi-FX Suite";
        ug.category = "Vocal Multi-FX";
        ug.description = "Flagship Master Vocal Multi-FX Suite featuring 6 Pro DSP Engines, 3-Pole PSOLA Formant Filters, Neve 1073 Iron Transformer Warmth, and 100% Mono-Safe 3D Spatial Aura.";
        ug.dspHighlights = {
            "6 Rebuilt Studio DSP Engines (Formants, Triode Drive, Air Exciter, Micro-Detuner, Dattorro Space, Compressor)",
            "Dynamic 12AX7 SPICE-Modeled Tube Saturation & Dynamic Iron Core",
            "4-Band Dynamic Harshness & Sibilance Resonance Tamer",
            "Zero-Latency Tracking Mode & 7 Master Flagship Presets"
        };
        ug.supportedFormats = { "VST3 64-Bit", "AU (macOS)", "Standalone" };
        ug.latestVersion = "4.2.2";
        ug.changelog = "• Decimated NSDF Pitch Tracking (<0.05% CPU, zero buffer underruns).\n• 64-Sample Lookahead Noise Protection.\n• Synchronized 2-Head Quadrature Overlap-Add Engine (Zero Comb Notches).\n• Series 3-Band Parametric Formant Warper.\n• Organic Voiced-Gated Diplophonia Sub-Harmonic Synthesizer.\n• 16 Flagship Vocal Presets.";
        products.push_back(ug);

        // 2. PLUGGED 1
        PluginProduct p1;
        p1.id = "pluggedin_plugged1";
        p1.name = "PLUGGED 1";
        p1.subtitle = "Flagship Hybrid Synthesizer & 808";
        p1.category = "Instrument";
        p1.description = "Dual-Layer PolyBLEP Multi-Wave Synthesizer and Multi-Sample Instrument with dedicated 808 Pitch Dive Machine and Zero-Delay Feedback Ladder Filter.";
        p1.dspHighlights = {
            "Dual-Layer Synth + Multi-Sample Acoustic & Digital Sound Engine",
            "Dedicated 808 Sub Machine with Pitch Dive & Legato Glide",
            "Zero-Delay Feedback Ladder Filter, Ping-Pong Tape Delay & Reverb",
            "100+ Production Factory Presets across 8 Instrument Categories"
        };
        p1.supportedFormats = { "VST3 64-Bit", "AU (macOS)", "Standalone" };
        p1.latestVersion = "1.0.0";
        p1.changelog = "• Dual-Layer Sampler + PolyBLEP Multi-Wave Synthesizer.\n• Dedicated 808 Machine with Pitch Dive and Legato Glide.\n• Studio ZDF Ladder Filter, Ping-Pong Tape Delay & Reverb.\n• 100+ Production Factory Presets.";
        products.push_back(p1);

        // 3. PLUGTUNE
        PluginProduct pt;
        pt.id = "pluggedin_plugtune";
        pt.name = "PLUGTUNE";
        pt.subtitle = "Real-Time AutoTune & Formant Suite";
        pt.category = "Vocal Multi-FX";
        pt.description = "Studio Vocal Pitch Correction & Formant Manipulation Suite featuring Sub-10ms Live Tracking Engine, Dynamic DAW Delay Compensation (PDC), Schmitt-Trigger Scale Quantization, 12-Tone Scale Heatmap Keyboard, Vocal Doubler, and Live Monophonic Tone Reference Audition.";
        pt.dspHighlights = {
            "Sub-10ms Live Tracking Mode with Automatic FL Studio / DAW Latency Compensation",
            "McLeod Pitch Method (MPM) Pitch Tracker with Schmitt-Trigger Hysteresis Anti-Chatter",
            "Continuous Retune Speed (0ms Hard Snap to 400ms Natural Glide)",
            "Dynamic 12-Tone Scale Heatmap Keyboard & Live Reference Tone Audition",
            "Vocal Doubler with Stereo Width Processor & Neutral Formant Filter"
        };
        pt.supportedFormats = { "VST3 64-Bit", "AU (macOS)", "Standalone" };
        pt.latestVersion = "1.0.0";
        pt.changelog = "• Initial Release (DEV-1059).\n• Sub-10ms Live Recording Monitoring Engine with dynamic PDC host sync.\n• Fast McLeod Pitch Method (MPM) Pitch Tracker with Anti-Chatter hysteresis.\n• 12-Tone Scale Heatmap Keyboard with live note visualization.\n• Integrated Vocal Doubler, Formant Shifter, and Reference Tone Generator.";
        products.push_back(pt);

        // 4. CRUSH (Future Ecosystem Product)
        PluginProduct crush;
        crush.id = "pluggedin_crush";
        crush.name = "CRUSH";
        crush.subtitle = "Analog Tube & Tape Saturation Beast";
        crush.category = "Analog Saturation";
        crush.description = "SPICE-Modeled 12AX7 Triode Preamp, Transformer Iron Core Saturation, and Multi-Stage Dynamic Warmth.";
        crush.dspHighlights = {
            "SPICE-Level Analog Circuit Emulation",
            "Neve Iron Core Transformer Emulation",
            "Dynamic ODF Harmonics Generator",
            "Zero-Latency Tracking Mode"
        };
        crush.supportedFormats = { "VST3 64-Bit", "AU (macOS)" };
        crush.latestVersion = "1.0.0";
        crush.isAvailableInCloud = false;
        crush.changelog = "• Initial Studio Beta Release.";
        products.push_back(crush);
    }

    void applyThemeColors()
    {
        searchInput.setColour(juce::TextEditor::backgroundColourId, CentralDesignSystem::darkBgInput());
        searchInput.setColour(juce::TextEditor::textColourId, CentralDesignSystem::textPrimary(isDarkMode));
        searchInput.setColour(juce::TextEditor::outlineColourId, CentralDesignSystem::borderSubtle(isDarkMode));
        searchInput.setColour(juce::TextEditor::focusedOutlineColourId, CentralDesignSystem::cyan(isDarkMode));

        themeToggleButton.setColour(juce::TextButton::buttonColourId, isDarkMode ? juce::Colour(0xff141822) : juce::Colour(0xffe2e8f0));
        themeToggleButton.setColour(juce::TextButton::textColourOffId, CentralDesignSystem::cyan(isDarkMode));

        browseAllButton.setColour(juce::TextButton::buttonColourId, CentralDesignSystem::cyan(isDarkMode).withAlpha(0.25f));
        browseAllButton.setColour(juce::TextButton::textColourOffId, CentralDesignSystem::cyan(isDarkMode));

        updateNavButtonsTheme();
        updateAccountButtonState();

        if (updatesView != nullptr)
            updatesView->updateTheme(isDarkMode);

        if (studioToolsView != nullptr)
            studioToolsView->updateTheme(isDarkMode);

        filterAndRefreshCards();
        repaint();
    }

    void updateNavButtonsTheme()
    {
        auto styleBtn = [this](juce::TextButton& btn, bool active)
        {
            if (active)
            {
                btn.setColour(juce::TextButton::buttonColourId, isDarkMode ? juce::Colour(0xff182234) : juce::Colour(0xffe0f2fe));
                btn.setColour(juce::TextButton::textColourOffId, CentralDesignSystem::cyan(isDarkMode));
            }
            else
            {
                btn.setColour(juce::TextButton::buttonColourId, isDarkMode ? juce::Colour(0xff10141d) : juce::Colour(0xffe2e8f0));
                btn.setColour(juce::TextButton::textColourOffId, isDarkMode ? CentralDesignSystem::textSecondary(true) : CentralDesignSystem::textSecondary(false));
            }
        };

        styleBtn(navMyProducts,  currentSection == NavSection::MyProducts);
        styleBtn(navAllPlugins,  currentSection == NavSection::AllPlugins);
        styleBtn(navUpdates,     currentSection == NavSection::Updates);
        styleBtn(navStudioTools, currentSection == NavSection::StudioTools);
    }

    void updateAccountButtonState()
    {
        if (AuthManager::isLoggedIn())
        {
            accountButton.setButtonText(AuthManager::getCurrentProducerName() + " | PRO");
            accountButton.setColour(juce::TextButton::buttonColourId, isDarkMode ? juce::Colour(0xff15241b) : juce::Colour(0xffdcfce7));
            accountButton.setColour(juce::TextButton::textColourOffId, CentralDesignSystem::mint(isDarkMode));
        }
        else
        {
            accountButton.setButtonText("SIGN IN / REGISTER");
            accountButton.setColour(juce::TextButton::buttonColourId, isDarkMode ? CentralDesignSystem::cyan(true).withAlpha(0.18f) : juce::Colour(0xffe0f2fe));
            accountButton.setColour(juce::TextButton::textColourOffId, CentralDesignSystem::cyan(isDarkMode));
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
        options.dialogBackgroundColour        = CentralDesignSystem::bgHeader(isDarkMode);
        options.escapeKeyTriggersCloseButton  = true;
        options.useNativeTitleBar             = true;
        options.resizable                     = false;
        options.launchAsync();
    }

    void updateActiveViews()
    {
        updateNavButtonsTheme();

        bool showUpdatesView = (currentSection == NavSection::Updates);
        bool showStudioTools = (currentSection == NavSection::StudioTools);
        bool showCards       = (currentSection == NavSection::MyProducts || currentSection == NavSection::AllPlugins);

        if (updatesView != nullptr)
        {
            updatesView->setVisible(showUpdatesView);
            if (showUpdatesView)
                updatesView->setPlugins(products, isDarkMode);
        }

        if (studioToolsView != nullptr)
            studioToolsView->setVisible(showStudioTools);

        cardsViewport.setVisible(showCards);

        filterAndRefreshCards();
        resized();
        repaint();
    }

    void filterAndRefreshCards()
    {
        juce::String filter = searchInput.getText().trim().toLowerCase();

        displayedProducts.clear();
        for (const auto& p : products)
        {
            if (currentSection == NavSection::MyProducts && !p.isInstalled())
                continue;

            if (filter.isNotEmpty())
            {
                bool matchName = p.name.toLowerCase().contains(filter);
                bool matchSub  = p.subtitle.toLowerCase().contains(filter);
                bool matchCat  = p.category.toLowerCase().contains(filter);
                bool matchDesc = p.description.toLowerCase().contains(filter);
                if (!matchName && !matchSub && !matchCat && !matchDesc)
                    continue;
            }

            displayedProducts.push_back(p);
        }

        // Rebuild card components inside cardsListContainer
        cardsListContainer.cards.clear();
        for (const auto& p : displayedProducts)
        {
            auto card = std::make_unique<PluginCardComponent>(p, isDarkMode);
            card->onInstallClicked     = [this](const juce::String& id) { handleInstallPlugin(id); };
            card->onRepairClicked      = [this](const juce::String& id) { handleRepairPlugin(id); };
            card->onUninstallClicked   = [this](const juce::String& id) { handleUninstallPlugin(id); };
            card->onOpenFolderClicked  = [this](const juce::String& id) { handleOpenFolder(id); };
            card->onViewDetailsClicked = [this](const juce::String& id) { handleViewDetails(id); };

            cardsListContainer.addAndMakeVisible(card.get());
            cardsListContainer.cards.push_back(std::move(card));
        }

        bool showEmptyBrowse = (currentSection == NavSection::MyProducts && displayedProducts.empty() && filter.isEmpty());
        browseAllButton.setVisible(showEmptyBrowse);

        layoutCards();
    }

    void layoutCards()
    {
        int cardW = cardsViewport.getWidth() - 16;
        if (cardW < 200) cardW = getWidth() - 260;
        int cardH = 120;
        int spacing = 12;

        int totalH = static_cast<int>(cardsListContainer.cards.size()) * (cardH + spacing) + 20;
        cardsListContainer.setSize(cardW, juce::jmax(totalH, cardsViewport.getHeight()));

        int y = 8;
        for (auto& card : cardsListContainer.cards)
        {
            card->setBounds(0, y, cardW, cardH);
            y += cardH + spacing;
        }
    }

    void checkInstallationStatus()
    {
        // 1. Ingest any new cloud plugins discovered dynamically from manifest.json
        for (const auto& cloudPlug : autoUpdater.getAllDiscoveredPlugins())
        {
            if (cloudPlug.id.isEmpty()) continue;
            bool exists = false;
            for (const auto& p : products)
            {
                if (p.id == cloudPlug.id)
                {
                    exists = true;
                    break;
                }
            }

            if (!exists)
            {
                PluginProduct newProd;
                newProd.id = cloudPlug.id;
                newProd.name = cloudPlug.name.isNotEmpty() ? cloudPlug.name : cloudPlug.id;
                newProd.subtitle = cloudPlug.subtitle;
                newProd.category = cloudPlug.category.isNotEmpty() ? cloudPlug.category : "Audio Plugin";
                newProd.description = cloudPlug.description;
                newProd.latestVersion = cloudPlug.latestVersion;
                newProd.downloadUrl = cloudPlug.downloadUrl;
                newProd.sha256 = cloudPlug.sha256;
                newProd.changelog = cloudPlug.changelog;
                newProd.supportedFormats = { "VST3 64-Bit", "AU (macOS)" };
                products.push_back(newProd);
            }
        }

        int updatesCount = 0;

        for (auto& p : products)
        {
            p.installedVersion = InstalledRegistry::getInstalledVersion(p.id);
            juce::String cloudVer = autoUpdater.getPluginLatestVersion(p.id);
            if (cloudVer.isNotEmpty()) p.latestVersion = cloudVer;

            p.downloadUrl = autoUpdater.getPluginDownloadUrl(p.id);
            p.sha256      = autoUpdater.getPluginSha256(p.id);
            p.changelog   = autoUpdater.getPluginChangelog(p.id);

            if (cloudDownloader.isBusy() && activeDownloadPluginId == p.id)
            {
                // State handled dynamically by progress callbacks
            }
            else if (p.installedVersion.isNotEmpty())
            {
                if (p.latestVersion.isNotEmpty() && InstalledRegistry::compareVersions(p.installedVersion, p.latestVersion) < 0)
                {
                    p.state = PluginInstallState::UpdateAvailable;
                    updatesCount++;
                }
                else
                {
                    p.state = PluginInstallState::Installed;
                }
            }
            else
            {
                p.state = PluginInstallState::NotInstalled;
            }
        }

        // Update Nav & Header Status Badges
        if (autoUpdater.isUpdateAvailable())
        {
            statusActionButton.setButtonText("CENTRAL APP UPDATE (" + autoUpdater.getLatestVersion() + ")");
            statusActionButton.setColour(juce::TextButton::buttonColourId, CentralDesignSystem::amber(isDarkMode).withAlpha(0.25f));
            statusActionButton.setColour(juce::TextButton::textColourOffId, CentralDesignSystem::amber(isDarkMode));
            navUpdates.setButtonText("UPDATES (1)");
            navUpdates.setColour(juce::TextButton::textColourOffId, CentralDesignSystem::amber(isDarkMode));
        }
        else if (updatesCount > 0)
        {
            statusActionButton.setButtonText(juce::String(updatesCount) + " PLUGIN UPDATE" + (updatesCount > 1 ? "S" : ""));
            statusActionButton.setColour(juce::TextButton::buttonColourId, CentralDesignSystem::amber(isDarkMode).withAlpha(0.25f));
            statusActionButton.setColour(juce::TextButton::textColourOffId, CentralDesignSystem::amber(isDarkMode));
            navUpdates.setButtonText("UPDATES (" + juce::String(updatesCount) + ")");
            navUpdates.setColour(juce::TextButton::textColourOffId, CentralDesignSystem::amber(isDarkMode));
        }
        else
        {
            statusActionButton.setButtonText("STATUS: ALL UP TO DATE");
            statusActionButton.setColour(juce::TextButton::buttonColourId, isDarkMode ? juce::Colour(0xff15241b) : juce::Colour(0xffdcfce7));
            statusActionButton.setColour(juce::TextButton::textColourOffId, CentralDesignSystem::mint(isDarkMode));
            navUpdates.setButtonText("UPDATES");
            navUpdates.setColour(juce::TextButton::textColourOffId, isDarkMode ? CentralDesignSystem::textSecondary(true) : CentralDesignSystem::textSecondary(false));
        }

        // Sync to active cards without rebuilding if busy
        for (auto& card : cardsListContainer.cards)
        {
            for (const auto& p : products)
            {
                if (card->getPluginData().id == p.id)
                {
                    card->updateData(p, isDarkMode);
                    break;
                }
            }
        }

        if (updatesView != nullptr && updatesView->isVisible())
            updatesView->setPlugins(products, isDarkMode);
    }

    void handleInstallPlugin(const juce::String& pluginId)
    {
        if (cloudDownloader.isBusy())
            return;

        PluginProduct* target = nullptr;
        for (auto& p : products)
        {
            if (p.id == pluginId)
            {
                target = &p;
                break;
            }
        }

        if (target == nullptr) return;

        activeDownloadPluginId = pluginId;
        target->state = PluginInstallState::Downloading;
        target->progress = 0.05f;
        target->statusMessage = "CONNECTING...";
        checkInstallationStatus();

        cloudDownloader.onProgressState = [this, pluginId](float fraction, const juce::String& stateLabel)
        {
            for (auto& p : products)
            {
                if (p.id == pluginId)
                {
                    p.progress = fraction;
                    p.statusMessage = stateLabel;
                    break;
                }
            }
            checkInstallationStatus();
        };

        cloudDownloader.onComplete = [this, pluginId](bool success, const juce::String& error)
        {
            activeDownloadPluginId = "";

            if (success)
            {
                juce::Thread::launch([pluginId]
                {
                    InstalledRegistry::selfHealInstallations(pluginId);
                });

                juce::Timer::callAfterDelay(2500, [this]
                {
                    autoUpdater.clearUpdateState();
                    checkInstallationStatus();
                    filterAndRefreshCards();
                });
            }
            else
            {
                for (auto& p : products)
                {
                    if (p.id == pluginId)
                    {
                        p.state = PluginInstallState::Failed;
                        p.errorMessage = error;
                        p.statusMessage = "INSTALL FAILED";
                        break;
                    }
                }

                if (error.isNotEmpty())
                {
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::AlertWindow::WarningIcon,
                        "Installation Notice",
                        error,
                        "OK"
                    );
                }
                checkInstallationStatus();
            }
        };

        juce::String cloudUrl = target->downloadUrl;
        if (cloudUrl.isEmpty()) cloudUrl = autoUpdater.getPluginDownloadUrl(pluginId);
        if (cloudUrl.isEmpty()) cloudUrl = autoUpdater.getDownloadUrl();

        cloudDownloader.startDownloadAsync(cloudUrl, target->id, target->latestVersion, target->sha256);
    }

    void handleRepairPlugin(const juce::String& pluginId)
    {
        handleInstallPlugin(pluginId);
    }

    void handleUninstallPlugin(const juce::String& pluginId)
    {
        juce::String pluginName = "Plugin";
        for (const auto& p : products)
        {
            if (p.id == pluginId)
            {
                pluginName = p.name;
                break;
            }
        }

        juce::AlertWindow::showOkCancelBox(
            juce::AlertWindow::QuestionIcon,
            "Confirm Plugin Uninstall",
            "Are you sure you want to uninstall " + pluginName + "?\n\n" +
            "• Plugin binaries will be safely removed from VST3 directories.\n" +
            "• Your user presets, project states, and DAW sessions will be preserved.",
            "UNINSTALL PLUGIN",
            "CANCEL",
            nullptr,
            juce::ModalCallbackFunction::create([this, pluginId](int result)
            {
                if (result == 1) // OK clicked
                {
                    CloudDownloader::uninstallPlugin(pluginId);
                    checkInstallationStatus();
                    filterAndRefreshCards();
                }
            })
        );
    }

    void handleOpenFolder(const juce::String& pluginId)
    {
        juce::File userVst3 = InstalledRegistry::getUserVst3Directory();
        if (!userVst3.exists()) userVst3.createDirectory();
        userVst3.startAsProcess();
    }

    void handleViewDetails(const juce::String& pluginId)
    {
        for (const auto& p : products)
        {
            if (p.id == pluginId)
            {
                auto* detailComp = new PluginDetailModalComponent(p, isDarkMode);
                detailComp->onPrimaryAction = [this, pluginId, detailComp]
                {
                    handleInstallPlugin(pluginId);
                    if (auto* window = detailComp->findParentComponentOfClass<juce::DialogWindow>())
                        window->exitModalState(1);
                };
                detailComp->onRepairAction = [this, pluginId, detailComp]
                {
                    handleRepairPlugin(pluginId);
                    if (auto* window = detailComp->findParentComponentOfClass<juce::DialogWindow>())
                        window->exitModalState(1);
                };
                detailComp->onUninstallAction = [this, pluginId, detailComp]
                {
                    if (auto* window = detailComp->findParentComponentOfClass<juce::DialogWindow>())
                        window->exitModalState(1);
                    handleUninstallPlugin(pluginId);
                };

                juce::DialogWindow::LaunchOptions options;
                options.content.setOwned(detailComp);
                options.dialogTitle                  = p.name + " — Product Overview";
                options.dialogBackgroundColour       = CentralDesignSystem::bgHeader(isDarkMode);
                options.escapeKeyTriggersCloseButton = true;
                options.useNativeTitleBar            = true;
                options.resizable                    = false;
                options.launchAsync();
                break;
            }
        }
    }

    void updateAllPendingPlugins()
    {
        for (const auto& p : products)
        {
            if (p.needsUpdate())
            {
                handleInstallPlugin(p.id);
                break; // Execute sequentially
            }
        }
    }

    void updateManagerApplication()
    {
        if (managerSelfUpdater.isBusy())
            return;

        statusActionButton.setButtonText("UPDATING CENTRAL...");
        statusActionButton.setColour(juce::TextButton::buttonColourId, CentralDesignSystem::amber(isDarkMode).withAlpha(0.4f));
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
                statusActionButton.setColour(juce::TextButton::textColourOffId, CentralDesignSystem::crimson(isDarkMode));

                if (error.isNotEmpty())
                {
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::AlertWindow::WarningIcon,
                        "Central Update Notice",
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

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        float sidebarWidth = 210.0f;
        float headerHeight = 60.0f;

        // 1. Background
        g.fillAll(CentralDesignSystem::bgMain(isDarkMode));

        // 2. Left Sidebar
        auto sidebarRect = juce::Rectangle<float>(0, 0, sidebarWidth, bounds.getHeight());
        juce::ColourGradient sideGrad(
            CentralDesignSystem::bgSidebar(isDarkMode), 0, 0,
            CentralDesignSystem::bgMain(isDarkMode), sidebarWidth, bounds.getHeight(), false);
        g.setGradientFill(sideGrad);
        g.fillRect(sidebarRect);

        g.setColour(CentralDesignSystem::borderSubtle(isDarkMode));
        g.drawVerticalLine((int)sidebarWidth, 0.0f, bounds.getHeight());

        // Sidebar Branding Logo
        g.setColour(CentralDesignSystem::cyan(isDarkMode));
        g.setFont(juce::Font(18.0f, juce::Font::bold));
        g.drawText("PLUGGEDIN", 20, 18, 170, 22, juce::Justification::left);

        g.setColour(CentralDesignSystem::textMuted(isDarkMode));
        g.setFont(juce::Font(9.5f, juce::Font::bold));
        g.drawText("STUDIO HUB", 20, 38, 170, 14, juce::Justification::left);

        // Sidebar Section Label
        g.setColour(CentralDesignSystem::textMuted(isDarkMode));
        g.setFont(juce::Font(9.0f, juce::Font::bold));
        g.drawText("MANAGE", 20, 78, 170, 14, juce::Justification::left);

        // Sidebar Bottom DAW Status
        float sideBotY = bounds.getHeight() - 100.0f;
        g.setColour(CentralDesignSystem::borderSubtle(isDarkMode));
        g.drawHorizontalLine((int)sideBotY, 15.0f, sidebarWidth - 15.0f);

        g.setColour(CentralDesignSystem::textMuted(isDarkMode));
        g.setFont(juce::Font(9.0f, juce::Font::bold));
        g.drawText("DETECTED HOSTS", 20, (int)sideBotY + 8, 170, 12, juce::Justification::left);

        float pillY = sideBotY + 24.0f;
        int drawn = 0;
        for (const auto& daw : detectedDaws)
        {
            if (daw.isInstalled && drawn < 3)
            {
                g.setColour(CentralDesignSystem::mint(isDarkMode));
                g.setFont(juce::Font(10.0f, juce::Font::plain));
                g.drawText("• " + daw.name, 22, (int)pillY, 160, 14, juce::Justification::left);
                pillY += 16.0f;
                drawn++;
            }
        }
        if (drawn == 0)
        {
            g.setColour(CentralDesignSystem::textDim(isDarkMode));
            g.setFont(juce::Font(9.5f, juce::Font::plain));
            g.drawText("No DAWs Detected", 22, (int)pillY, 160, 14, juce::Justification::left);
        }

        // Sidebar Version Footer
        juce::String appVer = (juce::JUCEApplication::getInstance() != nullptr) 
            ? juce::JUCEApplication::getInstance()->getApplicationVersion() 
            : "2.4.0";
        g.setColour(CentralDesignSystem::textDim(isDarkMode));
        g.setFont(juce::Font(9.0f, juce::Font::plain));
        g.drawText("v" + appVer + " | x64 NATIVE", 20, (int)bounds.getHeight() - 20, 170, 14, juce::Justification::left);

        // 3. Header Bar
        auto headerRect = juce::Rectangle<float>(sidebarWidth, 0, bounds.getWidth() - sidebarWidth, headerHeight);
        g.setColour(CentralDesignSystem::bgHeader(isDarkMode));
        g.fillRect(headerRect);
        g.setColour(CentralDesignSystem::borderSubtle(isDarkMode));
        g.drawHorizontalLine((int)headerHeight, sidebarWidth, bounds.getWidth());

        // 4. Main Section Title (for Card views)
        if (currentSection == NavSection::MyProducts || currentSection == NavSection::AllPlugins)
        {
            float mainX = sidebarWidth + 24.0f;
            float titleY = headerHeight + 14.0f;

            g.setColour(CentralDesignSystem::textPrimary(isDarkMode));
            g.setFont(juce::Font(18.0f, juce::Font::bold));
            juce::String sectionTitle = (currentSection == NavSection::MyProducts) ? "My Installed Plugins" : "Universal Plugin Catalog";
            g.drawText(sectionTitle, (int)mainX, (int)titleY, 300, 24, juce::Justification::left);

            // Empty state message if My Products has 0 items
            if (currentSection == NavSection::MyProducts && displayedProducts.empty() && searchInput.getText().trim().isEmpty())
            {
                auto centerBox = juce::Rectangle<float>(sidebarWidth + 40, headerHeight + 80, bounds.getWidth() - sidebarWidth - 80, 180);
                g.setColour(CentralDesignSystem::bgCard(isDarkMode));
                g.fillRoundedRectangle(centerBox, 8.0f);
                g.setColour(CentralDesignSystem::borderSubtle(isDarkMode));
                g.drawRoundedRectangle(centerBox, 8.0f, 1.0f);

                g.setColour(CentralDesignSystem::textPrimary(isDarkMode));
                g.setFont(juce::Font(16.0f, juce::Font::bold));
                g.drawText("No Plugins Installed Yet", centerBox.getX(), centerBox.getY() + 32, centerBox.getWidth(), 22, juce::Justification::centred);

                g.setColour(CentralDesignSystem::textSecondary(isDarkMode));
                g.setFont(juce::Font(12.0f, juce::Font::plain));
                g.drawText("Browse the universal catalog to install your boutique vocal multi-FX suites and instruments.",
                           centerBox.getX() + 20, centerBox.getY() + 58, centerBox.getWidth() - 40, 20, juce::Justification::centred);
            }
            else if (displayedProducts.empty())
            {
                g.setColour(CentralDesignSystem::textMuted(isDarkMode));
                g.setFont(juce::Font(13.0f, juce::Font::italic));
                g.drawText("No audio plugins found matching '" + searchInput.getText().trim() + "'",
                           (int)mainX, (int)titleY + 50, 400, 20, juce::Justification::left);
            }
        }
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        float sidebarWidth = 210.0f;
        float headerHeight = 60.0f;

        // Sidebar Navigation
        int navY = 100;
        int navHeight = 36;
        int navSpacing = 6;

        navMyProducts.setBounds(12, navY, (int)sidebarWidth - 24, navHeight);
        navY += navHeight + navSpacing;

        navAllPlugins.setBounds(12, navY, (int)sidebarWidth - 24, navHeight);
        navY += navHeight + navSpacing;

        navUpdates.setBounds(12, navY, (int)sidebarWidth - 24, navHeight);
        navY += navHeight + navSpacing;

        navStudioTools.setBounds(12, navY, (int)sidebarWidth - 24, navHeight);

        // Header Controls (Right to Left)
        int rightX = bounds.getWidth() - 16;

        // Account Button
        rightX -= 145;
        accountButton.setBounds(rightX, 12, 140, 36);

        // Theme Toggle
        rightX -= 120;
        themeToggleButton.setBounds(rightX, 12, 110, 36);

        // Status Button
        rightX -= 195;
        statusActionButton.setBounds(rightX, 12, 185, 36);

        // Search Input
        int searchX = (int)sidebarWidth + 20;
        int searchW = juce::jmax(120, rightX - searchX - 16);
        searchInput.setBounds(searchX, 14, searchW, 32);

        // Main Viewport & Sub-views Area
        int mainX = (int)sidebarWidth + 24;
        int mainY = (int)headerHeight + 46;
        int mainW = bounds.getWidth() - mainX - 24;
        int mainH = bounds.getHeight() - mainY - 16;

        cardsViewport.setBounds(mainX, mainY, mainW, mainH);
        layoutCards();

        if (updatesView != nullptr)
            updatesView->setBounds(mainX, (int)headerHeight + 20, mainW, bounds.getHeight() - (int)headerHeight - 36);

        if (studioToolsView != nullptr)
            studioToolsView->setBounds(mainX, (int)headerHeight + 20, mainW, bounds.getHeight() - (int)headerHeight - 36);

        // Empty browse button inside My Products
        if (browseAllButton.isVisible())
        {
            int btnW = 240;
            int btnH = 38;
            browseAllButton.setBounds(mainX + (mainW - btnW) / 2, (int)headerHeight + 190, btnW, btnH);
        }
    }

private:
    void setupNavButton(juce::TextButton& btn, const juce::String& text)
    {
        addAndMakeVisible(btn);
        btn.setButtonText(text);
    }

    void timerCallback() override
    {
        if (++pollCloudCounter >= 150) // 15 seconds
        {
            pollCloudCounter = 0;
            juce::String appVer = (juce::JUCEApplication::getInstance() != nullptr) 
                ? juce::JUCEApplication::getInstance()->getApplicationVersion() 
                : autoUpdater.getCurrentVersion();
            autoUpdater.checkForUpdatesAsync(appVer);
        }

        checkInstallationStatus();
    }

    struct CardsListContainer : public juce::Component
    {
        std::vector<std::unique_ptr<PluginCardComponent>> cards;
    };

    RackUnitLookAndFeel customLookAndFeel;
    PluggedINAutoUpdater autoUpdater;
    CloudDownloader cloudDownloader;
    ManagerSelfUpdater managerSelfUpdater;

    NavSection currentSection { NavSection::MyProducts };
    bool isDarkMode { true };
    int pollCloudCounter { 0 };
    juce::String activeDownloadPluginId { "" };

    juce::TextButton navMyProducts;
    juce::TextButton navAllPlugins;
    juce::TextButton navUpdates;
    juce::TextButton navStudioTools;

    juce::TextEditor searchInput;
    juce::TextButton themeToggleButton;
    juce::TextButton accountButton;
    juce::TextButton statusActionButton;
    juce::TextButton browseAllButton;

    juce::Viewport cardsViewport;
    CardsListContainer cardsListContainer;

    std::unique_ptr<UpdatesViewComponent> updatesView;
    std::unique_ptr<StudioToolsViewComponent> studioToolsView;

    std::vector<PluginProduct> products;
    std::vector<PluginProduct> displayedProducts;
    std::vector<StudioDiagnostics::DAWInfo> detectedDaws;
};

/**
 * @class CentralWindow
 * @brief Resizable commercial host window with splash sequence.
 */
class CentralWindow : public juce::DocumentWindow
{
public:
    CentralWindow(const juce::String& name)
        : juce::DocumentWindow(name,
                               CentralDesignSystem::darkBgMain(),
                               juce::DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar(true);
        setResizable(true, true);
        setResizeLimits(820, 540, 1920, 1080);

        rootContainer = std::make_unique<HubRootContainer>();
        setContentNonOwned(rootContainer.get(), true);
        centreWithSize(940, 620);
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

            setSize(940, 620);
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
