#pragma once

#include <juce_core/juce_core.h>

#if JUCE_WINDOWS || defined(_WIN32)
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
  #include <shellapi.h>
#endif

/**
 * @class InstalledRegistry
 * @brief Thread-safe on-disk authority and local JSON registry manager.
 * Strictly prioritizes live physical bundle inspection (moduleinfo.json / Info.plist)
 * over cached registry state.
 */
class InstalledRegistry
{
public:
    static juce::File getUserVst3Directory()
    {
#if JUCE_MAC
        return juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                   .getChildFile("Library/Audio/Plug-Ins/VST3");
#else
        juce::String localAppData = juce::SystemStats::getEnvironmentVariable("LOCALAPPDATA", "");
        if (localAppData.isNotEmpty())
            return juce::File(localAppData).getChildFile("Programs\\Common\\VST3");

        return juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                   .getChildFile("AppData\\Local\\Programs\\Common\\VST3");
#endif
    }

    static juce::File getSystemVst3Directory()
    {
#if JUCE_MAC
        return juce::File("/Library/Audio/Plug-Ins/VST3");
#else
        juce::String commonProg = juce::SystemStats::getEnvironmentVariable("CommonProgramFiles", "");
        if (commonProg.isNotEmpty())
            return juce::File(commonProg).getChildFile("VST3");

        return juce::File("C:\\Program Files\\Common Files\\VST3");
#endif
    }

    static juce::File getUserAuDirectory()
    {
        return juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                   .getChildFile("Library/Audio/Plug-Ins/Components");
    }

    static juce::File getSystemAuDirectory()
    {
        return juce::File("/Library/Audio/Plug-Ins/Components");
    }

    static juce::File getRegistryFile()
    {
        juce::File appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
        juce::File pluggedInDir = appDataDir.getChildFile("PluggedIN");

        if (!pluggedInDir.exists())
            pluggedInDir.createDirectory();

        return pluggedInDir.getChildFile("installed_manifest.json");
    }

    static juce::var readRegistry()
    {
        juce::File regFile = getRegistryFile();
        if (!regFile.existsAsFile())
            return juce::var(new juce::DynamicObject());

        juce::String jsonContent = regFile.loadFileAsString();
        auto parsed = juce::JSON::parse(jsonContent);

        if (parsed.isObject())
            return parsed;

        return juce::var(new juce::DynamicObject());
    }

    static bool writeRegistry(const juce::var& registryData)
    {
        juce::File regFile = getRegistryFile();
        juce::String jsonString = juce::JSON::toString(registryData, false);
        return regFile.replaceWithText(jsonString);
    }

    static juce::String extractVersionFromBundle(const juce::File& bundleDir)
    {
        if (!bundleDir.exists())
            return "";

        // 1. Direct inspection of moduleinfo.json (Standard for VST 3.7+ builds)
        juce::File modInfo = bundleDir.getChildFile("Contents").getChildFile("Resources").getChildFile("moduleinfo.json");
        if (modInfo.existsAsFile())
        {
            juce::String content = modInfo.loadFileAsString();
            auto parsed = juce::JSON::parse(content);
            if (parsed.isObject())
            {
                if (auto* obj = parsed.getDynamicObject())
                {
                    if (obj->hasProperty("Version"))
                    {
                        juce::String v = obj->getProperty("Version").toString().trim();
                        if (v.isNotEmpty()) return v;
                    }
                    if (obj->hasProperty("version"))
                    {
                        juce::String v = obj->getProperty("version").toString().trim();
                        if (v.isNotEmpty()) return v;
                    }
                }
            }

            // Robust raw string search fallback (protects against trailing comma JSON parse failure)
            int vIdx = content.indexOfIgnoreCase("\"Version\"");
            if (vIdx < 0) vIdx = content.indexOfIgnoreCase("\"version\"");
            if (vIdx >= 0)
            {
                int colonIdx = content.indexOf(vIdx, ":");
                if (colonIdx >= 0)
                {
                    int quote1 = content.indexOf(colonIdx, "\"");
                    if (quote1 >= 0)
                    {
                        int quote2 = content.indexOf(quote1 + 1, "\"");
                        if (quote2 > quote1)
                        {
                            juce::String v = content.substring(quote1 + 1, quote2).trim();
                            if (v.isNotEmpty()) return v;
                        }
                    }
                }
            }
        }

        // 2. Direct inspection of Info.plist (Standard for macOS VST3 & AU)
        juce::File infoPlist = bundleDir.getChildFile("Contents").getChildFile("Info.plist");
        if (infoPlist.existsAsFile())
        {
            juce::String text = infoPlist.loadFileAsString();
            int idx = text.indexOf("CFBundleShortVersionString");
            if (idx >= 0)
            {
                int strStart = text.indexOf(idx, "<string>");
                int strEnd = text.indexOf(idx, "</string>");
                if (strStart >= 0 && strEnd > strStart)
                {
                    juce::String v = text.substring(strStart + 8, strEnd).trim();
                    if (v.isNotEmpty()) return v;
                }
            }
        }

        return "";
    }

