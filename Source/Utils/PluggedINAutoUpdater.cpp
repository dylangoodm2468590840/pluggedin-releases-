#include "PluggedINAutoUpdater.h"

PluggedINAutoUpdater::PluggedINAutoUpdater()
    : juce::Thread("PluggedINAutoUpdaterThread")
{
}

PluggedINAutoUpdater::~PluggedINAutoUpdater()
{
    stopThread(2000);
}

void PluggedINAutoUpdater::checkForUpdatesAsync(const juce::String& currentVersionStr) noexcept
{
    currentVersion = currentVersionStr;
    if (!isThreadRunning())
    {
        startThread();
    }
}

void PluggedINAutoUpdater::clearUpdateState() noexcept
{
    updateAvailable.store(false);
    const juce::ScopedLock sl(cacheLock);
    for (auto& pair : pluginCatalogCache)
    {
        pair.second.updateAvailable = false;
    }
}

bool PluggedINAutoUpdater::isPluginUpdateAvailable(const juce::String& pluginId) const noexcept
{
    const juce::ScopedLock sl(cacheLock);
    auto it = pluginCatalogCache.find(pluginId);
    if (it != pluginCatalogCache.end())
        return it->second.updateAvailable;
    return false;
}

juce::String PluggedINAutoUpdater::getPluginLatestVersion(const juce::String& pluginId) const noexcept
{
    const juce::ScopedLock sl(cacheLock);
    auto it = pluginCatalogCache.find(pluginId);
    if (it != pluginCatalogCache.end() && it->second.latestVersion.isNotEmpty())
        return it->second.latestVersion;
    return "1.0.0";
}

juce::String PluggedINAutoUpdater::getPluginDownloadUrl(const juce::String& pluginId) const noexcept
{
    const juce::ScopedLock sl(cacheLock);
    auto it = pluginCatalogCache.find(pluginId);
    if (it != pluginCatalogCache.end())
        return it->second.downloadUrl;
    return "";
}

juce::String PluggedINAutoUpdater::getPluginSha256(const juce::String& pluginId) const noexcept
{
    const juce::ScopedLock sl(cacheLock);
    auto it = pluginCatalogCache.find(pluginId);
    if (it != pluginCatalogCache.end())
        return it->second.sha256;
    return "";
}

juce::String PluggedINAutoUpdater::getPluginChangelog(const juce::String& pluginId) const noexcept
{
    const juce::ScopedLock sl(cacheLock);
    auto it = pluginCatalogCache.find(pluginId);
    if (it != pluginCatalogCache.end())
        return it->second.changelog;
    return "";
}

juce::String PluggedINAutoUpdater::getPluginName(const juce::String& pluginId) const noexcept
{
    const juce::ScopedLock sl(cacheLock);
    auto it = pluginCatalogCache.find(pluginId);
    if (it != pluginCatalogCache.end())
        return it->second.name;
    return "";
}

std::vector<juce::String> PluggedINAutoUpdater::getDiscoveredPluginIds() const noexcept
{
    std::vector<juce::String> ids;
    const juce::ScopedLock sl(cacheLock);
    for (const auto& pair : pluginCatalogCache)
    {
        ids.push_back(pair.first);
    }
    return ids;
}

std::vector<CloudPluginEntry> PluggedINAutoUpdater::getAllDiscoveredPlugins() const noexcept
{
    std::vector<CloudPluginEntry> list;
    const juce::ScopedLock sl(cacheLock);
    for (const auto& pair : pluginCatalogCache)
    {
        list.push_back(pair.second);
    }
    return list;
}

