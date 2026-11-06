// SPDX-License-Identifier: AGPL-3.0-or-later
#include "PluginSettings.h"

#if JUCE_WINDOWS
 #include <windows.h>
#endif

// ---------------------------------------------------------------------------
//  Helpers
// ---------------------------------------------------------------------------

float PluginSettings::bloomStrength() const
{
    switch (bloom)
    {
        case BloomLevel::Off:    return 0.0f;
        case BloomLevel::Low:    return 0.25f;
        case BloomLevel::Medium: return 0.44f;
        case BloomLevel::High:   return 0.70f;
    }
    return 0.44f;
}

int PluginSettings::frameCapToTimerMs() const
{
    switch (frameCap)
    {
        case FrameCapFPS::FPS30:     return 33;
        case FrameCapFPS::FPS60:     return 16;
        case FrameCapFPS::FPS120:    return 8;
        case FrameCapFPS::Uncapped:  return 4;
    }
    return 16;
}

// ---------------------------------------------------------------------------
//  Plugin binary location — the VST3 bundle's Contents/<arch> folder
// ---------------------------------------------------------------------------

juce::File PluginSettings::getPluginBinaryDirectory()
{
   #if JUCE_WINDOWS
    // Resolve the directory of the module that contains this code, i.e. the
    // Lanterne.vst3 DLL itself, not the hosting DAW executable.
    HMODULE module = nullptr;
    const auto address = reinterpret_cast<const void*>(&PluginSettings::getPluginBinaryDirectory);

    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCWSTR>(address), &module) && module != nullptr)
    {
        wchar_t buffer[MAX_PATH] = {};
        const DWORD length = GetModuleFileNameW(module, buffer, MAX_PATH);

        if (length > 0 && length < MAX_PATH)
            return juce::File(juce::String(buffer)).getParentDirectory();
    }
   #endif

    return {};
}

// ---------------------------------------------------------------------------
//  Persistence path – sits next to the plugin binary
// ---------------------------------------------------------------------------

juce::File PluginSettings::getSettingsFile()
{
    juce::File pluginDir = getPluginBinaryDirectory();

    if (! pluginDir.isDirectory())
        pluginDir = juce::File::getSpecialLocation(juce::File::currentApplicationFile)
                        .getParentDirectory();

    return pluginDir.getChildFile("Lanterne.settings");
}

// ---------------------------------------------------------------------------
//  Save
// ---------------------------------------------------------------------------

static juce::String aaToString(AAQuality q)
{
    switch (q) { case AAQuality::Off: return "off"; case AAQuality::SMAA: return "smaa"; case AAQuality::MSAA4x: return "msaa4x"; }
    return "smaa";
}

static juce::String bloomToString(BloomLevel b)
{
    switch (b) { case BloomLevel::Off: return "off"; case BloomLevel::Low: return "low"; case BloomLevel::Medium: return "medium"; case BloomLevel::High: return "high"; }
    return "medium";
}

static juce::String fpsToString(FrameCapFPS f)
{
    switch (f) { case FrameCapFPS::FPS30: return "30"; case FrameCapFPS::FPS60: return "60"; case FrameCapFPS::FPS120: return "120"; case FrameCapFPS::Uncapped: return "uncapped"; }
    return "60";
}

static juce::String payloadToString(FFTPayload p)
{
    switch (p) { case FFTPayload::Bands: return "bands"; case FFTPayload::FFT64: return "fft64"; case FFTPayload::FFT256: return "fft256"; }
    return "fft256";
}

void PluginSettings::save() const
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty("resolutionW",   resolutionW);
    obj->setProperty("resolutionH",   resolutionH);
    obj->setProperty("pixelRatio",    (double)pixelRatio);
    obj->setProperty("antialias",     aaToString(antialias));
    obj->setProperty("bloom",         bloomToString(bloom));
    obj->setProperty("particleScale", (double)particleScale);
    obj->setProperty("frameCap",      fpsToString(frameCap));
    obj->setProperty("fftPayload",    payloadToString(fftPayload));
    obj->setProperty("showFPS",       showFPS);

    juce::var root(obj);
    juce::String json = juce::JSON::toString(root, false);
    getSettingsFile().replaceWithText(json);
}

// ---------------------------------------------------------------------------
//  Load
// ---------------------------------------------------------------------------

static AAQuality parseAA(const juce::String& s)
{
    if (s == "off")    return AAQuality::Off;
    if (s == "msaa4x") return AAQuality::MSAA4x;
    return AAQuality::SMAA;
}

static BloomLevel parseBloom(const juce::String& s)
{
    if (s == "off")  return BloomLevel::Off;
    if (s == "low")  return BloomLevel::Low;
    if (s == "high") return BloomLevel::High;
    return BloomLevel::Medium;
}

static FrameCapFPS parseFPS(const juce::String& s)
{
    if (s == "30")        return FrameCapFPS::FPS30;
    if (s == "120")       return FrameCapFPS::FPS120;
    if (s == "uncapped")  return FrameCapFPS::Uncapped;
    return FrameCapFPS::FPS60;
}

static FFTPayload parsePayload(const juce::String& s)
{
    if (s == "bands") return FFTPayload::Bands;
    if (s == "fft64") return FFTPayload::FFT64;
    return FFTPayload::FFT256;
}

void PluginSettings::load()
{
    juce::File f = getSettingsFile();
    bool migrateLegacyFile = false;

    if (! f.existsAsFile())
    {
        // One-time migration: the pre-Lanterne build wrote VisualROM.settings
        // beside the plugin. Read it once and write it back as Lanterne.settings.
        juce::File legacy = f.getSiblingFile("VisualROM.settings");

        if (! legacy.existsAsFile())
            return;

        f = legacy;
        migrateLegacyFile = true;
    }

    juce::var parsed = juce::JSON::parse(f.loadFileAsString());
    if (!parsed.isObject()) return;

    auto* obj = parsed.getDynamicObject();
    if (!obj) return;

    if (obj->hasProperty("resolutionW"))   resolutionW   = (int)obj->getProperty("resolutionW");
    if (obj->hasProperty("resolutionH"))   resolutionH   = (int)obj->getProperty("resolutionH");
    if (obj->hasProperty("pixelRatio"))    pixelRatio    = (float)(double)obj->getProperty("pixelRatio");
    if (obj->hasProperty("antialias"))     antialias     = parseAA(obj->getProperty("antialias").toString());
    if (obj->hasProperty("bloom"))         bloom         = parseBloom(obj->getProperty("bloom").toString());
    if (obj->hasProperty("particleScale")) particleScale = (float)(double)obj->getProperty("particleScale");
    if (obj->hasProperty("frameCap"))      frameCap      = parseFPS(obj->getProperty("frameCap").toString());
    if (obj->hasProperty("fftPayload"))    fftPayload    = parsePayload(obj->getProperty("fftPayload").toString());
    if (obj->hasProperty("showFPS"))       showFPS       = (bool)obj->getProperty("showFPS");

    if (migrateLegacyFile)
        save();
}
