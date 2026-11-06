#include "AudioAnalyzer.h"

AudioAnalyzer::AudioAnalyzer() : fft(fftOrder) {}

void AudioAnalyzer::prepare(double sr, int maxBlockSize)
{
    sampleRate = sr;

    monoBuffer.assign(static_cast<size_t>(maxBlockSize), 0.0f);
    ringBuffer.assign(static_cast<size_t>(fftSize), 0.0f);
    fftBuffer.assign(static_cast<size_t>(fftSize * 2), 0.0f);
    window.resize(static_cast<size_t>(fftSize));
    fftFront.assign(static_cast<size_t>(fftSize / 2), 0.0f);
    fftBack.assign(static_cast<size_t>(fftSize / 2), 0.0f);

    juce::dsp::WindowingFunction<float>::fillWindowingTables(
        window.data(), static_cast<size_t>(fftSize),
        juce::dsp::WindowingFunction<float>::hann, true, 0.0f);

    windowSum = 0.0f;
    for (float w : window) windowSum += w;
    windowSum = std::max(windowSum, 1e-10f);

    writePos = 0;
    samplesSinceLastFFT = 0;

    {
        const juce::SpinLock::ScopedLockType lock(dataLock);
        rms = bass = mid = treble = 0.0f;
    }
}

void AudioAnalyzer::process(const juce::AudioBuffer<float>& buffer)
{
    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    if (numSamples <= 0 || numChannels <= 0) return;

    // --- Mono mix into pre-allocated monoBuffer ---
    // monoBuffer is sized to maxBlockSize in prepare(); clip safely if somehow larger.
    const int n = std::min(numSamples, static_cast<int>(monoBuffer.size()));

    const float* ch0 = buffer.getReadPointer(0);
    if (numChannels == 1)
    {
        std::copy(ch0, ch0 + n, monoBuffer.begin());
    }
    else
    {
        const float* ch1 = buffer.getReadPointer(1);
        for (int i = 0; i < n; ++i)
            monoBuffer[static_cast<size_t>(i)] = (ch0[i] + ch1[i]) * 0.5f;
        // For >2 channels, mix all:
        for (int c = 2; c < numChannels; ++c)
        {
            const float* chN = buffer.getReadPointer(c);
            const float inv = 1.0f / static_cast<float>(numChannels);
            for (int i = 0; i < n; ++i)
                monoBuffer[static_cast<size_t>(i)] =
                    monoBuffer[static_cast<size_t>(i)] * (1.0f - inv) + chN[i] * inv;
        }
    }

    // --- RMS ---
    float sumSq = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        float s = monoBuffer[static_cast<size_t>(i)];
        sumSq += s * s;
    }
    const float newRms = std::min(1.0f, std::sqrt(sumSq / static_cast<float>(n)));

    // --- Write into ring buffer and trigger FFT every hopSize samples ---
    for (int i = 0; i < n; ++i)
    {
        ringBuffer[static_cast<size_t>(writePos)] = monoBuffer[static_cast<size_t>(i)];
        writePos = (writePos + 1) % fftSize;
        ++samplesSinceLastFFT;

        if (samplesSinceLastFFT >= hopSize)
        {
            samplesSinceLastFFT = 0;
            runFFT();
        }
    }

    // --- Write RMS under lock ---
    {
        const juce::SpinLock::ScopedLockType lock(dataLock);
        rms = newRms;
    }
}

void AudioAnalyzer::runFFT()
{
    // Linearize ring buffer into fftBuffer starting from writePos
    // (writePos points to the oldest sample in the ring)
    for (int i = 0; i < fftSize; ++i)
    {
        int idx = (writePos + i) % fftSize;
        fftBuffer[static_cast<size_t>(i)] =
            ringBuffer[static_cast<size_t>(idx)] * window[static_cast<size_t>(i)];
    }
    // Zero the second half (FFT working space)
    std::fill(fftBuffer.begin() + fftSize, fftBuffer.end(), 0.0f);

    fft.performFrequencyOnlyForwardTransform(fftBuffer.data(), false);

    // Convert to dB-normalized 0..1, write into fftBack (fftSize/2 bins)
    const int numBins = fftSize / 2;
    for (int i = 0; i < numBins; ++i)
    {
        float magnitude = fftBuffer[static_cast<size_t>(i)] / windowSum;
        float db = 20.0f * std::log10(magnitude + 1e-10f);
        float normalized = (db - kDbFloor) / (kDbCeil - kDbFloor);
        fftBack[static_cast<size_t>(i)] = std::max(0.0f, std::min(1.0f, normalized));
    }

    // Compute bands from fftBack BEFORE swap (audio thread owns fftBack here)
    computeBands();

    // Swap front/back under lock
    {
        const juce::SpinLock::ScopedLockType lock(fftSwapLock);
        std::swap(fftFront, fftBack);
    }
}

void AudioAnalyzer::computeBands()
{
    // fftBack has the new FFT data; called before the swap, audio thread owns it
    const int numBins = fftSize / 2;

    auto avg = [&](int lo, int hi) -> float {
        lo = std::max(1, std::min(lo, numBins - 1));
        hi = std::max(lo, std::min(hi, numBins - 1));
        float sum = 0.0f;
        int count = hi - lo + 1;
        for (int i = lo; i <= hi; ++i)
            sum += fftBack[static_cast<size_t>(i)];
        return (count > 0) ? (sum / static_cast<float>(count)) : 0.0f;
    };

    float newBass   = avg(binForHz(60.0f),   binForHz(250.0f));
    float newMid    = avg(binForHz(250.0f),  binForHz(4000.0f));
    float newTreble = avg(binForHz(4000.0f), binForHz(16000.0f));

    {
        const juce::SpinLock::ScopedLockType lock(dataLock);
        bass   = newBass;
        mid    = newMid;
        treble = newTreble;
    }
}

int AudioAnalyzer::binForHz(float hz) const
{
    return static_cast<int>(hz * static_cast<float>(fftSize) / static_cast<float>(sampleRate));
}

float AudioAnalyzer::getRMS() const
{
    const juce::SpinLock::ScopedLockType lock(dataLock);
    return rms;
}

float AudioAnalyzer::getBPM() const
{
    return 0.0f;
}

void AudioAnalyzer::getFFTData(std::vector<float>& dest) const
{
    const juce::SpinLock::ScopedLockType lock(fftSwapLock);
    dest = fftFront;  // copies 256 floats under lock — safe, fast
}

float AudioAnalyzer::getBand(const std::string& band) const
{
    const juce::SpinLock::ScopedLockType lock(dataLock);
    if (band == "bass")   return bass;
    if (band == "mid")    return mid;
    if (band == "treble") return treble;
    return 0.0f;
}
