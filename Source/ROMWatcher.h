#pragma once
#include <JuceHeader.h>
#include <functional>
#include <map>

/// Watches a directory for file changes and calls a callback on any modification.
/// Uses polling (manual mtime tracking) for simplicity and cross-platform safety.
class ROMWatcher : public juce::Timer
{
public:
    /// Callback is called on the message thread when any file in the watched folder changes.
    using ChangeCallback = std::function<void()>;

    ROMWatcher();
    ~ROMWatcher() override;

    /// Start watching a folder. Replaces any previous watch.
    void watch(const juce::File& folder, ChangeCallback callback);

    /// Stop watching.
    void stop();

private:
    void timerCallback() override;

    juce::File      watchedFolder;
    ChangeCallback  onChange;

    /// Map of filename → last modified time, used to detect changes.
    std::map<juce::String, juce::Time> lastModified;

    void snapshotFolder();
    bool folderHasChanged();
};
