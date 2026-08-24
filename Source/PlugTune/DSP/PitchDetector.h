#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <array>

namespace PlugTuneDSP
{

enum class VocalRange
{
    Low = 0,   // Bass / Baritone (65 Hz - 400 Hz)
    Mid = 1,   // Tenor / Alto (100 Hz - 700 Hz)
    High = 2   // Soprano / High Pop (150 Hz - 1200 Hz)
};

struct PitchDetectionResult
{
    float frequencyHz = 0.0f;
    float midiNote = 0.0f;
    float clarity = 0.0f; // 0.0 to 1.0 (NSDF peak height)
    bool isVoiced = false;
};

class PitchDetector
{
public:
    PitchDetector();
    ~PitchDetector() = default;

    void prepare(double sampleRate, int samplesPerBlock);
    void reset();

    // Process incoming audio buffer, returns detected pitch result
    PitchDetectionResult processSample(float sample);
    PitchDetectionResult processBlock(const float* channelData, int numSamples);

    void setVocalRange(VocalRange range);
    void setLiveMode(bool isLive);
    bool isLiveMode() const { return mIsLiveMode; }

    static float frequencyToMidi(float freqHz);
    static float midiToFrequency(float midiNote);

private:
    double mSampleRate = 44100.0;
    VocalRange mRange = VocalRange::Mid;
    bool mIsLiveMode = true;

    int mBufferSize = 1024;
    int mMinLag = 36;
    int mMaxLag = 441;
    float mClarityThreshold = 0.60f;

    std::vector<float> mInputBuffer;
    int mWriteIndex = 0;

    std::vector<float> mNsdf;
    std::vector<int> mMaxPositions;
    std::vector<float> mPeriodEstimates;
    std::vector<float> mAmpEstimates;

    PitchDetectionResult mLastResult;
    int mProcessCounter = 0;
    int mHopSize = 128; // Analyze pitch every 128 samples (~2.9ms @ 44.1k)

    // 3-point median filter for jitter-free pitch tracking
    std::array<float, 3> mMidiHistory { 0.0f, 0.0f, 0.0f };
    int mHistoryIdx = 0;

    void updateLagLimits();
    PitchDetectionResult computeMPM();
    float parabolicInterpolation(int peakIndex, float& amp);
};

} // namespace PlugTuneDSP
