#include "SettingsPanel.h"

// ---------------------------------------------------------------------------
//  Construction
// ---------------------------------------------------------------------------

SettingsPanel::SettingsPanel(const PluginSettings& current,
                             std::function<void(const PluginSettings&)> onApply,
                             std::function<void()> onCancel)
    : editedSettings(current),
      applyCallback(std::move(onApply)),
      cancelCallback(std::move(onCancel))
{
    setInterceptsMouseClicks(true, true);
    buildUI();
    setSize(580, 500);
}

// ---------------------------------------------------------------------------
//  Build rows
// ---------------------------------------------------------------------------

void SettingsPanel::buildUI()
{
    // -- Display --
    {
        int sel = 0;
        if (editedSettings.resolutionW == 1024) sel = 1;
        else if (editedSettings.resolutionW == 1280) sel = 2;
        addEnumRow("Resolution", { "800x600", "1024x768", "1280x960" }, sel);
    }
    {
        int sel = editedSettings.pixelRatio >= 1.5f ? 1 : 0;
        addEnumRow("Pixel Ratio", { "1x", "2x" }, sel);
    }

    // -- Rendering --
    {
        int sel = 0;
        if (editedSettings.antialias == AAQuality::SMAA) sel = 1;
        else if (editedSettings.antialias == AAQuality::MSAA4x) sel = 2;
        addEnumRow("Antialiasing", { "Off", "SMAA", "MSAA 4x" }, sel);
    }
    {
        int sel = 0;
        if (editedSettings.bloom == BloomLevel::Low) sel = 1;
        else if (editedSettings.bloom == BloomLevel::Medium) sel = 2;
        else if (editedSettings.bloom == BloomLevel::High) sel = 3;
        addEnumRow("Bloom", { "Off", "Low", "Medium", "High" }, sel);
    }
    {
        int sel = 1;
        if (editedSettings.particleScale < 0.75f) sel = 0;
        else if (editedSettings.particleScale > 1.5f) sel = 2;
        addEnumRow("Particle Count", { "0.5x", "1x", "2x" }, sel);
    }

    // -- Engine --
    {
        int sel = 1;
        if (editedSettings.frameCap == FrameCapFPS::FPS30) sel = 0;
        else if (editedSettings.frameCap == FrameCapFPS::FPS120) sel = 2;
        else if (editedSettings.frameCap == FrameCapFPS::Uncapped) sel = 3;
        addEnumRow("Frame Rate Cap", { "30", "60", "120", "Uncap" }, sel);
    }
    {
        int sel = 2;
        if (editedSettings.fftPayload == FFTPayload::Bands) sel = 0;
        else if (editedSettings.fftPayload == FFTPayload::FFT64) sel = 1;
        addEnumRow("JSON Payload", { "Bands", "FFT-64", "FFT-256" }, sel);
    }
    {
        int sel = editedSettings.showFPS ? 1 : 0;
        addEnumRow("Show FPS", { "Off", "On" }, sel);
    }

    // -- Apply / Cancel buttons --
    applyButton.setColour(juce::TextButton::buttonColourId, accent.withAlpha(0.25f));
    applyButton.setColour(juce::TextButton::textColourOffId, accent);
    applyButton.onClick = [this]
    {
        // Map row selections back to editedSettings
        // Row 0: Resolution
        switch (rows[0].selected)
        {
            case 0: editedSettings.resolutionW = 800;  editedSettings.resolutionH = 600; break;
            case 1: editedSettings.resolutionW = 1024; editedSettings.resolutionH = 768; break;
            case 2: editedSettings.resolutionW = 1280; editedSettings.resolutionH = 960; break;
        }
        // Row 1: Pixel Ratio
        editedSettings.pixelRatio = rows[1].selected == 0 ? 1.0f : 2.0f;
        // Row 2: Antialiasing
        editedSettings.antialias = static_cast<AAQuality>(rows[2].selected);
        // Row 3: Bloom
        editedSettings.bloom = static_cast<BloomLevel>(rows[3].selected);
        // Row 4: Particle Scale
        { const float scales[] = { 0.5f, 1.0f, 2.0f }; editedSettings.particleScale = scales[rows[4].selected]; }
        // Row 5: Frame Cap
        editedSettings.frameCap = static_cast<FrameCapFPS>(rows[5].selected);
        // Row 6: FFT Payload
        editedSettings.fftPayload = static_cast<FFTPayload>(rows[6].selected);
        // Row 7: Show FPS
        editedSettings.showFPS = rows[7].selected == 1;

        if (applyCallback) applyCallback(editedSettings);
    };
    addAndMakeVisible(applyButton);

    cancelButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff333333));
    cancelButton.setColour(juce::TextButton::textColourOffId, textCol);
    cancelButton.onClick = [this]
    {
        if (cancelCallback) cancelCallback();
    };
    addAndMakeVisible(cancelButton);
}

