#pragma once
// SPDX-License-Identifier: AGPL-3.0-or-later
#include <JuceHeader.h>

/// Antialiasing quality level.
enum class AAQuality { Off, SMAA, MSAA4x };

/// Bloom intensity preset.
enum class BloomLevel { Off, Low, Medium, High };

/// Frame-rate cap (FPS).
enum class FrameCapFPS { FPS30, FPS60, FPS120, Uncapped };

/// JSON payload size sent to ROMs each timer tick.
enum class FFTPayload { Bands, FFT64, FFT256 };

/// All user-facing settings, persisted to disk.
struct PluginSettings
{
    // -- Display --
    int  resolutionW   = 800;
    int  resolutionH   = 600;
    float pixelRatio   = 1.0f;   // 1.0 or 2.0

    // -- Rendering --
    AAQuality    antialias      = AAQuality::SMAA;
    BloomLevel   bloom          = BloomLevel::Medium;
    float        particleScale  = 1.0f;  // 0.5, 1.0, 2.0

    // -- Engine --
    FrameCapFPS  frameCap       = FrameCapFPS::FPS60;
    FFTPayload   fftPayload     = FFTPayload::FFT256;
    bool         showFPS        = false;

    // Persistence
    void save() const;
    void load();

    // Helpers
    float bloomStrength() const;
    int   frameCapToTimerMs() const;

    static juce::File getSettingsFile();

    /// Directory containing the plugin binary (the VST3 bundle's Contents/<arch> folder).
    /// Empty file if it cannot be determined.
    static juce::File getPluginBinaryDirectory();
};
