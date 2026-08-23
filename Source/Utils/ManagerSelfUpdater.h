#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <functional>
#include <atomic>

#if JUCE_WINDOWS || defined(_WIN32)
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
  #include <wincrypt.h>
#elif JUCE_MAC
  #include <CommonCrypto/CommonCrypto.h>
  #include <unistd.h>
#endif

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

#if JUCE_WINDOWS || defined(_WIN32)
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

#elif JUCE_MAC
        // macOS: CommonCrypto — no external library required
        CC_SHA256_CTX ctx;
        CC_SHA256_Init(&ctx);

        unsigned char buf[8192];
        while (!stream.isExhausted())
        {
            int bytesRead = stream.read(buf, sizeof(buf));
            if (bytesRead > 0)
                CC_SHA256_Update(&ctx, buf, static_cast<CC_LONG>(bytesRead));
        }

        unsigned char digest[CC_SHA256_DIGEST_LENGTH];
        CC_SHA256_Final(digest, &ctx);

        juce::String hashStr;
        for (int i = 0; i < CC_SHA256_DIGEST_LENGTH; ++i)
            hashStr += juce::String::toHexString(digest[i]).paddedLeft('0', 2);
        return hashStr.toLowerCase();

#else
        return ""; // SHA-256 not implemented on this platform — verification skipped
#endif
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
        juce::String errorMsg;

        // ── Platform-specific temp dir and download target ──
#if JUCE_MAC
        juce::File tempDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                                  .getChildFile("PluggedIN/temp/manager_update");
        juce::File downloadedFile = tempDir.getChildFile("PluggedIN_Central_New.zip");
#else
        juce::File tempDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                                  .getChildFile("PluggedIN\\temp\\manager_update");
        juce::File downloadedFile = tempDir.getChildFile("PluggedIN_Central_New.exe");
#endif

        if (tempDir.exists()) tempDir.deleteRecursively();
        tempDir.createDirectory();

        juce::File currentExe = juce::File::getSpecialLocation(juce::File::currentExecutableFile);

        // ─────────────────────────────────────────────────
        // STEP 1: Download the new Manager package
        // ─────────────────────────────────────────────────
        bool downloadDone = false;

        if (updateUrl.startsWithIgnoreCase("http://") || updateUrl.startsWithIgnoreCase("https://"))
        {
#if JUCE_MAC
            // macOS Tier 1: system /usr/bin/curl (present since macOS 10.13, supports TLS 1.3 + GitHub 302)
            juce::String curlCmd = "/usr/bin/curl -L -s -f --max-time 60 -o \""
                                    + downloadedFile.getFullPathName() + "\" \""
                                    + updateUrl + "\"";
            juce::ChildProcess curlProc;
            if (curlProc.start(curlCmd) && curlProc.waitForProcessToFinish(65000))
            {
                if (downloadedFile.existsAsFile() && downloadedFile.getSize() > 1000)
                {
                    downloadDone = true;
                    if (onProgress) juce::MessageManager::callAsync([this] { if (onProgress) onProgress(1.0f); });
                }
            }
            // macOS Tier 2: JUCE URL stream fallback
            if (!downloadDone)
            {
                juce::URL url(updateUrl);
                auto stream = url.createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                                                    .withConnectionTimeoutMs(15000)
                                                    .withHttpRequestCmd("GET")
                                                    .withExtraHeaders("User-Agent: PluggedIN-Central\r\nAccept: */*\r\n"));
                if (stream != nullptr)
                {
                    juce::FileOutputStream fileOut(downloadedFile);
                    if (fileOut.openedOk())
                    {
                        juce::int64 totalBytes = stream->getTotalLength();
                        juce::int64 bytesWritten = 0;
                        char buf[8192];
                        while (!stream->isExhausted() && !threadShouldExit())
                        {
                            int n = stream->read(buf, sizeof(buf));
                            if (n > 0)
                            {
                                fileOut.write(buf, static_cast<size_t>(n));
                                bytesWritten += n;
                                if (totalBytes > 0 && onProgress)
                                {
                                    float frac = static_cast<float>(bytesWritten) / static_cast<float>(totalBytes);
                                    juce::MessageManager::callAsync([this, frac] { if (onProgress) onProgress(frac); });
                                }
                            }
                        }
                        fileOut.flush();
                        if (downloadedFile.existsAsFile() && downloadedFile.getSize() > 1000) downloadDone = true;
                    }
                }
            }

#else // Windows
            // Tier 1: Windows native curl.exe (follows 302 redirects, supports TLS 1.3)
            juce::File curlExe("C:\\Windows\\System32\\curl.exe");
            if (curlExe.existsAsFile())
            {
                juce::String curlCmd = "curl.exe -L -s -f -o \"" + downloadedFile.getFullPathName() + "\" \"" + updateUrl + "\"";
                juce::ChildProcess proc;
                if (proc.start(curlCmd) && proc.waitForProcessToFinish(30000))
                {
                    if (downloadedFile.existsAsFile() && downloadedFile.getSize() > 1000)
                    {
                        downloadDone = true;
                        if (onProgress) juce::MessageManager::callAsync([this] { if (onProgress) onProgress(1.0f); });
                    }
                }
            }
            // Tier 2: JUCE URL Stream Fallback
            if (!downloadDone)
            {
                juce::URL url(updateUrl);
                auto stream = url.createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                                                    .withConnectionTimeoutMs(15000)
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
                                    juce::MessageManager::callAsync([this, fraction] { if (onProgress) onProgress(fraction); });
                                }
                            }
                        }
                        fileOut.flush();
                        if (downloadedFile.existsAsFile() && downloadedFile.getSize() > 1000) downloadDone = true;
                    }
                }
            }
            // Tier 3: PowerShell WebClient Fallback
            if (!downloadDone)
            {
                juce::String psCmd = "powershell.exe -NoProfile -Command \"(New-Object System.Net.WebClient).DownloadFile('" + updateUrl + "', '" + downloadedFile.getFullPathName() + "')\"";
                juce::ChildProcess psProc;
                if (psProc.start(psCmd) && psProc.waitForProcessToFinish(30000))
                {
                    if (downloadedFile.existsAsFile() && downloadedFile.getSize() > 1000) downloadDone = true;
                }
            }
