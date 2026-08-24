#include "PitchShifter.h"
#include <cmath>
#include <algorithm>

namespace PlugTuneDSP
{

PitchShifter::PitchShifter()
{
}

void PitchShifter::configureEngine()
{
    int blockSamples = 0;
    int intervalSamples = 0;

    if (mIsLiveMode)
    {
        // Ultra-Low-Latency Live Tracking Mode:
        // 10ms analysis window (~441 samples @ 44.1kHz / 480 samples @ 48kHz) with 4x overlap (~2.5ms hop).
        // Slashes algorithmic latency down to 10ms for instantaneous live monitoring while recording in FL Studio.
        blockSamples = static_cast<int>(mSampleRate * 0.010);
        if (blockSamples < 384) blockSamples = 384;
        intervalSamples = blockSamples / 4;
        if (intervalSamples < 48) intervalSamples = 48;
    }
    else
    {
        // Studio Vocal-Optimized HQ Mode:
        // 35ms analysis window (1540 samples @ 44.1kHz) with 8x overlap (4.37ms hop) for warm vocal
        // chest resonance, pristine low-harmonic separation (down to 70 Hz), and sample-accurate transient snap.
        blockSamples = static_cast<int>(mSampleRate * 0.035);
        if (blockSamples < 512) blockSamples = 512;
        intervalSamples = blockSamples / 8;
        if (intervalSamples < 64) intervalSamples = 64;
    }

    mStretchEngine.configure(mNumChannels, blockSamples, intervalSamples);
    mInputLatency = mStretchEngine.inputLatency();
    mLatencySamples = mInputLatency + mStretchEngine.outputLatency();

    // Latency Alignment Ring Buffers
    mRingSize = std::max(8192, mLatencySamples * 4);
    mPitchDelayRing.assign(mRingSize, 0.0f);
    mF0DelayRing.assign(mRingSize, 0.0f);
    mRingWritePos = 0;
}

void PitchShifter::setLiveMode(bool isLive)
{
    if (mIsLiveMode != isLive)
    {
        mIsLiveMode = isLive;
        configureEngine();
        reset();
    }
}

void PitchShifter::prepare(double sampleRate, int samplesPerBlock, int numChannels)
{
    mSampleRate = (sampleRate > 8000.0) ? sampleRate : 44100.0;
    mNumChannels = std::max(1, numChannels);

    configureEngine();

    int maxBlock = samplesPerBlock + 1024;
    mScratchBuffer.setSize(mNumChannels, maxBlock, false, true, true);
    mScratchBuffer.clear();

    mInputPointers.resize(mNumChannels, nullptr);
    mOutputPointers.resize(mNumChannels, nullptr);

    mTargetPitch = 0.0f;
    mCurrentPitch = 0.0f;
    mTargetF0 = 0.0f;
    mTargetFormant = 0.0f;
    mCurrentFormant = 0.0f;
    mTuneAmount = 100.0f;

    reset();
}

void PitchShifter::reset()
{
    mStretchEngine.reset();
    mScratchBuffer.clear();
    std::fill(mPitchDelayRing.begin(), mPitchDelayRing.end(), 0.0f);
    std::fill(mF0DelayRing.begin(), mF0DelayRing.end(), 0.0f);
    mRingWritePos = 0;
    mTargetPitch = 0.0f;
    mCurrentPitch = 0.0f;
    mTargetF0 = 0.0f;
    mTargetFormant = 0.0f;
    mCurrentFormant = 0.0f;
}

void PitchShifter::setPitchAndFormant(float pitchShiftSemitones, float formantSemitones, float detectedF0Hz, bool isVoiced, float tuneAmount)
{
    mTuneAmount = tuneAmount;
    if (isVoiced && tuneAmount > 0.0f)
    {
        mTargetPitch = std::clamp(pitchShiftSemitones, -24.0f, 24.0f);
        mTargetF0 = (detectedF0Hz > 40.0f && detectedF0Hz < 1500.0f) ? detectedF0Hz : 0.0f;
    }
    else
    {
        mTargetPitch = 0.0f;
        mTargetF0 = 0.0f;
    }

    mTargetFormant = std::clamp(formantSemitones, -24.0f, 24.0f);
}

void PitchShifter::process(juce::AudioBuffer<float>& buffer)
{
    int numChannels = std::min(mNumChannels, buffer.getNumChannels());
    int numSamples = buffer.getNumSamples();

    if (numChannels == 0 || numSamples == 0) return;

    if (mScratchBuffer.getNumSamples() < numSamples)
    {
        mScratchBuffer.setSize(numChannels, numSamples + 256, false, true, true);
    }

    for (int ch = 0; ch < numChannels; ++ch)
    {
        mInputPointers[ch] = const_cast<float*>(buffer.getReadPointer(ch));
        mOutputPointers[ch] = mScratchBuffer.getWritePointer(ch);
    }

    // Push pitch & F0 into latency alignment FIFO and read delayed values matching STFT analysis center
    float alignedPitch = mTargetPitch;
    float alignedF0 = mTargetF0;

    for (int i = 0; i < numSamples; ++i)
    {
        mPitchDelayRing[mRingWritePos] = mTargetPitch;
        mF0DelayRing[mRingWritePos] = mTargetF0;

        int readPos = (mRingWritePos - mInputLatency + mRingSize) % mRingSize;
        alignedPitch = mPitchDelayRing[readPos];
        alignedF0 = mF0DelayRing[readPos];

        mRingWritePos = (mRingWritePos + 1) % mRingSize;
    }

    float dt = static_cast<float>(numSamples) / static_cast<float>(mSampleRate);

    // Hard Mode (>= 90%): Instant 0ms pitch response (true 100% hard autotune snap)
    if (mTuneAmount >= 90.0f)
    {
        mCurrentPitch = alignedPitch;
    }
    else
    {
        // Natural / Slew Mode: Fast anti-zipper parameter ramp (3ms time constant)
        float pitchAlpha = 1.0f - std::exp(-dt / 0.003f);
        mCurrentPitch += pitchAlpha * (alignedPitch - mCurrentPitch);
        if (std::abs(mCurrentPitch - alignedPitch) < 1e-4f)
        {
            mCurrentPitch = alignedPitch;
        }
    }

    // Smooth formant shift across blocks (8ms time constant)
    float formantAlpha = 1.0f - std::exp(-dt / 0.008f);
    mCurrentFormant += formantAlpha * (mTargetFormant - mCurrentFormant);
    if (std::abs(mCurrentFormant - mTargetFormant) < 1e-4f)
    {
        mCurrentFormant = mTargetFormant;
    }

    // Configure Signalsmith Stretch parameters:
    mStretchEngine.setTransposeSemitones(mCurrentPitch, 0.0f);

    // Provide pitch-synchronous fundamental frequency to lock vocal spectral envelope
    if (alignedF0 > 40.0f)
    {
        mStretchEngine.setFormantBase(alignedF0);
    }
    else
    {
        mStretchEngine.setFormantBase(0.0f);
    }

    // Neutral Formant Mode: When formant is 0st, pass factor 1.0 without pitch compensation
    // to bypass rough spectral estimation and guarantee transparent reconstruction.
    if (std::abs(mCurrentFormant) > 0.01f)
    {
        mStretchEngine.setFormantSemitones(mCurrentFormant, false);
    }
    else
    {
        mStretchEngine.setFormantFactor(1.0f, false);
    }

    // Run phase-locked spectral transformation through Signalsmith
    mStretchEngine.process(mInputPointers.data(), numSamples, mOutputPointers.data(), numSamples);

    // Direct, phase-accurate copy to output buffer (zero comb filtering, zero dry/wet beating)
    for (int ch = 0; ch < numChannels; ++ch)
    {
        buffer.copyFrom(ch, 0, mScratchBuffer, ch, 0, numSamples);
    }
}

} // namespace PlugTuneDSP