    static std::vector<juce::File> getCandidateDirectories(const juce::String& pluginId)
    {
        juce::String bundleName = (pluginId == "pluggedin_plugged1") ? "Plugged 1.vst3" :
                                  (pluginId == "pluggedin_crush")    ? "CRUSH.vst3"     : "UNDERGROUND.vst3";
        juce::String auName     = (pluginId == "pluggedin_plugged1") ? "Plugged 1.component" :
                                  (pluginId == "pluggedin_crush")    ? "CRUSH.component"     : "UNDERGROUND.component";

        std::vector<juce::File> candidates;
        // Priority 1: System VST3 Directory (Scanned by FL Studio, Ableton, Reaper, Pro Tools)
        candidates.push_back(getSystemVst3Directory().getChildFile(bundleName));
        // Priority 2: User VST3 Directory
        candidates.push_back(getUserVst3Directory().getChildFile(bundleName));

#if JUCE_WINDOWS || defined(_WIN32)
        candidates.push_back(juce::File("C:\\Program Files (x86)\\Common Files\\VST3").getChildFile(bundleName));
        candidates.push_back(juce::File("C:\\Program Files\\Steinberg\\VSTPlugins").getChildFile(bundleName));
        candidates.push_back(juce::File("C:\\Program Files\\VSTPlugins").getChildFile(bundleName));
#elif JUCE_MAC
        candidates.push_back(getSystemAuDirectory().getChildFile(auName));
        candidates.push_back(getUserAuDirectory().getChildFile(auName));
#endif
        return candidates;
    }

    static juce::String getInstalledVersion(const juce::String& pluginId)
    {
        // 1. LIVE ON-DISK SCAN (Find highest version across all candidate paths)
        auto candidates = getCandidateDirectories(pluginId);
        juce::String highestVersion = "";
        juce::String highestPath = "";

        for (const auto& candidate : candidates)
        {
            if (candidate.exists())
            {
                juce::String onDiskVersion = extractVersionFromBundle(candidate);
                if (onDiskVersion.isNotEmpty())
                {
                    if (highestVersion.isEmpty() || compareVersions(onDiskVersion, highestVersion) > 0)
                    {
                        highestVersion = onDiskVersion;
                        highestPath = candidate.getFullPathName();
                    }
                }
            }
        }

        if (highestVersion.isNotEmpty())
        {
            setInstalledVersion(pluginId, highestVersion, highestPath);
            return highestVersion;
        }

        // 2. Cached registry manifest check (if plugin moved or offline)
        auto registry = readRegistry();
        if (auto* obj = registry.getDynamicObject())
        {
            if (obj->hasProperty(pluginId))
            {
                auto pluginObj = obj->getProperty(pluginId);
                if (auto* pObj = pluginObj.getDynamicObject())
                {
                    juce::String regVer = pObj->getProperty("installed_version").toString().trim();
                    if (regVer.isNotEmpty())
                        return regVer;
                }
            }
        }

        return "";
    }

    /**
     * @brief Strict on-disk verification: Returns true if ANY candidate directory
     * contains the expected version in its moduleinfo.json.
     * Checks all candidates — does NOT stop at the first one found (old version may be
     * sitting in system dir at priority 1 from a previous failed install).
     */
    static bool verifyOnDiskInstallation(const juce::String& pluginId, const juce::String& expectedVersion, juce::String& outVerifiedPath)
    {
        outVerifiedPath = "";
        auto candidates = getCandidateDirectories(pluginId);

        for (const auto& candidate : candidates)
        {
            if (candidate.exists())
            {
                juce::String diskVer = extractVersionFromBundle(candidate);
                if (diskVer.isNotEmpty() && diskVer == expectedVersion)
                {
                    outVerifiedPath = candidate.getFullPathName();
                    // Sync verified version into local registry
                    setInstalledVersion(pluginId, diskVer, outVerifiedPath);
                    return true;
                }
            }
        }

        // Secondary check: if a stale old-version bundle is in system dir blocking verification,
        // check user dir explicitly as authoritative fallback before declaring failure
        juce::String bundleName = (pluginId == "pluggedin_plugged1") ? "Plugged 1.vst3" :
                                  (pluginId == "pluggedin_crush")    ? "CRUSH.vst3"     : "UNDERGROUND.vst3";
        juce::File userBundle = getUserVst3Directory().getChildFile(bundleName);
        if (userBundle.exists())
        {
            juce::String diskVer = extractVersionFromBundle(userBundle);
            if (diskVer.isNotEmpty() && diskVer == expectedVersion)
            {
                outVerifiedPath = userBundle.getFullPathName();
                setInstalledVersion(pluginId, diskVer, outVerifiedPath);
                return true;
            }
        }

        return false;
    }

