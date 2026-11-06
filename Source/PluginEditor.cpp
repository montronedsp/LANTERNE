// SPDX-License-Identifier: AGPL-3.0-or-later
#include "PluginEditor.h"

static juce::String escapeForJS(const juce::String& s)
{
    juce::String out;
    for (juce::juce_wchar c : s)
    {
        if (c == '\\') out << "\\\\";
        else if (c == '\'') out << "\\'";
        else out << c;
    }
    return out;
}

static juce::String aaQualityToString(AAQuality q)
{
    switch (q) { case AAQuality::Off: return "off"; case AAQuality::SMAA: return "smaa"; case AAQuality::MSAA4x: return "msaa4x"; }
    return "smaa";
}

static juce::String bloomLevelToString(BloomLevel b)
{
    switch (b) { case BloomLevel::Off: return "off"; case BloomLevel::Low: return "low"; case BloomLevel::Medium: return "medium"; case BloomLevel::High: return "high"; }
    return "medium";
}

juce::File LanterneEditor::getROMsRoot()
{
    // Primary: the ROMs folder is copied next to the plugin binary at build
    // time, so it travels inside the VST3 bundle.
    const juce::File binaryDir = PluginSettings::getPluginBinaryDirectory();

    if (binaryDir.isDirectory())
    {
        const juce::File bundled = binaryDir.getChildFile("ROMs");
        if (bundled.exists() && bundled.isDirectory())
            return bundled;
    }

    // Dev fallback: the ROMs folder of this source tree, used when the plugin
    // runs without a bundle copy (for example straight out of a build folder).
    juce::File devRoot = juce::File(__FILE__).getParentDirectory().getParentDirectory();
    const juce::File fromSource = devRoot.getChildFile("ROMs");
    if (fromSource.exists() && fromSource.isDirectory())
        return fromSource;

    return juce::File();
}

LanterneEditor::LanterneEditor(LanterneProcessor& p)
    : juce::AudioProcessorEditor(p),
      processor(p),
      webBrowser(juce::WebBrowserComponent::Options{}.withBackend(juce::WebBrowserComponent::Options::Backend::webview2))
{
    settings.load();
    setSize(settings.resolutionW, settings.resolutionH + 40);
    setResizable(false, false);

    // Enable right-click on the editor itself
    addMouseListener(this, true);

    juce::File romsRoot = getROMsRoot();
    romRegistry.scan(romsRoot);

    prevButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff404040));
    prevButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    prevButton.onClick = [this]
    {
        if (romRegistry.size() == 0) return;
        activeROMIndex = (activeROMIndex - 1 + romRegistry.size()) % romRegistry.size();
        loadROM(activeROMIndex);
    };
    addAndMakeVisible(prevButton);

    nextButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff404040));
    nextButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    nextButton.onClick = [this]
    {
        if (romRegistry.size() == 0) return;
        activeROMIndex = (activeROMIndex + 1) % romRegistry.size();
        loadROM(activeROMIndex);
    };
    addAndMakeVisible(nextButton);

    romLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff2a2a2a));
    romLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    romLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(romLabel);

    addAndMakeVisible(webBrowser);

    loadROM(0);
    startTimer(settings.frameCapToTimerMs());
}

LanterneEditor::~LanterneEditor()
{
    stopTimer();
    romWatcher.stop();
}

void LanterneEditor::resized()
{
    auto w = getWidth();
    auto h = getHeight();

    prevButton.setBounds(0, 0, 40, 40);
    nextButton.setBounds(w - 40, 0, 40, 40);
    romLabel.setBounds(40, 0, w - 80, 40);
    webBrowser.setBounds(0, 40, w, h - 40);
}

void LanterneEditor::loadROM(int index)
{
    metaInjected = false;
    settingsInjected = false;
    romWatcher.stop();

    if (romRegistry.size() == 0)
    {
        romLabel.setText("No ROMs found", juce::dontSendNotification);
        webBrowser.goToURL("about:blank");
        return;
    }

    index = juce::jlimit(0, romRegistry.size() - 1, index);
    activeROMIndex = index;

    const ROMManifest& rom = romRegistry.getROMs()[static_cast<size_t>(index)];
    romLabel.setText(rom.name + "  \xe2\x80\x94  " + rom.author, juce::dontSendNotification);
    webBrowser.goToURL(juce::URL(rom.indexFile).toString(false));

    romWatcher.watch(rom.folder, [this]()
    {
        juce::MessageManager::callAsync([this]()
        {
            loadROM(activeROMIndex);
        });
    });
}

