#include "ROMRegistry.h"
#include <algorithm>

ROMRegistry::ROMRegistry() {}

void ROMRegistry::scan(const juce::File& romsRoot)
{
    roms.clear();
    if (!romsRoot.exists() || !romsRoot.isDirectory())
        return;

    juce::Array<juce::File> dirs;
    romsRoot.findChildFiles(dirs, juce::File::findDirectories, false);

    for (const juce::File& dir : dirs)
    {
        ROMManifest manifest;
        if (isValidROM(dir, manifest))
            roms.push_back(manifest);
    }

    std::sort(roms.begin(), roms.end(),
              [](const ROMManifest& a, const ROMManifest& b) { return a.name.compareNatural(b.name) < 0; });
}

const std::vector<ROMManifest>& ROMRegistry::getROMs() const
{
    return roms;
}

const ROMManifest* ROMRegistry::findByFolderName(const juce::String& folderName) const
{
    for (const ROMManifest& m : roms)
        if (m.folder.getFileName() == folderName)
            return &m;
    return nullptr;
}

int ROMRegistry::size() const
{
    return static_cast<int>(roms.size());
}

bool ROMRegistry::isValidROM(const juce::File& folder, ROMManifest& outManifest) const
{
    juce::File romJson = folder.getChildFile("rom.json");
    juce::File indexHtml = folder.getChildFile("index.html");
    if (!romJson.existsAsFile() || !indexHtml.existsAsFile())
        return false;

    juce::var root = juce::JSON::parse(romJson.loadFileAsString());
    if (!root.isObject())
        return false;

    outManifest.name    = root.getProperty("name",    juce::var()).toString();
    outManifest.author  = root.getProperty("author",  juce::var()).toString();
    outManifest.version = root.getProperty("version", juce::var()).toString();
    outManifest.entry   = root.getProperty("entry",   juce::var()).toString();
    outManifest.api     = root.getProperty("api",     juce::var()).toString();

    if (outManifest.entry.isEmpty())
        outManifest.entry = "main.js";

    outManifest.folder    = folder;
    outManifest.indexFile = indexHtml;

    return outManifest.name.isNotEmpty();
}