#endif
        }
        else if (juce::File(updateUrl).existsAsFile())
        {
            juce::File(updateUrl).copyFileTo(downloadedFile);
            downloadDone = downloadedFile.existsAsFile() && downloadedFile.getSize() > 1000;
        }

        if (!downloadedFile.existsAsFile() || downloadedFile.getSize() < 1000)
        {
            errorMsg = "Failed to download new Manager update payload.";
            isUpdating.store(false);
            juce::MessageManager::callAsync([this, errorMsg] { if (onComplete) onComplete(false, errorMsg); });
            return;
        }

        // ─────────────────────────────────────────────────
        // STEP 2: SHA-256 integrity check
        // ─────────────────────────────────────────────────
        if (targetSha256.isNotEmpty())
        {
            juce::String actualHash = computeSHA256(downloadedFile);
            if (actualHash != targetSha256)
            {
                errorMsg = "SHA-256 integrity verification failed!";
                downloadedFile.deleteFile();
                isUpdating.store(false);
                juce::MessageManager::callAsync([this, errorMsg] { if (onComplete) onComplete(false, errorMsg); });
                return;
            }
        }

        // ─────────────────────────────────────────────────
        // STEP 3: Extract ZIP payload + create trampoline
        // ─────────────────────────────────────────────────
#if JUCE_MAC
        // macOS: use ditto to extract the ZIP, preserving symlinks and resource forks
        juce::File extractDir = tempDir.getChildFile("extracted");
        extractDir.createDirectory();
        juce::String dittoCmd = "/usr/bin/ditto -x -k \"" + downloadedFile.getFullPathName()
                                + "\" \"" + extractDir.getFullPathName() + "\"";
        juce::ChildProcess dittoProc;
        if (dittoProc.start(dittoCmd)) dittoProc.waitForProcessToFinish(30000);

        // Find the .app bundle that was extracted
        juce::Array<juce::File> appBundles;
        extractDir.findChildFiles(appBundles, juce::File::findDirectories, false, "*.app");
        juce::String srcBundle = (appBundles.size() > 0)
                                     ? appBundles[0].getFullPathName()
                                     : downloadedFile.getFullPathName();

        // Walk up from the executable to find the .app bundle path
        juce::File appBundlePath = currentExe;
        while (appBundlePath.getParentDirectory() != appBundlePath)
        {
            if (appBundlePath.getFullPathName().endsWithIgnoreCase(".app")) break;
            appBundlePath = appBundlePath.getParentDirectory();
        }
        juce::String dstBundle = appBundlePath.getFullPathName();
        pid_t currentPid = getpid();

        // Write the .sh trampoline
        juce::File trampolineScript = tempDir.getChildFile("apply_manager_update.sh");
        juce::String shScript =
            "#!/bin/bash\n"
            "PID_TO_WAIT=" + juce::String(currentPid) + "\n"
            "for i in $(seq 1 60); do\n"
            "  kill -0 $PID_TO_WAIT 2>/dev/null || break\n"
            "  sleep 0.5\n"
            "done\n"
            "sleep 0.5\n"
            "rm -rf \"" + dstBundle + "\"\n"
            "/usr/bin/ditto \"" + srcBundle + "\" \"" + dstBundle + "\"\n"
            "codesign --force --deep --sign - \"" + dstBundle + "\" 2>/dev/null\n"
            "open \"" + dstBundle + "\"\n"
            "rm -rf \"" + tempDir.getFullPathName() + "\"\n";

        trampolineScript.replaceWithText(shScript);

        juce::ChildProcess chmodProc;
        chmodProc.start("chmod +x \"" + trampolineScript.getFullPathName() + "\"");
        chmodProc.waitForProcessToFinish(3000);

        // Launch the trampoline detached from this process
        juce::String launchCmd = "bash \"" + trampolineScript.getFullPathName() + "\" &";
        success = (system(launchCmd.toRawUTF8()) == 0);
        if (!success) errorMsg = "Failed to launch update trampoline script.";

#else // Windows
        // Extract .exe from ZIP if needed
        juce::File extractedExe = tempDir.getChildFile("Unpacked_Central.exe");
        if (extractedExe.existsAsFile()) extractedExe.deleteFile();
        {
            juce::ZipFile zip(downloadedFile);
            for (int e = 0; e < zip.getNumEntries(); ++e)
            {
                auto* entry = zip.getEntry(e);
                if (entry != nullptr && entry->filename.endsWithIgnoreCase(".exe"))
                {
                    std::unique_ptr<juce::InputStream> inStream(zip.createStreamForEntry(e));
                    if (inStream != nullptr)
                    {
                        juce::FileOutputStream fos(extractedExe);
                        if (fos.openedOk()) fos.writeFromInputStream(*inStream, -1);
                        break;
                    }
                }
            }
        }
        if (extractedExe.existsAsFile() && extractedExe.getSize() > 100000)
        {
            downloadedFile.deleteFile();
            extractedExe.moveFileTo(downloadedFile);
        }

        // Write the .bat trampoline
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
#endif

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
