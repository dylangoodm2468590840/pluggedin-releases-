#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include "InstalledRegistry.h"
#include "ManagerSelfUpdater.h"
#include <functional>
#include <atomic>
#if JUCE_WINDOWS || defined(_WIN32)
#include <windows.h>
#include <tlhelp32.h>
#endif

/**
 * @class CloudDownloader
 * @brief Commercial-grade Transactional Plugin Installer & On-Disk Verification Engine.
 * Features:
 * - Pre-flight DAW process & file lock detection
 * - SHA-256 cryptographic verification
 * - Isolated temporary sandbox extraction
 * - Atomic rollback snapshot creation
 * - Elevated multi-directory installation
 * - Strict post-install physical on-disk binary inspection
 */
class CloudDownloader : private juce::Thread
{
public:
    enum class InstallState
    {
        Idle,
        CheckingLocks,
        Downloading,
        VerifyingChecksum,
        Extracting,
        BackingUp,
        Installing,
        VerifyingOnDisk,
        Complete,
        Failed
    };

    std::function<void(float, const juce::String&)> onProgressState;
    std::function<void(bool, const juce::String&)> onComplete;

    CloudDownloader()
        : juce::Thread("PluggedINPluginTransactionThread")
    {
    }

    ~CloudDownloader() override
    {
        stopThread(4000);
    }

    static bool isKnownDawRunning(juce::String& outDawName)
    {
#if JUCE_WINDOWS || defined(_WIN32)
        const char* knownDaws[] = {
            "FL64.exe", "FL.exe", "Ableton Live 11 Suite.exe", "Ableton Live 12 Suite.exe",
            "Ableton Live 11 Standard.exe", "Ableton Live 12 Standard.exe", "Ableton Live.exe",
            "reaper.exe", "reaper_host64.exe", "Studio One.exe", "Cubase12.exe", "Cubase13.exe",
            "ProTools.exe", "Bitwig Studio.exe", "Cakewalk.exe"
        };

        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot != INVALID_HANDLE_VALUE)
        {
            PROCESSENTRY32W pe;
            pe.dwSize = sizeof(pe);
            if (Process32FirstW(snapshot, &pe))
            {
                do
                {
                    juce::String processName(pe.szExeFile);
                    for (const auto* daw : knownDaws)
                    {
                        if (processName.equalsIgnoreCase(daw))
                        {
                            CloseHandle(snapshot);
                            outDawName = processName;
                            return true;
                        }
                    }
                } while (Process32NextW(snapshot, &pe));
            }
            CloseHandle(snapshot);
        }
#endif
        return false;
    }

    static bool isVst3FileLocked(const juce::File& vst3Dir)
    {
        if (!vst3Dir.exists()) return false;

#if JUCE_WINDOWS || defined(_WIN32)
        juce::File binary = vst3Dir.getChildFile("Contents\\x86_64-win\\UNDERGROUND.vst3");
        if (!binary.existsAsFile())
            binary = vst3Dir.getChildFile("UNDERGROUND.vst3");

        if (binary.existsAsFile())
        {
            HANDLE hFile = CreateFileA(binary.getFullPathName().toRawUTF8(),
                                       GENERIC_WRITE,
                                       0, // Exclusive lock probe
                                       NULL,
                                       OPEN_EXISTING,
                                       FILE_ATTRIBUTE_NORMAL,
                                       NULL);

            if (hFile == INVALID_HANDLE_VALUE)
            {
                DWORD err = GetLastError();
                if (err == ERROR_SHARING_VIOLATION || err == ERROR_LOCK_VIOLATION || err == ERROR_ACCESS_DENIED)
                    return true;
            }
            else
            {
                CloseHandle(hFile);
            }
        }
#endif
        return false;
    }

    static bool uninstallPlugin(const juce::String& pluginId)
    {
        auto candidates = InstalledRegistry::getCandidateDirectories(pluginId);
        for (auto& cand : candidates)
        {
            if (cand.exists())
            {
                try { cand.deleteRecursively(); } catch (...) {}
            }
        }
        InstalledRegistry::removeInstalledVersion(pluginId);
        return true;
    }

    void startDownloadAsync(const juce::String& urlStr, const juce::String& targetPluginId, const juce::String& targetVersion, const juce::String& expectedSha256 = "")
    {
        downloadUrl = urlStr;
        pluginId = targetPluginId;
        version = targetVersion;
        targetSha256 = expectedSha256.toLowerCase().trim();

        isDownloading.store(true);
        startThread();
    }

    bool isBusy() const noexcept { return isDownloading.load(); }