void PluggedINAutoUpdater::parseManifestJson(const juce::String& jsonText)
{
    auto jsonResult = juce::JSON::parse(jsonText);
    if (!jsonResult.isObject())
        return;

    auto* obj = jsonResult.getDynamicObject();
    if (!obj)
        return;

    // 1. Manager Section (Supports both new schema and legacy hub_version)
    if (obj->hasProperty("manager"))
    {
        auto mgrVar = obj->getProperty("manager");
        if (auto* mgrObj = mgrVar.getDynamicObject())
        {
            latestVersion = mgrObj->getProperty("version").toString();
            changelog = mgrObj->getProperty("release_notes").toString();

            if (mgrObj->hasProperty("packages"))
            {
                auto pkgVar = mgrObj->getProperty("packages");
                if (auto* pkgObj = pkgVar.getDynamicObject())
                {
#if JUCE_MAC
                    juce::String platformKey = "macos_universal";
#else
                    juce::String platformKey = "windows_x64";
#endif
                    if (pkgObj->hasProperty(platformKey))
                    {
                        auto platVar = pkgObj->getProperty(platformKey);
                        if (auto* platObj = platVar.getDynamicObject())
                        {
                            downloadUrl = platObj->getProperty("url").toString();
                            managerSha256 = platObj->getProperty("sha256").toString();
                        }
                    }
                }
            }

            bool needsHubUpdate = (InstalledRegistry::compareVersions(currentVersion, latestVersion) < 0);
            updateAvailable.store(needsHubUpdate);
        }
    }
    else if (obj->hasProperty("hub_version"))
    {
        latestVersion = obj->getProperty("hub_version").toString();
        bool needsHubUpdate = (InstalledRegistry::compareVersions(currentVersion, latestVersion) < 0);
        updateAvailable.store(needsHubUpdate);
    }

    // 2. Plugins Catalog
    if (obj->hasProperty("plugins"))
    {
        auto pluginsArray = obj->getProperty("plugins");
        if (pluginsArray.isArray())
        {
            for (int i = 0; i < pluginsArray.size(); ++i)
            {
                auto pluginEntry = pluginsArray[i];
                if (auto* pObj = pluginEntry.getDynamicObject())
                {
                    juce::String id = pObj->getProperty("id").toString();
                    if (id.isEmpty()) id = pObj->getProperty("plugin_id").toString();
                    if (id.isEmpty()) continue;

                    juce::String latest = pObj->getProperty("latest_version").toString();
                    juce::String pChangelog = pObj->getProperty("changelog").toString();

                    juce::String pUrl = "";
                    juce::String pSha = "";

                    if (pObj->hasProperty("packages"))
                    {
                        auto pkgVar = pObj->getProperty("packages");
                        if (auto* pkgObj = pkgVar.getDynamicObject())
                        {
#if JUCE_MAC
                            juce::String platformKey = "macos_universal";
#else
                            juce::String platformKey = "windows_x64";
#endif
                            if (pkgObj->hasProperty(platformKey))
                            {
                                auto platVar = pkgObj->getProperty(platformKey);
                                if (auto* platObj = platVar.getDynamicObject())
                                {
                                    pUrl = platObj->getProperty("url").toString();
                                    pSha = platObj->getProperty("sha256").toString();
                                }
                            }
                        }
                    }
                    
                    if (pUrl.isEmpty())
                    {
#if JUCE_MAC
                        if (pObj->hasProperty("download_url_mac"))
                            pUrl = pObj->getProperty("download_url_mac").toString();
                        else if (pObj->hasProperty("download_url_win"))
                            pUrl = pObj->getProperty("download_url_win").toString();
#else
                        if (pObj->hasProperty("download_url_win"))
                            pUrl = pObj->getProperty("download_url_win").toString();
#endif
                    }

                    CloudPluginEntry entry;
                    entry.id = id;
                    entry.name = pObj->getProperty("name").toString();
                    entry.subtitle = pObj->getProperty("subtitle").toString();
                    entry.category = pObj->getProperty("category").toString();
                    entry.description = pObj->getProperty("description").toString();
                    entry.latestVersion = latest.isNotEmpty() ? latest : "1.0.0";
                    entry.downloadUrl = pUrl;
                    entry.sha256 = pSha;
                    entry.changelog = pChangelog;

                    juce::String installed = InstalledRegistry::getInstalledVersion(id);
                    entry.updateAvailable = (installed.isNotEmpty() && InstalledRegistry::compareVersions(installed, entry.latestVersion) < 0);

                    {
                        const juce::ScopedLock sl(cacheLock);
                        pluginCatalogCache[id] = entry;
                    }
                }
            }
        }
    }
}

void PluggedINAutoUpdater::run()
{
    bool fetchedOnline = false;

    // 1. Permanent Cloud Endpoints — GitHub Raw (Instant with cache buster) + GitHub API + Edge CDN
    std::vector<juce::String> cloudEndpoints = {
        "https://raw.githubusercontent.com/dylangoodm2468590840/pluggedin-releases-/main/manifest.json",
        "https://api.github.com/repos/dylangoodm2468590840/pluggedin-releases-/contents/manifest.json",
        "https://cdn.jsdelivr.net/gh/dylangoodm2468590840/pluggedin-releases-@main/manifest.json"
    };

    int64_t nowSec = juce::Time::currentTimeMillis() / 1000;

    for (const auto& endpoint : cloudEndpoints)
    {
        juce::String urlWithCacheBuster = endpoint + (endpoint.containsChar('?') ? "&t=" : "?t=") + juce::String(nowSec);
        juce::URL versionUrl(urlWithCacheBuster);
        auto stream = versionUrl.createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                                                    .withConnectionTimeoutMs(5000)
                                                    .withHttpRequestCmd("GET")
                                                    .withExtraHeaders("User-Agent: PluggedIN-Central\r\nAccept: */*\r\nCache-Control: no-cache\r\nPragma: no-cache\r\n"));

        if (stream != nullptr)
        {
            juce::String responseText = stream->readEntireStreamAsString();
            if (responseText.isNotEmpty() && (responseText.contains("manager") || responseText.contains("hub_version") || responseText.contains("plugins")))
            {
                parseManifestJson(responseText);
                fetchedOnline = true;
                break;
            }
        }
    }

    // 2. Fallback / Offline / Local Distribution manifest
    if (!fetchedOnline)
    {
        juce::File appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
        juce::File currentExe = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
        juce::File appDir = currentExe.getParentDirectory();

        std::vector<juce::File> fallbackFiles = {
            appDataDir.getChildFile("PluggedIN\\manifest.json"),
            appDataDir.getChildFile("PluggedIN\\version.json"),
            appDir.getChildFile("manifest.json"),
            appDir.getChildFile("version.json"),
            appDir.getChildFile("dist\\manifest.json"),
            appDir.getChildFile("dist\\version.json"),
            appDir.getParentDirectory().getChildFile("dist\\version.json")
        };

        for (const auto& file : fallbackFiles)
        {
            if (file.existsAsFile())
            {
                juce::String fileText = file.loadFileAsString();
                if (fileText.isNotEmpty())
                {
                    parseManifestJson(fileText);
                    break;
                }
            }
        }
    }
}
