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

bool PluggedINAutoUpdater::isPluginUpdateAvailable(const juce::String& pluginId) const noexcept
{
    if (pluginId == "pluggedin_underground")
        return undergroundUpdateAvailable.load();
    return false;
}

juce::String PluggedINAutoUpdater::getPluginLatestVersion(const juce::String& pluginId) const noexcept
{
    if (pluginId == "pluggedin_underground")
        return undergroundLatestVersion;
    return "1.0.0";
}

juce::String PluggedINAutoUpdater::getPluginDownloadUrl(const juce::String& pluginId) const noexcept
{
    if (pluginId == "pluggedin_underground")
        return undergroundDownloadUrl;
    return "";
}

juce::String PluggedINAutoUpdater::getPluginSha256(const juce::String& pluginId) const noexcept
{
    if (pluginId == "pluggedin_underground")
        return undergroundSha256;
    return "";
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
                    if (pkgObj->hasProperty("windows_x64"))
                    {
                        auto winVar = pkgObj->getProperty("windows_x64");
                        if (auto* winObj = winVar.getDynamicObject())
                        {
                            downloadUrl = winObj->getProperty("url").toString();
                            managerSha256 = winObj->getProperty("sha256").toString();
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

                    juce::String latest = pObj->getProperty("latest_version").toString();

                    if (id == "pluggedin_underground")
                    {
                        undergroundLatestVersion = latest;

                        if (pObj->hasProperty("packages"))
                        {
                            auto pkgVar = pObj->getProperty("packages");
                            if (auto* pkgObj = pkgVar.getDynamicObject())
                            {
                                if (pkgObj->hasProperty("windows_x64"))
                                {
                                    auto winVar = pkgObj->getProperty("windows_x64");
                                    if (auto* winObj = winVar.getDynamicObject())
                                    {
                                        undergroundDownloadUrl = winObj->getProperty("url").toString();
                                        undergroundSha256 = winObj->getProperty("sha256").toString();
                                    }
                                }
                            }
                        }
                        else if (pObj->hasProperty("download_url_win"))
                        {
                            undergroundDownloadUrl = pObj->getProperty("download_url_win").toString();
                        }

                        juce::String installed = InstalledRegistry::getInstalledVersion(id);
                        bool needsPluginUpdate = (installed.isNotEmpty() && InstalledRegistry::compareVersions(installed, latest) < 0);
                        undergroundUpdateAvailable.store(needsPluginUpdate);
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
            appDir.getParentDirectory().getChildFile("dist\\version.json"),
            juce::File("C:\\Users\\dylan\\Documents\\First plug in\\dist\\version.json"),
            juce::File("C:\\Users\\dylan\\Documents\\First plug in\\dist\\manifest.json")
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
