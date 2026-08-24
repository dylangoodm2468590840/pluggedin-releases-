#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>

namespace PlugTuneDSP
{

enum class ScaleType
{
    Minor = 0,           // 'm' Natural Minor
    Major = 1,           // 'M' Major
    Chromatic = 2,       // All 12 notes
    HarmonicMinor = 3,   // Harmonic Minor
    PentatonicMinor = 4, // Minor Pentatonic
    PentatonicMajor = 5, // Major Pentatonic
    Custom = 6           // User selected
};

enum class NoteTrackerState
{
    Unvoiced = 0,
    VoiceOnset,
    LockedSteady,
    Transitioning
};

class ScaleQuantizer
{
public:
    ScaleQuantizer();
    ~ScaleQuantizer() = default;

    void prepare(double sampleRate);
    void reset();

    // Scale Configuration
    void setRootKey(int rootNote0to11); // 0 = C, 1 = C#, ... 11 = B
    void setScaleType(ScaleType type);
    void setNoteEnabled(int note0to11, bool enabled);
    bool isNoteEnabled(int note0to11) const;

    // Simplified Master Tuning Parameter (0% = Natural/Off, 100% = Instant Hard Snap)
    void setTuneAmount(float amount0to100);

    // Optional Tuning Controls
    void setRetuneSpeedMs(float speedMs);
    void setNoteStabilizerMs(float stabilizerMs);
    void setHumanize(float humanize0to1);

    // MIDI target override (if MIDI mode is enabled)
    void setMidiTargetNote(int midiNote, bool isActive);

    // Process detected pitch, returns quantized target MIDI note
    float processQuantization(float inputMidiNote, bool isVoiced, int numSamplesInBlock);

    float getCorrectionDeltaSemitones() const { return mSmoothedCorrectionDelta; }
    float getCurrentTargetMidi() const { return mSmoothedTargetMidi; }
    int getCurrentLockedMidiNote() const { return mCurrentLockedNote; }
    NoteTrackerState getTrackerState() const { return mState; }

    // Static continuous retune curve formula: tau(u) = 300 * (1 - u)^1.25 * 10^(-1.45 * u)
    static float calculateRetuneSpeedMs(float tuneAmount0to100);

private:
    double mSampleRate = 44100.0;
    int mRootKey = 0; // C
    ScaleType mScaleType = ScaleType::Chromatic;
    std::array<bool, 12> mCustomNoteMask;

    // Parameters
    float mTuneAmount = 100.0f;
    float mTargetTuneAmount = 100.0f;
    float mRetuneSpeedMs = 0.0f;
    float mStabilizerMs = 0.0f;
    float mHumanize = 0.0f;
    int mMidiOverrideNote = -1;

    // State Tracking
    NoteTrackerState mState = NoteTrackerState::Unvoiced;
    int mCurrentLockedNote = 60;
    float mSmoothedCorrectionDelta = 0.0f;
    float mSmoothedTargetMidi = 60.0f;
    float mLastInputMidi = 0.0f;
    bool mWasVoiced = false;

    // Micro-hysteresis settings
    static constexpr float kBaseHysteresisSemitones = 0.20f; // 20 cents
    static constexpr float kMinHysteresisSemitones  = 0.04f; // 4 cents at high velocity

    void updateScaleMask();
    int findClosestScaleNote(float midiNote) const;
    void getScaleNeighbors(int currentNote, int& lowerNeighbor, int& upperNeighbor) const;
};

} // namespace PlugTuneDSP

