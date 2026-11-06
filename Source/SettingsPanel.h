#pragma once
#include <JuceHeader.h>
#include "PluginSettings.h"

/// Game-style settings overlay panel with dark UI and cyan accents.
class SettingsPanel : public juce::Component
{
public:
    SettingsPanel(const PluginSettings& current,
                  std::function<void(const PluginSettings&)> onApply,
                  std::function<void()> onCancel);

    void paint(juce::Graphics& g) override;
    void resized() override;

    /// Consume mouse clicks so they don't pass through.
    void mouseDown(const juce::MouseEvent&) override {}

private:
    struct OptionRow
    {
        juce::String label;
        std::vector<juce::String> options;
        int selected = 0;
        std::vector<std::unique_ptr<juce::TextButton>> buttons;
    };

    void buildUI();
    void addEnumRow(const juce::String& label, const std::vector<juce::String>& options, int selected);
    void refreshRow(OptionRow& row);
    void layoutRow(OptionRow& row, int y);

    PluginSettings editedSettings;
    std::function<void(const PluginSettings&)> applyCallback;
    std::function<void()> cancelCallback;

    std::vector<OptionRow> rows;

    juce::TextButton applyButton { "APPLY & RESTART ROM" };
    juce::TextButton cancelButton { "CANCEL" };

    juce::Colour bgDark     { 0xff0d0d0d };
    juce::Colour panelBg    { 0xff1a1a1a };
    juce::Colour accent     { 0xff00ffc8 };
    juce::Colour accentDim  { 0xff007a61 };
    juce::Colour textCol    { 0xffe0e0e0 };

    static constexpr int panelW = 560;
    static constexpr int panelH = 460;
    static constexpr int rowHeight = 32;
    static constexpr int rowGap = 4;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsPanel)
};