void LanterneEditor::timerCallback()
{
    AudioSnapshot snapshot = processor.getSnapshot();
    juce::String json = buildJSON(snapshot);
    webBrowser.evaluateJavascript("window.__ROMReceiveAudio(" + json + ")");

    if (!metaInjected && romRegistry.size() > 0)
    {
        const ROMManifest& rom = romRegistry.getROMs()[static_cast<size_t>(activeROMIndex)];
        juce::String metaJS = "window.__ROMMeta = { name: '" + escapeForJS(rom.name) + "', "
                             "author: '" + escapeForJS(rom.author) + "', "
                             "version: '" + escapeForJS(rom.version) + "', "
                             "api: '" + escapeForJS(rom.api) + "' };";
        webBrowser.evaluateJavascript(metaJS);
        metaInjected = true;
    }

    if (!settingsInjected)
    {
        webBrowser.evaluateJavascript(buildSettingsJS());
        settingsInjected = true;
    }
}

juce::String LanterneEditor::buildJSON(const AudioSnapshot& snap)
{
    auto* root = new juce::DynamicObject();
    root->setProperty("rms", snap.rms);
    root->setProperty("bpm", snap.bpm);

    // FFT payload based on settings
    if (settings.fftPayload != FFTPayload::Bands)
    {
        juce::Array<juce::var> fftArray;
        int count = (settings.fftPayload == FFTPayload::FFT64) ? 64 : 256;
        int step  = (int)snap.fft.size() / count;
        if (step < 1) step = 1;

        for (int i = 0; i < count && i * step < (int)snap.fft.size(); ++i)
        {
            float sum = 0.0f;
            int n = 0;
            for (int j = 0; j < step && (i * step + j) < (int)snap.fft.size(); ++j)
            {
                sum += snap.fft[static_cast<size_t>(i * step + j)];
                ++n;
            }
            fftArray.add(n > 0 ? sum / n : 0.0f);
        }
        root->setProperty("fft", juce::var(fftArray));
    }

    auto* bands = new juce::DynamicObject();
    bands->setProperty("bass", snap.bass);
    bands->setProperty("mid", snap.mid);
    bands->setProperty("treble", snap.treble);
    root->setProperty("bands", juce::var(bands));

    juce::var rootVar(root);
    return juce::JSON::toString(rootVar, false);
}

juce::String LanterneEditor::buildSettingsJS()
{
    juce::String js = "window.__ROMSettings = { "
        "pixelRatio: "     + juce::String(settings.pixelRatio, 1) + ", "
        "antialias: '"     + aaQualityToString(settings.antialias) + "', "
        "bloom: '"         + bloomLevelToString(settings.bloom) + "', "
        "bloomStrength: "  + juce::String(settings.bloomStrength(), 2) + ", "
        "particleScale: "  + juce::String(settings.particleScale, 1) + ", "
        "showFPS: "        + juce::String(settings.showFPS ? "true" : "false") + " };";

    js += " if (typeof window.__ROMOnSettings === 'function') window.__ROMOnSettings(window.__ROMSettings);";
    return js;
}

// ---------------------------------------------------------------------------
//  Right-click -> Settings Panel
// ---------------------------------------------------------------------------

void LanterneEditor::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        juce::PopupMenu menu;
        menu.addItem(1, "Settings...");
        menu.showMenuAsync(juce::PopupMenu::Options(),
            [this](int result)
            {
                if (result == 1)
                    openSettingsPanel();
            });
    }
}

void LanterneEditor::openSettingsPanel()
{
    if (settingsWindow != nullptr)
    {
        settingsWindow->toFront(true);
        return;
    }

    settingsWindow = new SettingsWindow(settings, this);

    settingsWindow->onApply = [this](const PluginSettings& s)
    {
        settingsWindow = nullptr;  // clear dangling ptr before apply triggers resize
        applySettings(s);
    };

    settingsWindow->onCancel = [this]()
    {
        settingsWindow = nullptr;
    };
}

void LanterneEditor::closeSettingsPanel()
{
    // Window is self-owned and self-destructs.
    // This function kept for call-site compatibility but does nothing now.
    // The window closes itself via its onApply/onCancel callbacks.
    settingsWindow = nullptr;
}

void LanterneEditor::applySettings(const PluginSettings& newSettings)
{
    settings = newSettings;
    settings.save();

    closeSettingsPanel();

    // Resize if resolution changed
    setSize(settings.resolutionW, settings.resolutionH + 40);

    // Restart timer with new interval
    stopTimer();
    startTimer(settings.frameCapToTimerMs());

    // Re-inject settings and reload ROM
    settingsInjected = false;
    loadROM(activeROMIndex);
}
