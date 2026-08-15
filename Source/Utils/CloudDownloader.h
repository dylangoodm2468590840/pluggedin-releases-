#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include "InstalledRegistry.h"
#include "ManagerSelfUpdater.h"
#include <functional>
#include <atomic>
#include <windows.h>

/**
 * @class CloudDownloader
 * @brief Thread-safe background cloud binary downloader & hot-swap installer for PluggedIN plugins.
 * Equipped with DAW file-lock detection, SHA-256 verification, and atomic staging.
 */
class CloudDownloader : private juce::Thread
{
public:
    std::function<void(float)> onProgress;
    std::function<void(bool, const juce::String&)> onComplete;

    CloudDownloader()
        : juce::Thread("PluggedINCloudDownloaderThread")
    {
    }

    ~CloudDownloader() override
    {
        stopThread(3000);
    }

    static bool isVst3FileLocked(const juce::File& vst3Dir)
    {
        if (!vst3Dir.exists()) return false;
        juce::File binary = vst3Dir.getChildFile("Contents\\x86_64-win\\UNDERGROUND.vst3");
        if (!binary.existsAsFile()) return false;

        HANDLE hFile = CreateFileA(binary.getFullPathName().toRawUTF8(),
                                   GENERIC_WRITE,
                                   0, // Exclusive access test
                                   NULL,
                                   OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL,
                                   NULL);

        if (hFile == INVALID_HANDLE_VALUE)
        {
            DWORD err = GetLastError();
            if (err == ERROR_SHARING_VIOLATION || err == ERROR_LOCK_VIOLATION)
                return true;
        }
        else
        {
            CloseHandle(hFile);
        }

        return false;
    }

    static bool uninstallPlugin(const juce::String& pluginId)
    {
        juce::File targetVst3Dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                                      .getChildFile("Local\\Programs\\Common\\VST3\\UNDERGROUND.vst3");
        if (targetVst3Dir.exists())
            targetVst3Dir.deleteRecursively();

        juce::File sysTarget("C:\\Program Files\\Common Files\\VST3\\UNDERGROUND.vst3");
        if (sysTarget.exists())
            sysTarget.deleteRecursively();

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

        juce::File targetVst3Dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                                      .getChildFile("Local\\Programs\\Common\\VST3\\UNDERGROUND.vst3");

        // 1. Proactive DAW File-Lock Detection
        if (isVst3FileLocked(targetVst3Dir))
        {
            errorMsg = "UNDERGROUND.vst3 is currently in use! Please close FL Studio / your DAW before updating.";
            isDownloading.store(false);
            juce::MessageManager::callAsync([this, errorMsg] {
                if (onComplete) onComplete(false, errorMsg);
            });
            return;
        }

        juce::File tempDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                                  .getChildFile("PluggedIN\\temp\\plugin_staging");
        juce::String pluginFolder = (pluginId == "pluggedin_crush") ? "CRUSH.vst3" : "UNDERGROUND.vst3";
        juce::String macAuFolder   = (pluginId == "pluggedin_crush") ? "CRUSH.component" : "UNDERGROUND.component";

#if JUCE_MAC
        juce::File userVst3Dir = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                                    .getChildFile("Library/Audio/Plug-Ins/VST3").getChildFile(pluginFolder);
        juce::File sysVst3Dir("/Library/Audio/Plug-Ins/VST3/" + pluginFolder);
        juce::File userAuDir   = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                                    .getChildFile("Library/Audio/Plug-Ins/Components").getChildFile(macAuFolder);
        juce::File sysAuDir("/Library/Audio/Plug-Ins/Components/" + macAuFolder);

        std::vector<juce::File> allOldLocations = {
            userVst3Dir, sysVst3Dir, userAuDir, sysAuDir
        };
#else
        juce::File userVst3Dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                                    .getChildFile("Local\\Programs\\Common\\VST3\\" + pluginFolder);
        juce::File sysVst3Dir("C:\\Program Files\\Common Files\\VST3\\" + pluginFolder);
        juce::File x86Vst3Dir("C:\\Program Files (x86)\\Common Files\\VST3\\" + pluginFolder);
        juce::File steinbergVst3Dir("C:\\Program Files\\Steinberg\\VSTPlugins\\" + pluginFolder);
        juce::File generalVst3Dir("C:\\Program Files\\VSTPlugins\\" + pluginFolder);

        std::vector<juce::File> allOldLocations = {
            userVst3Dir, sysVst3Dir, x86Vst3Dir, steinbergVst3Dir, generalVst3Dir
        };
#endif

        // 1. Proactive DAW File-Lock Detection
        for (const auto& loc : allOldLocations)
        {
            if (isVst3FileLocked(loc))
            {
                errorMsg = "UNDERGROUND.vst3 is currently in use! Please close FL Studio / your DAW before updating.";
                isDownloading.store(false);
                juce::MessageManager::callAsync([this, errorMsg] {
                    if (onComplete) onComplete(false, errorMsg);
                });
                return;
            }
        }

        if (!tempDir.exists())
            tempDir.createDirectory();

        if (downloadUrl.isNotEmpty())
        {
            // 2. HTTPS Cloud Download Stream
            juce::URL url(downloadUrl);
            juce::File downloadedFile = tempDir.getChildFile("downloaded_plugin.zip");
            if (downloadedFile.existsAsFile())
                downloadedFile.deleteFile();

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

                            if (totalBytes > 0 && onProgress)
                            {
                                float fraction = static_cast<float>(bytesWritten) / static_cast<float>(totalBytes);
                                juce::MessageManager::callAsync([this, fraction] {
                                    if (onProgress) onProgress(fraction);
                                });
                            }
                        }
                    }
                    fileOut.flush();

