#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "PolyBLEP.h"
#include "PluggedSound.h"

namespace Plugged1
{

struct VoiceParams
{
    // Voice Mode & Glide
    int voiceMode = 0; // 0: Poly, 1: Mono, 2: Legato
    float glideTimeMs = 50.0f;

    // Layer 1: 808 & Sub Machine
    bool subEnabled = true;
    int subWaveform = 0; // 0: Sine, 1: Tri, 2: Warm Sine, 3: Saturated Saw
    int subOctave = -1;
    float subTuneSemi = 0.0f;
    float subPunchAmount = 0.5f; // Pitch dive depth
    float subPunchDecayMs = 40.0f; // Pitch dive decay
    float subDrive = 0.2f;
    float subGain = 0.8f;

    // Layer 2: Synth Engine (ian leads, brass, bells, plucks)
    bool synthEnabled = true;
    int synthShape = 2; // 0: Saw, 1: Square, 2: Triangle, 3: Bell, 4: Pluck
    int synthUnison = 1; // 1 to 4
    float synthDetune = 0.1f;
    float synthSpread = 0.5f;
    int synthOctave = 0;
    float synthGain = 0.7f;

    // Envelopes
    float ampAttackMs = 5.0f;
    float ampDecayMs = 200.0f;
    float ampSustain = 0.7f;
    float ampReleaseMs = 250.0f;

    float filtAttackMs = 5.0f;
    float filtDecayMs = 300.0f;
    float filtSustain = 0.4f;
    float filtReleaseMs = 250.0f;

    // Filter
    int filterType = 0; // 0: Moog 24dB LP, 1: SVF 12dB LP, 2: HP, 3: BP
    float cutoffHz = 8000.0f;
    float resonance = 1.0f;
    float envAmount = 0.0f;

    // Macros
    float macroPunch = 0.0f;
    float macroDirt = 0.0f;
    float macroSpace = 0.0f;
    float macroAir = 0.0f;
};

class PluggedVoice : public juce::SynthesiserVoice
{
public:
    PluggedVoice();
    ~PluggedVoice() override = default;

    bool canPlaySound(juce::SynthesiserSound* sound) override;
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int newPitchWheelValue) override;
    void controllerMoved(int controllerNumber, int newControllerValue) override;
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    void setCurrentPlaybackSampleRate(double newRate) override;
    void updateVoiceParams(const VoiceParams& newParams);

    void setTargetPitch(int midiNoteNumber, float velocity, bool isLegatoGlide);

    bool isVoiceActive() const override { return isPlaying || ampAdsr.isActive() || SynthesiserVoice::isVoiceActive(); }

private:
    bool isPlaying = false;
    int currentPlayingNoteNumber = -1;
    void updateEnvelopes();
    void updateFilter();
    float processSubSample(double currentFreq);
    float processSynthSample(double currentFreq, float& rightChannelOut);

    VoiceParams params;

    // Pitch smoothing and 808 transient envelope
    double currentSampleRate = 44100.0;
    double currentFrequency = 440.0;
    double targetFrequency = 440.0;
    float currentVelocity = 1.0f;
    float pitchBendRatio = 1.0f;

    // 808 Pitch Drop Transient state
    float punchEnvelope = 0.0f;
    float punchDecayCoeff = 0.99f;

    // Glide smoothing
    float glideCoeff = 1.0f;

    // Envelopes
    juce::ADSR ampAdsr;
    juce::ADSR filtAdsr;
    juce::ADSR::Parameters ampAdsrParams;
    juce::ADSR::Parameters filtAdsrParams;

    // Oscillators
    PolyBLEP subOsc;
    static constexpr int maxUnison = 4;
    std::array<PolyBLEP, maxUnison> synthOscs;

    // Voice Filter
    juce::dsp::StateVariableTPTFilter<float> svfFilter;
};

} // namespace Plugged1
