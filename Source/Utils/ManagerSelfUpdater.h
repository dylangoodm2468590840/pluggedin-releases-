#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <functional>
#include <atomic>
#include <windows.h>
#include <wincrypt.h>

/**
 * @class ManagerSelfUpdater
 * @brief Handles downloading new Manager binaries, validating SHA-256,
 * and performing detached atomic trampoline hand-off to update the running Manager.
 */
class ManagerSelfUpdater : private juce::Thread
{
public:
    std::function<void(float)> onProgress;
    std::function<void(bool, const juce::String&)> onComplete;

    ManagerSelfUpdater()
        : juce::Thread("ManagerSelfUpdaterThread")
    {
    }

    ~ManagerSelfUpdater() override
    {
        stopThread(3000);
    }

    static juce::String computeSHA256(const juce::File& file)
    {
        if (!file.existsAsFile()) return "";

        juce::FileInputStream stream(file);
        if (!stream.openedOk()) return "";

        HCRYPTPROV hProv = 0;
        HCRYPTHASH hHash = 0;
        if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
            return "";

        if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash))
        {
            CryptReleaseContext(hProv, 0);
            return "";
        }

        char buffer[8192];
        while (!stream.isExhausted())
        {
            int bytesRead = stream.read(buffer, sizeof(buffer));
            if (bytesRead > 0)
            {
                CryptHashData(hHash, reinterpret_cast<const BYTE*>(buffer), static_cast<DWORD>(bytesRead), 0);
            }
        }

        BYTE hashBytes[32];
        DWORD hashLen = 32;
        juce::String hashStr = "";

        if (CryptGetHashParam(hHash, HP_HASHVAL, hashBytes, &hashLen, 0))
        {
            for (DWORD i = 0; i < hashLen; ++i)
            {
                hashStr += juce::String::toHexString(hashBytes[i]).paddedLeft('0', 2);
            }
        }

        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return hashStr.toLowerCase();
    }

    void startSelfUpdateAsync(const juce::String& urlStr, const juce::String& targetVersion, const juce::String& expectedSha256 = "")
    {
        updateUrl = urlStr;
        newVersion = targetVersion;
        targetSha256 = expectedSha256.toLowerCase().trim();

        isUpdating.store(true);
        startThread();
    }

    bool isBusy() const noexcept { return isUpdating.load(); }

private:
    void run() override
    {
        bool success = false;
        juce::String errorMsg = "";

        juce::File tempDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                                  .getChildFile("PluggedIN\\temp\\manager_update");
        if (tempDir.exists())
            tempDir.deleteRecursively();
        tempDir.createDirectory();

        juce::File downloadedFile = tempDir.getChildFile("PluggedIN_Central_New.exe");
        juce::File currentExe = juce::File::getSpecialLocation(juce::File::currentExecutableFile);

        // 1. Download or Stage the new Manager Executable
        if (updateUrl.startsWithIgnoreCase("http://") || updateUrl.startsWithIgnoreCase("https://"))
        {
            juce::URL url(updateUrl);
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
                }
            }
        }
        else if (juce::File(updateUrl).existsAsFile())
        {
            // Local file/staged update source for testing
            juce::File localSrc(updateUrl);
            localSrc.copyFileTo(downloadedFile);
        }

        if (!downloadedFile.existsAsFile() || downloadedFile.getSize() < 1000)
        {
            errorMsg = "Failed to download new Manager update payload.";
            isUpdating.store(false);
            juce::MessageManager::callAsync([this, errorMsg] {
                if (onComplete) onComplete(false, errorMsg);
            });
            return;
        }

        // 2. Validate SHA-256 if supplied
        if (targetSha256.isNotEmpty())
        {
            juce::String actualHash = computeSHA256(downloadedFile);
            if (actualHash != targetSha256)
            {
                errorMsg = "SHA-256 integrity verification failed!";
                downloadedFile.deleteFile();
                isUpdating.store(false);
                juce::MessageManager::callAsync([this, errorMsg] {
                    if (onComplete) onComplete(false, errorMsg);
                });
                return;
            }
        }

        // 2.5 If downloaded payload is a ZIP archive, uncompress it to extract the executable
        juce::File extractedExe = tempDir.getChildFile("Unpacked_Central.exe");
        if (extractedExe.existsAsFile()) extractedExe.deleteFile();

        {
            juce::ZipFile zip(downloadedFile);
            if (zip.getNumEntries() > 0)
            {
                for (int e = 0; e < zip.getNumEntries(); ++e)
                {
                    auto* entry = zip.getEntry(e);
                    if (entry != nullptr && entry->filename.endsWithIgnoreCase(".exe"))
                    {
                        std::unique_ptr<juce::InputStream> inStream(zip.createStreamForEntry(e));
                        if (inStream != nullptr)
                        {
                            juce::FileOutputStream fos(extractedExe);
                            if (fos.openedOk())
                            {
                                fos.writeFromInputStream(*inStream, -1);
                            }
                            break;
                        }
                    }
                }
            }
        } // ZipFile destroyed, downloadedFile handle closed!

        if (extractedExe.existsAsFile() && extractedExe.getSize() > 100000)
        {
            downloadedFile.deleteFile();
            extractedExe.moveFileTo(downloadedFile);
        }

        // 3. Create Detached Windows Trampoline Batch Script with PowerShell
        juce::File trampolineScript = tempDir.getChildFile("apply_manager_update.bat");
        DWORD currentPid = GetCurrentProcessId();

        juce::String scriptContent =
            "@echo off\r\n"
            "powershell.exe -NoProfile -Command \""
            "Start-Sleep -Milliseconds 800; "
            "$pidToWait = " + juce::String(currentPid) + "; "
            "for ($i=0; $i -lt 30; $i++) { "
            "  $p = Get-Process -Id $pidToWait -ErrorAction SilentlyContinue; "
            "  if ($p -eq $null) { break; } "
            "  Start-Sleep -Milliseconds 500; "
            "} "
            "Start-Sleep -Milliseconds 500; "
            "Copy-Item -LiteralPath '" + downloadedFile.getFullPathName() + "' -Destination '" + currentExe.getFullPathName() + "' -Force; "
            "Start-Process -FilePath '" + currentExe.getFullPathName() + "'\""
            "\r\nexit\r\n";

        trampolineScript.replaceWithText(scriptContent);

        // 4. Launch Detached Trampoline Process
        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
        ZeroMemory(&pi, sizeof(pi));

        juce::String cmdLine = "cmd.exe /c \"" + trampolineScript.getFullPathName() + "\"";
        std::vector<char> cmdLineBuf(cmdLine.toRawUTF8(), cmdLine.toRawUTF8() + cmdLine.getNumBytesAsUTF8() + 1);

        if (CreateProcessA(NULL, cmdLineBuf.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW | DETACHED_PROCESS, NULL, NULL, &si, &pi))
        {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            success = true;
        }
        else
        {
            errorMsg = "Failed to launch update trampoline process.";
        }

        isUpdating.store(false);

        // 5. Notify UI & Gracefully Exit Current Application
        juce::MessageManager::callAsync([this, success, errorMsg] {
            if (onComplete)
                onComplete(success, errorMsg);

            if (success)
            {
                // Graceful quit to allow trampoline to swap executable
                juce::JUCEApplication::getInstance()->systemRequestedQuit();
            }
        });
    }

    std::atomic<bool> isUpdating { false };
    juce::String updateUrl { "" };
    juce::String newVersion { "" };
    juce::String targetSha256 { "" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ManagerSelfUpdater)
};
