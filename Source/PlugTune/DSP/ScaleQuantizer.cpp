#include "ScaleQuantizer.h"
#include <cmath>
#include <algorithm>

namespace PlugTuneDSP
{

ScaleQuantizer::ScaleQuantizer()
{
    mCustomNoteMask.fill(true);
    updateScaleMask();
}

void ScaleQuantizer::prepare(double sampleRate)
{
    mSampleRate = (sampleRate > 8000.0) ? sampleRate : 44100.0;
    reset();
}

void ScaleQuantizer::reset()
{
    mState = NoteTrackerState::Unvoiced;
    mSmoothedCorrectionDelta = 0.0f;
    mSmoothedTargetMidi = 60.0f;
    mCurrentLockedNote = 60;
    mLastInputMidi = 0.0f;
    mWasVoiced = false;
    mMidiOverrideNote = -1;
    mTuneAmount = mTargetTuneAmount;
}

void ScaleQuantizer::setRootKey(int rootNote0to11)
{
    mRootKey = std::clamp(rootNote0to11, 0, 11);
    updateScaleMask();
}

void ScaleQuantizer::setScaleType(ScaleType type)
{
    mScaleType = type;
    updateScaleMask();
}

void ScaleQuantizer::setNoteEnabled(int note0to11, bool enabled)
{
    int n = std::clamp(note0to11, 0, 11);
    mCustomNoteMask[n] = enabled;
    mScaleType = ScaleType::Custom;
}

bool ScaleQuantizer::isNoteEnabled(int note0to11) const
{
    int n = std::clamp(note0to11, 0, 11);
    return mCustomNoteMask[n];
}

void ScaleQuantizer::setRetuneSpeedMs(float speedMs)
{
    mRetuneSpeedMs = std::clamp(speedMs, -3.0f, 400.0f);
}

void ScaleQuantizer::setNoteStabilizerMs(float stabilizerMs)
{
    mStabilizerMs = std::max(0.0f, stabilizerMs);
}

void ScaleQuantizer::setHumanize(float humanize0to1)
{
    mHumanize = std::clamp(humanize0to1, 0.0f, 1.0f);
}

void ScaleQuantizer::setMidiTargetNote(int midiNote, bool isActive)
{
    mMidiOverrideNote = isActive ? midiNote : -1;
}

void ScaleQuantizer::setTuneAmount(float amount0to100)
{
    mTargetTuneAmount = std::clamp(amount0to100, 0.0f, 100.0f);
}

void ScaleQuantizer::updateScaleMask()
{
    if (mScaleType == ScaleType::Custom)
        return; // Keep user's custom mask

    mCustomNoteMask.fill(false);

    // Standard scale interval definitions (semitones from root)
    std::vector<int> intervals;
    switch (mScaleType)
    {
        case ScaleType::Minor: // 'm' Natural Minor
            intervals = { 0, 2, 3, 5, 7, 8, 10 };
            break;
        case ScaleType::Major: // 'M' Major
            intervals = { 0, 2, 4, 5, 7, 9, 11 };
            break;
        case ScaleType::Chromatic:
            mCustomNoteMask.fill(true);
            return;
        case ScaleType::HarmonicMinor:
            intervals = { 0, 2, 3, 5, 7, 8, 11 };
            break;
        case ScaleType::PentatonicMinor:
            intervals = { 0, 3, 5, 7, 10 };
            break;
        case ScaleType::PentatonicMajor:
            intervals = { 0, 2, 4, 7, 9 };
            break;
        case ScaleType::Custom:
            return;
    }

    for (int interval : intervals)
    {
        int note = (mRootKey + interval) % 12;
        mCustomNoteMask[note] = true;
    }
}

int ScaleQuantizer::findClosestScaleNote(float midiNote) const
{
    int roundNote = static_cast<int>(std::round(midiNote));
    
    // Check if roundNote itself is in scale
    int chroma = ((roundNote % 12) + 12) % 12;
    if (mCustomNoteMask[chroma])
        return roundNote;

    // Search outwards for closest active note in scale
    for (int dist = 1; dist <= 6; ++dist)
    {
        int lower = roundNote - dist;
        int upper = roundNote + dist;

        int lowerChroma = ((lower % 12) + 12) % 12;
        int upperChroma = ((upper % 12) + 12) % 12;

        bool lowerActive = mCustomNoteMask[lowerChroma];
        bool upperActive = mCustomNoteMask[upperChroma];

        if (lowerActive && upperActive)
        {
            // Pick closer based on fractional distance from incoming pitch
            return (std::abs(midiNote - lower) <= std::abs(upper - midiNote)) ? lower : upper;
        }
        if (lowerActive) return lower;
        if (upperActive) return upper;
    }

    return roundNote;
}

void ScaleQuantizer::getScaleNeighbors(int currentNote, int& lowerNeighbor, int& upperNeighbor) const
{
    lowerNeighbor = currentNote - 1;
    upperNeighbor = currentNote + 1;

    for (int d = 1; d <= 12; ++d)
    {
        int check = currentNote - d;
        int chroma = ((check % 12) + 12) % 12;
        if (mCustomNoteMask[chroma])
        {
            lowerNeighbor = check;
            break;
        }
    }

    for (int d = 1; d <= 12; ++d)
    {
        int check = currentNote + d;
        int chroma = ((check % 12) + 12) % 12;
        if (mCustomNoteMask[chroma])
        {
            upperNeighbor = check;
            break;
        }
    }
}

float ScaleQuantizer::calculateRetuneSpeedMs(float tuneAmount0to100)
{
    float amt = std::clamp(tuneAmount0to100, 0.0f, 100.0f);
    if (amt >= 98.0f)
        return 0.0f; // Instant Hard Trap Quantize (0.0 ms)
    if (amt <= 0.5f)
        return 350.0f; // Bypassed/Transparent

    float u = amt / 100.0f;
    // Continuous Perceptual Retune Curve: tau(u) = 300 * (1 - u)^1.25 * 10^(-1.45 * u)
    float speedMs = 300.0f * std::pow(1.0f - u, 1.25f) * std::pow(10.0f, -1.45f * u);
    return std::max(0.0f, speedMs);
}

float ScaleQuantizer::processQuantization(float inputMidiNote, bool isVoiced, int numSamplesInBlock)
{
    if (!isVoiced || inputMidiNote <= 10.0f)
    {
        mState = NoteTrackerState::Unvoiced;
        mWasVoiced = false;
        mSmoothedCorrectionDelta = 0.0f;
        mSmoothedTargetMidi = inputMidiNote;
        mLastInputMidi = inputMidiNote;
        return inputMidiNote;
    }

    float dt = static_cast<float>(numSamplesInBlock) / static_cast<float>(mSampleRate);

    // 1. Smooth Tune Amount Dial Changes (15ms anti-zipper ramp)
    float dialAlpha = 1.0f - std::exp(-dt / 0.015f);
    mTuneAmount += dialAlpha * (mTargetTuneAmount - mTuneAmount);
    if (std::abs(mTuneAmount - mTargetTuneAmount) < 0.01f)
        mTuneAmount = mTargetTuneAmount;

    // 2. Compute pitch derivative velocity for adaptive hysteresis (st/sec)
    float pitchVelocity = 0.0f;
    if (mWasVoiced && dt > 1e-6f)
    {
        pitchVelocity = std::abs(inputMidiNote - mLastInputMidi) / dt;
    }
    mLastInputMidi = inputMidiNote;

    // 3. Adaptive Micro-Hysteresis Calculation
    float dynamicHysteresis = kBaseHysteresisSemitones;
    if (pitchVelocity > 12.0f) // If pitch is moving faster than 12 st/sec, collapse hysteresis
    {
        float speedNorm = std::clamp((pitchVelocity - 12.0f) / 28.0f, 0.0f, 1.0f);
        dynamicHysteresis = kBaseHysteresisSemitones * (1.0f - speedNorm) + kMinHysteresisSemitones * speedNorm;
    }

    // 4. Determine Target Scale Note via Schmitt-Trigger State Machine
    if (!mWasVoiced)
    {
        // Voice Onset Transition: Snap to nearest scale note immediately
        mState = NoteTrackerState::VoiceOnset;
        mCurrentLockedNote = (mMidiOverrideNote >= 0) ? mMidiOverrideNote : findClosestScaleNote(inputMidiNote);
        mWasVoiced = true;
    }
    else if (mMidiOverrideNote >= 0)
    {
        mCurrentLockedNote = mMidiOverrideNote;
        mState = NoteTrackerState::LockedSteady;
    }
    else
    {
        // Scale-Aware Schmitt Trigger
        int lowerNeighbor = 0, upperNeighbor = 0;
        getScaleNeighbors(mCurrentLockedNote, lowerNeighbor, upperNeighbor);

        float midLower = 0.5f * (static_cast<float>(mCurrentLockedNote) + static_cast<float>(lowerNeighbor));
        float midUpper = 0.5f * (static_cast<float>(mCurrentLockedNote) + static_cast<float>(upperNeighbor));

        float escapeDown = midLower - dynamicHysteresis;
        float escapeUp   = midUpper + dynamicHysteresis;

        if (inputMidiNote < escapeDown)
        {
            mCurrentLockedNote = findClosestScaleNote(inputMidiNote);
            mState = NoteTrackerState::Transitioning;
        }
        else if (inputMidiNote > escapeUp)
        {
            mCurrentLockedNote = findClosestScaleNote(inputMidiNote);
            mState = NoteTrackerState::Transitioning;
        }
        else
        {
            mState = NoteTrackerState::LockedSteady;
        }
    }

    // 5. Target Pitch Delta (Always 100% in-tune in steady state)
    float rawTargetMidi = static_cast<float>(mCurrentLockedNote);
    float targetDelta = rawTargetMidi - inputMidiNote;

    // Optional Humanize: Subtle nuance preservation near pitch center (< 30 cents)
    if (mHumanize > 0.0f && std::abs(targetDelta) < 0.30f)
    {
        targetDelta *= (1.0f - mHumanize * 0.40f);
    }

    // 6. Dynamic Retune Slew Filtering
    float speedMs = calculateRetuneSpeedMs(mTuneAmount);

    if (speedMs <= 0.05f || mTuneAmount >= 95.0f || mState == NoteTrackerState::VoiceOnset || mState == NoteTrackerState::Transitioning)
    {
        // Zero-lag instant assignment (Hard Tune / Attack Onset / Melodic Jump)
        mSmoothedCorrectionDelta = targetDelta;
    }
    else
    {
        // Continuous 1-pole exponential tracking
        float tauSec = speedMs * 0.001f;
        float alpha = 1.0f - std::exp(-dt / std::max(1e-4f, tauSec));
        mSmoothedCorrectionDelta += alpha * (targetDelta - mSmoothedCorrectionDelta);
    }

    mSmoothedTargetMidi = inputMidiNote + mSmoothedCorrectionDelta;
    return mSmoothedTargetMidi;
}

} // namespace PlugTuneDSP