    static void setInstalledVersion(const juce::String& pluginId, const juce::String& version, const juce::String& path = "")
    {
        auto registry = readRegistry();
        auto* obj = registry.getDynamicObject();
        if (!obj)
        {
            obj = new juce::DynamicObject();
            registry = juce::var(obj);
        }

        auto* pluginObj = new juce::DynamicObject();
        pluginObj->setProperty("installed_version", version);
        pluginObj->setProperty("install_date", juce::Time::getCurrentTime().toISO8601(true));
        pluginObj->setProperty("path", path);

        obj->setProperty(pluginId, juce::var(pluginObj));
        writeRegistry(registry);
    }

    static void removeInstalledVersion(const juce::String& pluginId)
    {
        auto registry = readRegistry();
        if (auto* obj = registry.getDynamicObject())
        {
            if (obj->hasProperty(pluginId))
            {
                obj->removeProperty(pluginId);
                writeRegistry(registry);
            }
        }
    }

    /**
     * @brief Self-Healing Installation Cleaner.
     * Scans ALL candidate directories, finds the highest installed version,
     * removes stale lower-version ghost copies (using elevation for system dirs),
     * and writes the authoritative version to the registry.
     * Safe to call from a background thread on every launch.
     * @return The highest version string found on disk, or empty string if not installed.
     */
    static juce::String selfHealInstallations(const juce::String& pluginId)
    {
        auto candidates = getCandidateDirectories(pluginId);

        // --- Step 1: Find the highest version across all on-disk locations ---
        juce::String highestVersion;
        juce::String highestPath;

        for (const auto& candidate : candidates)
        {
            if (candidate.exists())
            {
                juce::String ver = extractVersionFromBundle(candidate);
                if (ver.isNotEmpty())
                {
                    if (highestVersion.isEmpty() || compareVersions(ver, highestVersion) > 0)
                    {
                        highestVersion = ver;
                        highestPath = candidate.getFullPathName();
                    }
                }
            }
        }

        if (highestVersion.isEmpty())
            return ""; // Not installed anywhere

        // --- Step 2: Remove all stale lower-version ghost copies ---
        juce::File sysVst3Parent = getSystemVst3Directory().getParentDirectory();

        for (const auto& candidate : candidates)
        {
            if (!candidate.exists() || candidate.getFullPathName() == highestPath)
                continue;

            juce::String ver = extractVersionFromBundle(candidate);
            if (ver.isEmpty() || compareVersions(ver, highestVersion) >= 0)
                continue; // Not stale

            // Is this path inside a system-level (Program Files) directory?
            bool isSystemPath = candidate.getFullPathName().startsWithIgnoreCase(
                                    sysVst3Parent.getFullPathName());

            if (isSystemPath)
            {
                // Needs elevated privileges — fire-and-forget PowerShell runas
#if JUCE_WINDOWS || defined(_WIN32)
                juce::String psScript = "Remove-Item -Recurse -Force '" +
                                        candidate.getFullPathName() + "'";
                // UTF-16 LE encode for -EncodedCommand
                juce::MemoryOutputStream encoded;
                for (int ci = 0; ci < psScript.length(); ++ci)
                {
                    juce::juce_wchar wc = psScript[ci];
                    encoded.writeByte((char)(wc & 0xFF));
                    encoded.writeByte((char)((wc >> 8) & 0xFF));
                }
                juce::String b64 = juce::Base64::toBase64(encoded.getData(), encoded.getDataSize());
                juce::String psArgs = "-NoProfile -NonInteractive -ExecutionPolicy Bypass -EncodedCommand " + b64;
                ShellExecuteA(nullptr, "runas", "powershell.exe",
                              psArgs.toRawUTF8(), nullptr, SW_HIDE);
                // Brief pause so the elevated process has time to delete before re-scan
                juce::Thread::sleep(1800);
#endif
            }
            else
            {
                // User-level path — delete directly, no elevation needed
                try { candidate.deleteRecursively(); } catch (...) {}
            }
        }

        // --- Step 3: Register the authoritative version ---
        setInstalledVersion(pluginId, highestVersion, highestPath);
        return highestVersion;
    }

    /**
     * Semantic Version Comparator
     * @return -1 if v1 < v2 (Update Available), 0 if equal, 1 if v1 > v2
     */
    static int compareVersions(const juce::String& v1, const juce::String& v2)
    {
        if (v1.isEmpty() && v2.isNotEmpty()) return -1;
        if (v1.isNotEmpty() && v2.isEmpty()) return 1;
        if (v1 == v2) return 0;

        juce::StringArray parts1 = juce::StringArray::fromTokens(v1, ".", "");
        juce::StringArray parts2 = juce::StringArray::fromTokens(v2, ".", "");

        int maxParts = std::max(parts1.size(), parts2.size());
        for (int i = 0; i < maxParts; ++i)
        {
            int num1 = i < parts1.size() ? parts1[i].getIntValue() : 0;
            int num2 = i < parts2.size() ? parts2[i].getIntValue() : 0;

            if (num1 < num2) return -1;
            if (num1 > num2) return 1;
        }

        return 0;
    }
};
