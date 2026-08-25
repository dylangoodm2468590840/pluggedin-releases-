#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "CentralDesignSystem.h"
#include "../Utils/StudioDiagnostics.h"
#include "../Utils/InstalledRegistry.h"
#include <vector>

/**
 * @class StudioToolsViewComponent
 * @brief Dedicated Studio Tools, DAW Scanner, Audio Engine Diagnostics, and Tester Support Hub.
 */
class StudioToolsViewComponent : public juce::Component
{
public:
    StudioToolsViewComponent(bool isDarkMode = true)
        : darkMode(isDarkMode)
    {
        // 1. Rescan DAWs Button
        addAndMakeVisible(rescanDawsButton);
        rescanDawsButton.setButtonText("RESCAN DAWS");
        rescanDawsButton.onClick = [this] { rescanDaws(); };

        // 2. Run Diagnostics Button
        addAndMakeVisible(runDiagnosticsButton);
        runDiagnosticsButton.setButtonText("RUN DSP DIAGNOSTIC AUDIT");
        runDiagnosticsButton.onClick = [this] { runDspAudit(); };

        // 2b. Sync to FL Studio / Fix Mac Permissions
        addAndMakeVisible(syncFlStudioButton);
        syncFlStudioButton.setButtonText("⚡ SYNC TO FL STUDIO / LOGIC (FIX PERMISSIONS)");
        syncFlStudioButton.onClick = [this]
        {
            juce::String report = StudioDiagnostics::repairMacPermissionsAndSync();
            diagOutput.setText(report);
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::InfoIcon,
                "FL Studio & DAW Synchronization",
                "Permissions and folder paths verified!\n\n"
                "In FL Studio (Mac):\n"
                "1. Options -> Manage plugins\n"
                "2. Turn ON 'Rescan previously verified plugins' & 'Rescan plugins with errors'\n"
                "3. Click 'Find installed plugins'\n\n"
                "• UNDERGROUND -> In Mixer FX slots\n"
                "• PLUGGED 1 -> In Channel Rack (+)",
                "GOT IT"
            );
        };

        // 3. Open System VST3 Folder
        addAndMakeVisible(openSysVst3Button);
        openSysVst3Button.setButtonText("OPEN SYSTEM VST3");
        openSysVst3Button.onClick = [this]
        {
            juce::File dir = InstalledRegistry::getSystemVst3Directory();
            if (!dir.exists()) dir.createDirectory();
            dir.startAsProcess();
        };

        // 4. Open User VST3 Folder
        addAndMakeVisible(openUserVst3Button);
        openUserVst3Button.setButtonText("OPEN USER VST3");
        openUserVst3Button.onClick = [this]
        {
            juce::File dir = InstalledRegistry::getUserVst3Directory();
            if (!dir.exists()) dir.createDirectory();
            dir.startAsProcess();
        };

        // 5. Open Presets Folder
        addAndMakeVisible(openPresetsButton);
        openPresetsButton.setButtonText("OPEN PRESETS");
        openPresetsButton.onClick = [this]
        {
            juce::File dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                                 .getChildFile("PluggedIN\\Presets");
            if (!dir.exists()) dir.createDirectory();
            dir.startAsProcess();
        };

        // 6. Copy Tester Diagnostics
        addAndMakeVisible(copyDiagnosticsButton);
        copyDiagnosticsButton.setButtonText("COPY TESTER DIAGNOSTICS REPORT");
        copyDiagnosticsButton.onClick = [this] { copyDiagnosticsReport(); };

        // 7. Clear Cache Button
        addAndMakeVisible(clearCacheButton);
        clearCacheButton.setButtonText("CLEAR DOWNLOAD CACHE");
        clearCacheButton.onClick = [this] { clearTempCache(); };

