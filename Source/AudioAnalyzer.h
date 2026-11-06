#pragma once
#include <JuceHeader.h>
#include <vector>
#include <string>
#include <atomic>

/// Professional audio analyzer: ring-buffer STFT with 50% overlap,
/// dB-normalized FFT output, Hz-based frequency bands, thread-safe double buffering.
class AudioAnalyzer
{
public:
    AudioAnalyzer();

    /// Call once before playback begins. Allocates all buffers; no allocation after this.
    void prepare(double sampleRate, int maxBlockSize);

    /// Call every processBlock. No allocation. Audio thread only.
    void process(const juce::AudioBuffer<float>& buffer);

    /// Thread-safe. Returns smoothed RMS, 0.0–1.0.
    float getRMS() const;

    /// Stubbed. Returns 0.0f.
    float getBPM() const;

    /// Thread-safe. Returns the latest dB-normalized FFT magnitude bins, 0.0–1.0.
    /// 256 bins (fftSize/2). Safe to call from the editor thread.
    void getFFTData(std::vector<float>& dest) const;

    /// Thread-safe. Returns averaged magnitude for "bass", "mid", or "treble".
    float getBand(const std::string& band) const;

private:
    // --- Configuration ---
    static constexpr int   fftOrder  = 9;
    static constexpr int   fftSize   = 1 << fftOrder;   // 512
    static constexpr int   hopSize   = fftSize / 2;      // 256
    static constexpr float kDbFloor  = -80.0f;
    static constexpr float kDbCeil   =   0.0f;

    // --- JUCE DSP ---
    juce::dsp::FFT fft;

    // --- Pre-allocated buffers (no allocation in process()) ---
    std::vector<float> monoBuffer;   // max block size, pre-sized in prepare()
    std::vector<float> ringBuffer;   // fftSize, circular accumulator
    std::vector<float> fftBuffer;    // fftSize * 2, working buffer for FFT
    std::vector<float> window;       // fftSize, Hann coefficients
    float              windowSum = 1.0f;

    // --- Ring buffer state ---
    int writePos            = 0;
    int samplesSinceLastFFT = 0;

    // --- Double-buffered FFT output ---
    std::vector<float>    fftFront;       // read by editor thread
    std::vector<float>    fftBack;        // written by audio thread
    mutable juce::SpinLock fftSwapLock;

    // --- Analysis results (protected by dataLock) ---
    float rms    = 0.0f;
    float bass   = 0.0f;
    float mid    = 0.0f;
    float treble = 0.0f;
    mutable juce::SpinLock dataLock;

    // --- Sample rate (set in prepare, read in computeBands) ---
    double sampleRate = 44100.0;

    // --- Internal ---
    void runFFT();
    void computeBands();
    int  binForHz(float hz) const;
};
