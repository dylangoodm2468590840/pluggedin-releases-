#include "PitchDetector.h"
#include <cmath>
#include <algorithm>

namespace PlugTuneDSP
{

PitchDetector::PitchDetector()
{
    mInputBuffer.resize(2048, 0.0f);
    mNsdf.resize(2048, 0.0f);
    mMaxPositions.reserve(128);
    mPeriodEstimates.reserve(128);
    mAmpEstimates.reserve(128);
}

void PitchDetector::prepare(double sampleRate, int /*samplesPerBlock*/)
{
    mSampleRate = (sampleRate > 8000.0) ? sampleRate : 44100.0;
    
    // Choose buffer size based on lowest detectable frequency (~50 Hz)
    mBufferSize = static_cast<int>(mSampleRate / 40.0);
    if (mBufferSize < 1024) mBufferSize = 1024;
    if (mBufferSize > 4096) mBufferSize = 4096;

    mInputBuffer.assign(mBufferSize * 2, 0.0f);
    mNsdf.assign(mBufferSize, 0.0f);
    mWriteIndex = 0;
    mProcessCounter = 0;
    mHopSize = mIsLiveMode ? std::max(64, static_cast<int>(mSampleRate * 0.0025))
                           : std::max(64, static_cast<int>(mSampleRate * 0.004)); // ~2.5ms update in Live Mode

    updateLagLimits();
    reset();
}

void PitchDetector::setLiveMode(bool isLive)
{
    mIsLiveMode = isLive;
    mHopSize = mIsLiveMode ? std::max(64, static_cast<int>(mSampleRate * 0.0025))
                           : std::max(64, static_cast<int>(mSampleRate * 0.004));
}

void PitchDetector::reset()
{
    std::fill(mInputBuffer.begin(), mInputBuffer.end(), 0.0f);
    std::fill(mNsdf.begin(), mNsdf.end(), 0.0f);
    mMidiHistory.fill(0.0f);
    mHistoryIdx = 0;
    mWriteIndex = 0;
    mProcessCounter = 0;
    mLastResult = PitchDetectionResult();
}

void PitchDetector::setVocalRange(VocalRange range)
{
    if (mRange != range)
    {
        mRange = range;
        updateLagLimits();
    }
}

void PitchDetector::updateLagLimits()
{
    float minFreq = 100.0f;
    float maxFreq = 700.0f;

    switch (mRange)
    {
        case VocalRange::Low:
            minFreq = 55.0f;  // A1 (~55 Hz)
            maxFreq = 400.0f; // G4 (~392 Hz)
            break;
        case VocalRange::Mid:
            minFreq = 90.0f;  // F2 (~87 Hz)
            maxFreq = 750.0f; // F#5 (~740 Hz)
            break;
        case VocalRange::High:
            minFreq = 140.0f; // D3 (~146 Hz)
            maxFreq = 1200.0f;// D6 (~1174 Hz)
            break;
    }

    mMinLag = std::max(2, static_cast<int>(mSampleRate / maxFreq));
    mMaxLag = std::min(mBufferSize - 1, static_cast<int>(mSampleRate / minFreq));
}

float PitchDetector::frequencyToMidi(float freqHz)
{
    if (freqHz <= 10.0f) return 0.0f;
    return 69.0f + 12.0f * std::log2(freqHz / 440.0f);
}

float PitchDetector::midiToFrequency(float midiNote)
{
    return 440.0f * std::pow(2.0f, (midiNote - 69.0f) / 12.0f);
}

PitchDetectionResult PitchDetector::processSample(float sample)
{
    mInputBuffer[mWriteIndex] = sample;
    mWriteIndex = (mWriteIndex + 1) % mInputBuffer.size();

    if (++mProcessCounter >= mHopSize)
    {
        mProcessCounter = 0;
        mLastResult = computeMPM();
    }

    return mLastResult;
}

PitchDetectionResult PitchDetector::processBlock(const float* channelData, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
    {
        processSample(channelData[i]);
    }
    return mLastResult;
}

float PitchDetector::parabolicInterpolation(int peakIndex, float& amp)
{
    if (peakIndex <= 0 || peakIndex >= static_cast<int>(mNsdf.size()) - 1)
    {
        amp = mNsdf[peakIndex];
        return static_cast<float>(peakIndex);
    }

    float alpha = mNsdf[peakIndex - 1];
    float beta  = mNsdf[peakIndex];
    float gamma = mNsdf[peakIndex + 1];

    float denominator = 2.0f * (2.0f * beta - alpha - gamma);
    if (std::abs(denominator) < 1e-6f)
    {
        amp = beta;
        return static_cast<float>(peakIndex);
    }

    float delta = (gamma - alpha) / denominator;
    amp = beta - 0.25f * (alpha - gamma) * delta;
    return static_cast<float>(peakIndex) + delta;
}

PitchDetectionResult PitchDetector::computeMPM()
{
    PitchDetectionResult result;
    int W = mBufferSize / 2;
    if (W <= 0) return result;

    // Check RMS signal energy first (silent/noise gate)
    float sumSq = 0.0f;
    int readStart = (mWriteIndex - mBufferSize + static_cast<int>(mInputBuffer.size())) % mInputBuffer.size();
    for (int j = 0; j < W; ++j)
    {
        float val = mInputBuffer[(readStart + j) % mInputBuffer.size()];
        sumSq += val * val;
    }

    float rms = std::sqrt(sumSq / static_cast<float>(W));
    if (rms < 0.005f) // Silence gate
    {
        return result;
    }

    // Step 1: Compute NSDF (Normalized Square Difference Function)
    int startTau = std::max(0, mMinLag - 1);
    int maxTau = std::min(mMaxLag + 1, static_cast<int>(mNsdf.size()));
    const float energy1 = sumSq;

    for (int tau = startTau; tau < maxTau; ++tau)
    {
        float acf = 0.0f;
        float energy2 = 0.0f;

        for (int j = 0; j < W; ++j)
        {
            float x1 = mInputBuffer[(readStart + j) % mInputBuffer.size()];
            float x2 = mInputBuffer[(readStart + j + tau) % mInputBuffer.size()];
            acf += x1 * x2;
            energy2 += x2 * x2;
        }

        float norm = energy1 + energy2;
        mNsdf[tau] = (norm > 1e-7f) ? (2.0f * acf / norm) : 0.0f;
    }

    // Step 2: Peak Picking
    mMaxPositions.clear();
    mPeriodEstimates.clear();
    mAmpEstimates.clear();

    bool isPositive = false;
    for (int tau = std::max(1, mMinLag); tau < maxTau - 1; ++tau)
    {
        if (mNsdf[tau] > 0.0f && mNsdf[tau] >= mNsdf[tau - 1] && mNsdf[tau] >= mNsdf[tau + 1])
        {
            float peakAmp = 0.0f;
            float exactTau = parabolicInterpolation(tau, peakAmp);
            if (peakAmp > 0.0f)
            {
                mPeriodEstimates.push_back(exactTau);
                mAmpEstimates.push_back(peakAmp);
            }
        }
    }

    if (mAmpEstimates.empty())
    {
        return result;
    }

    // Step 3: Find highest peak above clarity threshold * maxAmp
    float maxAmp = *std::max_element(mAmpEstimates.begin(), mAmpEstimates.end());
    if (maxAmp < mClarityThreshold)
    {
        // Unvoiced / Sibilance / Noise
        mMidiHistory.fill(0.0f);
        result.isVoiced = false;
        result.clarity = maxAmp;
        return result;
    }

    float cutoff = maxAmp * 0.85f;
    float bestPeriod = 0.0f;
    float bestAmp = 0.0f;

    // Pick the first significant peak (to avoid picking octave subharmonics)
    for (size_t i = 0; i < mAmpEstimates.size(); ++i)
    {
        if (mAmpEstimates[i] >= cutoff && mAmpEstimates[i] >= mClarityThreshold)
        {
            bestPeriod = mPeriodEstimates[i];
            bestAmp = mAmpEstimates[i];
            break;
        }
    }

    if (bestPeriod > 0.0f)
    {
        float freq = static_cast<float>(mSampleRate) / bestPeriod;
        if (freq >= 50.0f && freq <= 1500.0f)
        {
            float rawMidi = frequencyToMidi(freq);

            // If onset or melodic jump (> 0.75 st / half-step+), prime history for instantaneous attack
            int lastIdx = (mHistoryIdx + 2) % 3;
            if (mMidiHistory[0] <= 0.0f || std::abs(rawMidi - mMidiHistory[lastIdx]) > 0.75f)
            {
                mMidiHistory[0] = rawMidi;
                mMidiHistory[1] = rawMidi;
                mMidiHistory[2] = rawMidi;
            }
            else
            {
                mMidiHistory[mHistoryIdx] = rawMidi;
                mHistoryIdx = (mHistoryIdx + 1) % 3;
            }

            float a = mMidiHistory[0], b = mMidiHistory[1], c = mMidiHistory[2];
            float medianMidi = (a > b) ? ((b > c) ? b : ((a > c) ? c : a))
                                       : ((a > c) ? a : ((b > c) ? c : b));
            if (medianMidi <= 0.0f) medianMidi = rawMidi;

            result.frequencyHz = midiToFrequency(medianMidi);
            result.midiNote = medianMidi;
            result.clarity = bestAmp;
            result.isVoiced = true;
        }
    }

    return result;
}

} // namespace PlugTuneDSP
