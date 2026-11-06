#pragma once
// SPDX-License-Identifier: AGPL-3.0-or-later
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "PluginSettings.h"
#include "SettingsPanel.h"
#include "ROMRegistry.h"
#include "ROMWatcher.h"

// ─── SettingsWindow ───────────────────────────────────────────────────────────
// A self-owned native OS window hosting SettingsPanel.
// Floats above WebBrowserComponent in the Win32 layer.
// Deletes itself on close — never delete externally.
class SettingsWindow : public juce::DocumentWindow
{
public:
    std::function<void(const PluginSettings&)> onApply;
    std::function<void()>                      onCancel;

    SettingsWindow(const PluginSettings& current, juce::Component* centreOver)
        : juce::DocumentWindow(
              "Lanterne  //  ENGINE SETTINGS",
              juce::Colour(0xff0a0a0a),
              0,      // no native title bar buttons — panel has its own controls
              true)   // addToDesktop: true — gives this window its own native HWND
    {
        panel = std::make_unique<SettingsPanel>(
            current,
            [this](const PluginSettings& s) { if (onApply) onApply(s); selfDestruct(); },
            [this]()                         { if (onCancel) onCancel(); selfDestruct(); }
        );

        setContentOwned(panel.release(), true);
        setResizable(false, false);
        setUsingNativeTitleBar(false);
        setTitleBarHeight(0);
        setVisible(true);
        setAlwaysOnTop(true);

        if (centreOver != nullptr)
            centreAroundComponent(centreOver, getWidth(), getHeight());
        else
            centreWithSize(getWidth(), getHeight());
    }

    ~SettingsWindow() override = default;

    void closeButtonPressed() override
    {
        if (onCancel) onCancel();
        selfDestruct();
    }

private:
    std::unique_ptr<SettingsPanel> panel;

    void selfDestruct()
    {
        // Never delete 'this' synchronously inside a callback chain.
        juce::MessageManager::callAsync([this]() { delete this; });
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsWindow)
};

/// Plugin editor window. Hosts a ROM selector bar and WebBrowserComponent that renders the active ROM.
class LanterneEditor : public juce::AudioProcessorEditor,
                        private juce::Timer
{
public:
    explicit LanterneEditor(LanterneProcessor&);
    ~LanterneEditor() override;

    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;

private:
    void timerCallback() override;
    juce::String buildJSON(const AudioSnapshot& snap);
    juce::String buildSettingsJS();
    juce::File getROMsRoot();
    void loadROM(int index);
    void openSettingsPanel();
    void closeSettingsPanel();
    void applySettings(const PluginSettings& newSettings);

    LanterneProcessor&        processor;
    juce::WebBrowserComponent  webBrowser;
    ROMRegistry                 romRegistry;
    ROMWatcher                  romWatcher;
    int                         activeROMIndex = 0;
    bool                        metaInjected   = false;
    bool                        settingsInjected = false;

    PluginSettings              settings;
    SettingsWindow*             settingsWindow = nullptr;  // raw ptr — window owns itself

    juce::TextButton prevButton { "<" };
    juce::TextButton nextButton { ">" };
    juce::Label      romLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LanterneEditor)
};
