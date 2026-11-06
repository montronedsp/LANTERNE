#include "ROMWatcher.h"

ROMWatcher::ROMWatcher() {}

ROMWatcher::~ROMWatcher()
{
    stop();
}

void ROMWatcher::watch(const juce::File& folder, ChangeCallback callback)
{
    stopTimer();
    watchedFolder = folder;
    onChange = std::move(callback);
    snapshotFolder();
    startTimer(500);
}

void ROMWatcher::stop()
{
    stopTimer();
    watchedFolder = juce::File();
    onChange = nullptr;
    lastModified.clear();
}

void ROMWatcher::timerCallback()
{
    if (!watchedFolder.exists() || !watchedFolder.isDirectory())
        return;
    if (folderHasChanged())
    {
        snapshotFolder();
        if (onChange)
            onChange();
    }
}

void ROMWatcher::snapshotFolder()
{
    lastModified.clear();
    juce::Array<juce::File> files;
    watchedFolder.findChildFiles(files, juce::File::findFiles, true);
    for (const juce::File& f : files)
        lastModified[f.getFullPathName()] = f.getLastModificationTime();
}

bool ROMWatcher::folderHasChanged()
{
    juce::Array<juce::File> files;
    watchedFolder.findChildFiles(files, juce::File::findFiles, true);
    if (files.size() != static_cast<int>(lastModified.size()))
        return true;
    for (const juce::File& f : files)
    {
        auto it = lastModified.find(f.getFullPathName());
        if (it == lastModified.end() || it->second != f.getLastModificationTime())
            return true;
    }
    return false;
}