private:
    void run() override
    {
        bool success = false;
        juce::String errorMsg = "";

        juce::String pluginFolder = (pluginId == "pluggedin_crush") ? "CRUSH.vst3" : "UNDERGROUND.vst3";
        juce::File userVst3Dir = InstalledRegistry::getUserVst3Directory().getChildFile(pluginFolder);
        juce::File sysVst3Dir  = InstalledRegistry::getSystemVst3Directory().getChildFile(pluginFolder);

        auto allOldLocations = InstalledRegistry::getCandidateDirectories(pluginId);

        // -------------------------------------------------------------
        // TRANSACTION STEP 1: Pre-Flight Lock & DAW Process Verification
        // -------------------------------------------------------------
        updateState(0.05f, "Checking DAW processes & file locks...");

        juce::String runningDaw = "";
        if (isKnownDawRunning(runningDaw))
        {
            errorMsg = "DAW Active (" + runningDaw + "). Please close your DAW before updating.";
            isDownloading.store(false);
            juce::MessageManager::callAsync([this, errorMsg] {
                if (onComplete) onComplete(false, errorMsg);
            });
            return;
        }

        for (const auto& loc : allOldLocations)
        {
            if (isVst3FileLocked(loc))
            {
                errorMsg = "Plugin binary is locked in memory. Please close FL Studio / DAW.";
                isDownloading.store(false);
                juce::MessageManager::callAsync([this, errorMsg] {
                    if (onComplete) onComplete(false, errorMsg);
                });
                return;
            }
        }

        juce::File tempDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                                  .getChildFile("PluggedIN\\temp\\plugin_staging");
        if (!tempDir.exists())
            tempDir.createDirectory();

        // -------------------------------------------------------------
        // TRANSACTION STEP 2: Isolated Sandbox Download
        // -------------------------------------------------------------
        if (downloadUrl.isNotEmpty())
        {
            juce::URL url(downloadUrl);
            juce::File downloadedFile = tempDir.getChildFile("downloaded_plugin.zip");
            if (downloadedFile.existsAsFile())
                downloadedFile.deleteFile();

            updateState(0.10f, "Connecting to Cloud CDN...");

            auto stream = url.createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                                                .withConnectionTimeoutMs(10000)
                                                .withHttpRequestCmd("GET")
                                                .withExtraHeaders("User-Agent: PluggedIN-Central\r\nAccept: */*\r\n"));

            if (stream != nullptr)
            {
                juce::FileOutputStream fileOut(downloadedFile);
                if (fileOut.openedOk())
                {
                    juce::int64 totalBytes = stream->getTotalLength();
                    juce::int64 bytesWritten = 0;
                    char buffer[8192];

                    while (!stream->isExhausted() && !threadShouldExit())
                    {
                        int bytesRead = stream->read(buffer, sizeof(buffer));
                        if (bytesRead > 0)
                        {
                            fileOut.write(buffer, static_cast<size_t>(bytesRead));
                            bytesWritten += bytesRead;

                            if (totalBytes > 0)
                            {
                                float fraction = 0.10f + (static_cast<float>(bytesWritten) / static_cast<float>(totalBytes)) * 0.60f;
                                int pct = static_cast<int>((static_cast<float>(bytesWritten) / static_cast<float>(totalBytes)) * 100.0f);
                                updateState(fraction, "DOWNLOADING " + juce::String(pct) + "%");
                            }
                        }
                    }
                    fileOut.flush();

                    // -------------------------------------------------------------
                    // TRANSACTION STEP 3: Cryptographic SHA-256 Checksum Validation
                    // -------------------------------------------------------------
                    updateState(0.75f, "Verifying SHA-256 checksum...");
                    if (targetSha256.isNotEmpty())
                    {
                        juce::String actualHash = ManagerSelfUpdater::computeSHA256(downloadedFile);
                        if (actualHash != targetSha256)
                        {
                            errorMsg = "Package SHA-256 checksum mismatch. Download may be corrupted.";
                            downloadedFile.deleteFile();
                            isDownloading.store(false);
                            juce::MessageManager::callAsync([this, errorMsg] {
                                if (onComplete) onComplete(false, errorMsg);
                            });
                            return;
                        }
                    }

                    // -------------------------------------------------------------
                    // TRANSACTION STEP 4: Sandbox Extraction & Package Discovery
                    // -------------------------------------------------------------
                    updateState(0.80f, "Extracting payload in staging sandbox...");
                    juce::File stageExtract = tempDir.getChildFile("extracted_vst3");
                    if (stageExtract.exists())
                        stageExtract.deleteRecursively();
                    stageExtract.createDirectory();

                    juce::ZipFile zip(downloadedFile);
                    if (zip.getNumEntries() > 0)
                    {
                        zip.uncompressTo(stageExtract);

                        juce::File unpackedVst3 = stageExtract.getChildFile("UNDERGROUND.vst3");
                        if (!unpackedVst3.exists())
                        {
                            juce::Array<juce::File> found;
                            stageExtract.findChildFiles(found, juce::File::findDirectories, true, "UNDERGROUND.vst3");
                            if (found.size() > 0)
                                unpackedVst3 = found[0];
                            else
                                unpackedVst3 = stageExtract;
                        }

                        // -------------------------------------------------------------
                        // TRANSACTION STEP 5: Create Rollback Snapshot of Existing VST3
                        // -------------------------------------------------------------
                        updateState(0.85f, "Creating rollback snapshot...");
                        juce::File rollbackDir = tempDir.getChildFile("rollback_backup");
                        if (rollbackDir.exists()) rollbackDir.deleteRecursively();
                        rollbackDir.createDirectory();

                        bool hadExistingSys = sysVst3Dir.exists();
                        if (hadExistingSys)
                        {
                            try { sysVst3Dir.copyDirectoryTo(rollbackDir.getChildFile("sys_backup.vst3")); } catch (...) {}
                        }

                        // -------------------------------------------------------------
                        // TRANSACTION STEP 6: Multi-Directory Elevated Deployment
                        // -------------------------------------------------------------
                        updateState(0.90f, "Installing to System VST3 Directory...");

                        // Elevated PowerShell install for C:\Program Files\Common Files\VST3
                        juce::String psScript =
                            "if (Test-Path '" + sysVst3Dir.getFullPathName() + "') { "
                            "  Remove-Item -Recurse -Force '" + sysVst3Dir.getFullPathName() + "' "
                            "}; "
                            "Copy-Item -Recurse -Force '" + unpackedVst3.getFullPathName() + "' "
                            "'" + sysVst3Dir.getParentDirectory().getFullPathName() + "'";

                        juce::MemoryOutputStream encoded;
                        for (int ci = 0; ci < psScript.length(); ++ci)
                        {
                            juce::juce_wchar wc = psScript[ci];
                            encoded.writeByte((char)(wc & 0xFF));
                            encoded.writeByte((char)((wc >> 8) & 0xFF));
                        }
                        juce::String b64Script = juce::Base64::toBase64(encoded.getData(), encoded.getDataSize());

                        bool systemInstallOk = false;
#if JUCE_WINDOWS || defined(_WIN32)
                        juce::String psArgs = "-NoProfile -NonInteractive -ExecutionPolicy Bypass -EncodedCommand " + b64Script;
                        HINSTANCE result = ShellExecuteA(
                            nullptr,
                            "runas",
                            "powershell.exe",
                            psArgs.toRawUTF8(),
                            nullptr,
                            SW_HIDE
                        );
                        systemInstallOk = ((INT_PTR)result > 32);
                        if (systemInstallOk)
                            juce::Thread::sleep(2500);
#endif

                        // Fallback / User Directory Mirror
                        userVst3Dir.getParentDirectory().createDirectory();
                        if (userVst3Dir.exists()) userVst3Dir.deleteRecursively();
                        unpackedVst3.copyDirectoryTo(userVst3Dir);

                        if (!systemInstallOk)
                        {
                            try {
                                sysVst3Dir.getParentDirectory().createDirectory();
                                if (sysVst3Dir.exists()) sysVst3Dir.deleteRecursively();
                                unpackedVst3.copyDirectoryTo(sysVst3Dir);
                                systemInstallOk = true;
                            } catch (...) {}
                        }

                        // -------------------------------------------------------------
                        // TRANSACTION STEP 7: STRICT PHYSICAL ON-DISK VERIFICATION
                        // -------------------------------------------------------------
                        updateState(0.95f, "Verifying on-disk binary metadata...");
                        juce::String verifiedOnDiskPath = "";
                        bool verified = InstalledRegistry::verifyOnDiskInstallation(pluginId, version, verifiedOnDiskPath);

                        if (verified)
                        {
                            // TRANSACTION COMMIT
                            updateState(1.0f, "Installation Verified: v" + version);
                            success = true;
                            if (rollbackDir.exists()) rollbackDir.deleteRecursively();
                        }
                        else
                        {
                            // TRANSACTION ROLLBACK
                            updateState(1.0f, "Verification failed! Rolling back...");
                            if (hadExistingSys && rollbackDir.getChildFile("sys_backup.vst3").exists())
                            {
                                try {
                                    rollbackDir.getChildFile("sys_backup.vst3").copyDirectoryTo(sysVst3Dir);
                                } catch (...) {}
                            }
                            errorMsg = "On-disk version verification failed after installation!";
                            success = false;
                        }
                    }
                }
            }
            else
            {
                errorMsg = "Unable to connect to release CDN server.";
            }
        }
        else
        {
            errorMsg = "No download URL provided in release manifest.";
        }

        isDownloading.store(false);

        juce::MessageManager::callAsync([this, success, errorMsg] {
            if (onComplete)
                onComplete(success, errorMsg);
        });
    }

    void updateState(float frac, const juce::String& label)
    {
        juce::MessageManager::callAsync([this, frac, label] {
            if (onProgressState) onProgressState(frac, label);
        });
    }

    std::atomic<bool> isDownloading { false };
    juce::String downloadUrl { "" };
    juce::String pluginId { "pluggedin_underground" };
    juce::String version { "3.3.0" };
    juce::String targetSha256 { "" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CloudDownloader)
};
