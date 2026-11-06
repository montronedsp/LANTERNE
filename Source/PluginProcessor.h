#pragma once
// SPDX-License-Identifier: AGPL-3.0-or-later
#include <JuceHeader.h>
#include "AudioAnalyzer.h"
#include <vector>

/// Snapshot of audio analysis data, safe to copy across threads.
struct AudioSnapshot
{
    float rms    = 0.0f;
    float bpm    = 0.0f;
    float bass   = 0.0f;
    float mid    = 0.0f;
    float treble = 0.0f;
    std::vector<float> fft;   // 256 bins (fftSize/2), dB-normalized 0–1
};

/// Main VST3 plugin processor. Passes audio through unmodified and feeds AudioAnalyzer.
class LanterneProcessor : public juce::AudioProcessor
{
public:
    LanterneProcessor();
    ~LanterneProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Lanterne"; }
    bool acceptsMidi()  const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

    /// Thread-safe copy of latest analysis data for the editor to poll.
    AudioSnapshot getSnapshot();

private:
    AudioAnalyzer audioAnalyzer;
    juce::SpinLock snapshotLock;
    AudioSnapshot latestSnapshot;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LanterneProcessor)
};
