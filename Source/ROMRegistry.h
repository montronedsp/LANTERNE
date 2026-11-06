#pragma once
#include <JuceHeader.h>
#include <vector>
#include <string>

/// Metadata parsed from a ROM's rom.json manifest.
struct ROMManifest
{
    juce::String name;       // "name" field
    juce::String author;     // "author" field
    juce::String version;    // "version" field
    juce::String entry;      // "entry" field — filename of JS entry point (e.g. "main.js")
    juce::String api;        // "api" field
    juce::File   folder;     // Absolute path to the ROM folder
    juce::File   indexFile;  // Absolute path to index.html
};

/// Scans a root ROMs directory and exposes all valid ROM manifests.
class ROMRegistry
{
public:
    ROMRegistry();

    /// Scan romsRoot for subdirectories containing a valid rom.json and index.html.
    /// Clears and repopulates the internal list. Call once on startup and again after install.
    void scan(const juce::File& romsRoot);

    /// Returns all discovered ROMs in scan order.
    const std::vector<ROMManifest>& getROMs() const;

    /// Returns the ROM whose folder name matches id, or nullptr if not found.
    const ROMManifest* findByFolderName(const juce::String& folderName) const;

    /// Returns the number of ROMs found.
    int size() const;

private:
    std::vector<ROMManifest> roms;

    /// Returns true if the folder contains a parseable rom.json and an index.html.
    bool isValidROM(const juce::File& folder, ROMManifest& outManifest) const;
};
