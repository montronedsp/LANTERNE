// SPDX-License-Identifier: AGPL-3.0-or-later
#include "PluginProcessor.h"
#include "PluginEditor.h"

LanterneProcessor::LanterneProcessor()
    : juce::AudioProcessor(
          BusesProperties()
              .withInput("Input",  juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    latestSnapshot.fft.resize(256, 0.0f);
}

LanterneProcessor::~LanterneProcessor() {}

void LanterneProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    audioAnalyzer.prepare(sampleRate, samplesPerBlock);
}

void LanterneProcessor::releaseResources() {}

void LanterneProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    audioAnalyzer.process(buffer);

    const juce::SpinLock::ScopedLockType lock(snapshotLock);
    latestSnapshot.rms    = audioAnalyzer.getRMS();
    latestSnapshot.bpm    = audioAnalyzer.getBPM();
    latestSnapshot.bass   = audioAnalyzer.getBand("bass");
    latestSnapshot.mid    = audioAnalyzer.getBand("mid");
    latestSnapshot.treble = audioAnalyzer.getBand("treble");
    audioAnalyzer.getFFTData(latestSnapshot.fft);
}

juce::AudioProcessorEditor* LanterneProcessor::createEditor()
{
    return new LanterneEditor(*this);
}

AudioSnapshot LanterneProcessor::getSnapshot()
{
    const juce::SpinLock::ScopedLockType lock(snapshotLock);
    return latestSnapshot;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LanterneProcessor();
}