        // Output TextEditor
        addAndMakeVisible(diagOutput);
        diagOutput.setMultiLine(true);
        diagOutput.setReadOnly(true);
        diagOutput.setCaretVisible(false);
        diagOutput.setScrollbarsShown(true);
        diagOutput.setText("PluggedIN Audio Diagnostics Ready.\nClick 'RUN DSP DIAGNOSTIC AUDIT' to perform real-time buffer, denormal, and binary checks.");

        detectedDaws = StudioDiagnostics::scanInstalledDAWs();
        updateTheme(isDarkMode);
    }

    void updateTheme(bool isDark)
    {
        darkMode = isDark;

        rescanDawsButton.setColour(juce::TextButton::buttonColourId, darkMode ? juce::Colour(0xff161c28) : juce::Colour(0xffe2e8f0));
        rescanDawsButton.setColour(juce::TextButton::textColourOffId, CentralDesignSystem::cyan(darkMode));

        runDiagnosticsButton.setColour(juce::TextButton::buttonColourId, CentralDesignSystem::mint(darkMode).withAlpha(0.25f));
        runDiagnosticsButton.setColour(juce::TextButton::textColourOffId, CentralDesignSystem::mint(darkMode));

        syncFlStudioButton.setColour(juce::TextButton::buttonColourId, CentralDesignSystem::cyan(darkMode).withAlpha(0.25f));
        syncFlStudioButton.setColour(juce::TextButton::textColourOffId, CentralDesignSystem::cyan(darkMode));

        openSysVst3Button.setColour(juce::TextButton::buttonColourId, darkMode ? juce::Colour(0xff161c28) : juce::Colour(0xffe2e8f0));
        openSysVst3Button.setColour(juce::TextButton::textColourOffId, CentralDesignSystem::textSecondary(darkMode));

        openUserVst3Button.setColour(juce::TextButton::buttonColourId, darkMode ? juce::Colour(0xff161c28) : juce::Colour(0xffe2e8f0));
        openUserVst3Button.setColour(juce::TextButton::textColourOffId, CentralDesignSystem::textSecondary(darkMode));

        openPresetsButton.setColour(juce::TextButton::buttonColourId, darkMode ? juce::Colour(0xff161c28) : juce::Colour(0xffe2e8f0));
        openPresetsButton.setColour(juce::TextButton::textColourOffId, CentralDesignSystem::textSecondary(darkMode));

        copyDiagnosticsButton.setColour(juce::TextButton::buttonColourId, CentralDesignSystem::cyan(darkMode).withAlpha(0.2f));
        copyDiagnosticsButton.setColour(juce::TextButton::textColourOffId, CentralDesignSystem::cyan(darkMode));

        clearCacheButton.setColour(juce::TextButton::buttonColourId, darkMode ? juce::Colour(0xff221419) : juce::Colour(0xfffee2e2));
        clearCacheButton.setColour(juce::TextButton::textColourOffId, CentralDesignSystem::crimson(darkMode));

        diagOutput.setColour(juce::TextEditor::backgroundColourId, darkMode ? juce::Colour(0xff0b0e14) : juce::Colour(0xfff8fafc));
        diagOutput.setColour(juce::TextEditor::textColourId, darkMode ? juce::Colour(0xff00ff88) : juce::Colour(0xff16a34a));
        diagOutput.setColour(juce::TextEditor::outlineColourId, CentralDesignSystem::borderSubtle(darkMode));

        repaint();
    }

    void rescanDaws()
    {
        detectedDaws = StudioDiagnostics::scanInstalledDAWs();
        StudioDiagnostics::syncFactoryPresets();

        juce::String report = "DAW Scan Complete (" + juce::Time::getCurrentTime().formatted("%H:%M:%S") + "):\n";
        int installedCount = 0;
        for (const auto& daw : detectedDaws)
        {
            if (daw.isInstalled)
            {
                installedCount++;
                report += "  [✓] " + daw.name + " (" + daw.version + ") -> " + daw.installPath + "\n";
            }
        }
        if (installedCount == 0) report += "  [!] No standard DAW Program Files installations detected.\n";

        diagOutput.setText(report);
        repaint();
    }

    void runDspAudit()
    {
        juce::String report;
        bool passed = StudioDiagnostics::runAudioEngineDiagnostics(report);

        juce::String out = "=== PLUGGEDIN DSP & AUDIO ENGINE AUDIT ===\n";
        out += "Timestamp: " + juce::Time::getCurrentTime().toISO8601(true) + "\n";
        out += "OS: " + juce::SystemStats::getOperatingSystemName() + "\n";
        out += "CPU: " + juce::SystemStats::getCpuVendor() + " (" + juce::String(juce::SystemStats::getNumCpus()) + " Cores)\n";
        out += "Result: " + report + "\n";
        out += passed ? "STATUS: 100% OPERATIONAL & VERIFIED.\n" : "STATUS: AUDIT WARNINGS DETECTED.\n";

        diagOutput.setText(out);
    }

    void copyDiagnosticsReport()
    {
        juce::String report = "=== PLUGGEDIN BETA TESTER DIAGNOSTIC REPORT ===\n";
        report += "Central App Version: 2.3.0\n";
        report += "OS: " + juce::SystemStats::getOperatingSystemName() + " (" + (juce::SystemStats::isOperatingSystem64Bit() ? "64-bit" : "32-bit") + ")\n";
        report += "CPU: " + juce::SystemStats::getCpuVendor() + " | Cores: " + juce::String(juce::SystemStats::getNumCpus()) + "\n";
        report += "System VST3 Dir: " + InstalledRegistry::getSystemVst3Directory().getFullPathName() + "\n";
        report += "User VST3 Dir: " + InstalledRegistry::getUserVst3Directory().getFullPathName() + "\n";
        report += "UNDERGROUND Version: " + InstalledRegistry::getInstalledVersion("pluggedin_underground") + "\n";
        report += "PLUGGED 1 Version:    " + InstalledRegistry::getInstalledVersion("pluggedin_plugged1") + "\n";
        report += "PLUGTUNE Version:     " + InstalledRegistry::getInstalledVersion("pluggedin_plugtune") + "\n";
        report += "\nInstalled DAWs:\n";
        for (const auto& daw : detectedDaws)
        {
            if (daw.isInstalled)
                report += "  - " + daw.name + " (" + daw.version + "): " + daw.installPath + "\n";
        }

        juce::SystemClipboard::copyTextToClipboard(report);
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
            "Diagnostics Copied",
            "System & Plugin Diagnostics Report has been copied to your clipboard!\nPaste it directly into your bug report or email to the team.",
            "OK");
    }

    void clearTempCache()
    {
        juce::File tempDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                                  .getChildFile("PluggedIN\\temp");
        if (tempDir.exists())
            tempDir.deleteRecursively();

        diagOutput.setText("Temporary download and staging cache cleared successfully.");
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // 1. DAW Environments Section
        auto dawCard = juce::Rectangle<float>(0, 0, bounds.getWidth(), 180);
        g.setColour(CentralDesignSystem::bgCard(darkMode));
        g.fillRoundedRectangle(dawCard, 8.0f);
        g.setColour(CentralDesignSystem::borderSubtle(darkMode));
        g.drawRoundedRectangle(dawCard, 8.0f, 1.0f);

        g.setColour(CentralDesignSystem::textPrimary(darkMode));
        g.setFont(juce::Font(15.0f, juce::Font::bold));
        g.drawText("DETECTED STUDIO DAWS & HOSTS", 20, 14, bounds.getWidth() - 200, 22, juce::Justification::left);

        float dawY = 46.0f;
        float colW = (bounds.getWidth() - 40.0f) / 2.0f;
        int idx = 0;

        for (const auto& daw : detectedDaws)
        {
            float x = 20.0f + (idx % 2) * colW;
            float y = dawY + (idx / 2) * 22.0f;

            if (y < dawCard.getBottom() - 10.0f)
            {
                juce::Colour dotCol = daw.isInstalled ? CentralDesignSystem::mint(darkMode) : CentralDesignSystem::textDim(darkMode);
                g.setColour(dotCol);
                g.fillEllipse(x, y + 4.0f, 8.0f, 8.0f);

                g.setColour(daw.isInstalled ? CentralDesignSystem::textPrimary(darkMode) : CentralDesignSystem::textMuted(darkMode));
                g.setFont(juce::Font(11.5f, daw.isInstalled ? juce::Font::bold : juce::Font::plain));
                g.drawText(daw.name + (daw.isInstalled ? " (" + daw.version + ")" : " -- Not Installed"),
                           (int)x + 16, (int)y, (int)colW - 24, 18, juce::Justification::left);
            }
            idx++;
        }

        // 2. Directory Management & Tester Tools Section
        float toolsY = 194.0f;
        auto toolsCard = juce::Rectangle<float>(0, toolsY, bounds.getWidth(), 110);
        g.setColour(CentralDesignSystem::bgCard(darkMode));
        g.fillRoundedRectangle(toolsCard, 8.0f);
        g.setColour(CentralDesignSystem::borderSubtle(darkMode));
        g.drawRoundedRectangle(toolsCard, 8.0f, 1.0f);

        g.setColour(CentralDesignSystem::textPrimary(darkMode));
        g.setFont(juce::Font(15.0f, juce::Font::bold));
        g.drawText("PLUGIN DIRECTORIES & TESTER TOOLS", 20, (int)toolsY + 14, bounds.getWidth() - 40, 22, juce::Justification::left);

        // 3. Audio Engine Diagnostics Header
        float diagY = 316.0f;
        g.setColour(CentralDesignSystem::textPrimary(darkMode));
        g.setFont(juce::Font(15.0f, juce::Font::bold));
        g.drawText("AUDIO ENGINE & DSP DIAGNOSTIC TERMINAL", 0, (int)diagY, bounds.getWidth() - 250, 22, juce::Justification::left);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        // Rescan button in top right of DAW card
        rescanDawsButton.setBounds(bounds.getWidth() - 150, 10, 135, 30);

        // Directory buttons in Tools Card
        int toolsY = 194 + 48;
        int btnW = (bounds.getWidth() - 48) / 3;
        openSysVst3Button.setBounds(16, toolsY, btnW, 32);
        openUserVst3Button.setBounds(16 + btnW + 8, toolsY, btnW, 32);
        openPresetsButton.setBounds(16 + (btnW + 8) * 2, toolsY, btnW, 32);

        // Diagnostics action buttons
        int diagY = 312;
        int btnAuditW = 210;
        int btnSyncW = 340;
        runDiagnosticsButton.setBounds(bounds.getWidth() - btnAuditW - btnSyncW - 10, diagY, btnAuditW, 32);
        syncFlStudioButton.setBounds(bounds.getWidth() - btnSyncW, diagY, btnSyncW, 32);

        // Output editor
        int outY = 352;
        int outH = bounds.getHeight() - outY - 54;
        diagOutput.setBounds(0, outY, bounds.getWidth(), juce::jmax(80, outH));

        // Bottom Tester Buttons
        int botY = bounds.getHeight() - 42;
        copyDiagnosticsButton.setBounds(0, botY, 270, 34);
        clearCacheButton.setBounds(bounds.getWidth() - 200, botY, 200, 34);
    }

private:
    bool darkMode { true };
    std::vector<StudioDiagnostics::DAWInfo> detectedDaws;

    juce::TextButton rescanDawsButton;
    juce::TextButton runDiagnosticsButton;
    juce::TextButton syncFlStudioButton;
    juce::TextButton openSysVst3Button;
    juce::TextButton openUserVst3Button;
    juce::TextButton openPresetsButton;
    juce::TextButton copyDiagnosticsButton;
    juce::TextButton clearCacheButton;

    juce::TextEditor diagOutput;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StudioToolsViewComponent)
};