// ---------------------------------------------------------------------------
//  addEnumRow / refreshRow
// ---------------------------------------------------------------------------

void SettingsPanel::addEnumRow(const juce::String& label,
                                const std::vector<juce::String>& options,
                                int selected)
{
    OptionRow row;
    row.label    = label;
    row.options  = options;
    row.selected = selected;

    int rowIdx = static_cast<int>(rows.size());

    for (int i = 0; i < (int)options.size(); ++i)
    {
        auto btn = std::make_unique<juce::TextButton>(options[static_cast<size_t>(i)]);
        btn->setClickingTogglesState(false);

        int optIdx = i;
        btn->onClick = [this, rowIdx, optIdx]
        {
            rows[static_cast<size_t>(rowIdx)].selected = optIdx;
            refreshRow(rows[static_cast<size_t>(rowIdx)]);
        };

        addAndMakeVisible(*btn);
        row.buttons.push_back(std::move(btn));
    }

    rows.push_back(std::move(row));
    refreshRow(rows.back());
}

void SettingsPanel::refreshRow(OptionRow& row)
{
    for (int i = 0; i < (int)row.buttons.size(); ++i)
    {
        bool active = (i == row.selected);
        row.buttons[static_cast<size_t>(i)]->setColour(juce::TextButton::buttonColourId,
            active ? accent.withAlpha(0.3f) : juce::Colour(0xff2a2a2a));
        row.buttons[static_cast<size_t>(i)]->setColour(juce::TextButton::textColourOffId,
            active ? accent : textCol.withAlpha(0.5f));
    }
}

// ---------------------------------------------------------------------------
//  Paint
// ---------------------------------------------------------------------------

void SettingsPanel::paint(juce::Graphics& g)
{
    // Dark overlay background
    g.fillAll(bgDark.withAlpha(0.85f));

    // Centered panel
    auto area = getLocalBounds();
    auto panel = area.withSizeKeepingCentre(panelW, panelH);

    g.setColour(panelBg);
    g.fillRoundedRectangle(panel.toFloat(), 6.0f);

    g.setColour(accent.withAlpha(0.3f));
    g.drawRoundedRectangle(panel.toFloat().reduced(0.5f), 6.0f, 1.0f);

    // Title
    g.setColour(accent);
    g.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 16.0f, juce::Font::bold));
    g.drawText("SETTINGS", panel.removeFromTop(40).toFloat(), juce::Justification::centred);

    // Row labels
    g.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain));
    int yOff = getLocalBounds().withSizeKeepingCentre(panelW, panelH).getY() + 44;

    for (auto& row : rows)
    {
        g.setColour(textCol);
        g.drawText(row.label,
                   getLocalBounds().withSizeKeepingCentre(panelW, panelH).getX() + 16,
                   yOff, 140, rowHeight,
                   juce::Justification::centredLeft);
        yOff += rowHeight + rowGap;
    }
}

// ---------------------------------------------------------------------------
//  Layout
// ---------------------------------------------------------------------------

void SettingsPanel::resized()
{
    auto area = getLocalBounds();
    auto panel = area.withSizeKeepingCentre(panelW, panelH);
    int yOff = panel.getY() + 44;

    for (auto& row : rows)
    {
        layoutRow(row, yOff);
        yOff += rowHeight + rowGap;
    }

    // Buttons at bottom of panel
    int btnY = panel.getBottom() - 50;
    int btnW = 180;
    int gap  = 16;
    int totalW = btnW * 2 + gap;
    int startX = panel.getCentreX() - totalW / 2;

    applyButton.setBounds(startX, btnY, btnW, 34);
    cancelButton.setBounds(startX + btnW + gap, btnY, btnW, 34);
}

void SettingsPanel::layoutRow(OptionRow& row, int y)
{
    auto panel = getLocalBounds().withSizeKeepingCentre(panelW, panelH);
    int btnX = panel.getX() + 170; // after label area
    int btnW = (panelW - 170 - 20) / juce::jmax(1, (int)row.buttons.size()) - 4;

    for (int i = 0; i < (int)row.buttons.size(); ++i)
    {
        row.buttons[static_cast<size_t>(i)]->setBounds(btnX, y, btnW, rowHeight - 2);
        btnX += btnW + 4;
    }
}