                    // 3. SHA-256 Checksum Validation
                    if (targetSha256.isNotEmpty())
                    {
                        juce::String actualHash = ManagerSelfUpdater::computeSHA256(downloadedFile);
                        if (actualHash != targetSha256)
                        {
                            errorMsg = "Plugin package SHA-256 verification failed!";
                            downloadedFile.deleteFile();
                            isDownloading.store(false);
                            juce::MessageManager::callAsync([this, errorMsg] {
                                if (onComplete) onComplete(false, errorMsg);
                            });
                            return;
                        }
                    }

                    // 4. Clean purge of ALL old VST3 installations
                    for (auto& loc : allOldLocations)
                    {
                        if (loc.exists())
                        {
                            try { loc.deleteRecursively(); } catch (...) {}
                        }
                    }

                    // 5. Unpack fresh package to staging folder
                    juce::File stageExtract = tempDir.getChildFile("extracted_vst3");
                    if (stageExtract.exists())
                        stageExtract.deleteRecursively();
                    stageExtract.createDirectory();

                    juce::ZipFile zip(downloadedFile);
                    if (zip.getNumEntries() > 0)
                    {
                        zip.uncompressTo(stageExtract);

                        // Locate UNDERGROUND.vst3 within extracted structure
                        juce::File unpackedVst3 = stageExtract.getChildFile("UNDERGROUND.vst3");
                        if (!unpackedVst3.exists())
                        {
                            // If zip was packaged without top folder
                            unpackedVst3 = stageExtract;
                        }

                        // Install to User LocalAppData VST3 folder
                        userVst3Dir.getParentDirectory().createDirectory();
                        unpackedVst3.copyDirectoryTo(userVst3Dir);

                        // Also install to System-wide VST3 folder
                        try {
                            if (sysVst3Dir.getParentDirectory().exists())
                                unpackedVst3.copyDirectoryTo(sysVst3Dir);
                        } catch (...) {}

                        InstalledRegistry::setInstalledVersion(pluginId, version, userVst3Dir.getFullPathName());
                        success = true;
                    }
                }
            }
            else
            {
                errorMsg = "Unable to connect to cloud download server.";
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

    std::atomic<bool> isDownloading { false };
    juce::String downloadUrl { "" };
    juce::String pluginId { "pluggedin_underground" };
    juce::String version { "1.0.0" };
    juce::String targetSha256 { "" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CloudDownloader)
};
