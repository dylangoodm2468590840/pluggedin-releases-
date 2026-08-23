#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluggedVoice.h"
#include "PluggedSound.h"
#include <vector>

namespace Plugged1
{

class VoiceManager
{
public:
    VoiceManager();
    ~VoiceManager() = default;

    void prepare(double sampleRate, int samplesPerBlock);
    void updateParams(const VoiceParams& newParams);
    void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);
    void reset();

    int getNumActiveVoices() const;

private:
    void handleMidiEvent(const juce::MidiMessage& message);
    void handleNoteOn(int midiChannel, int midiNoteNumber, float velocity);
    void handleNoteOff(int midiChannel, int midiNoteNumber, float velocity);
    void handlePitchWheel(int midiChannel, int pitchWheelValue);

    juce::Synthesiser synth;
    VoiceParams currentParams;

    // Mono-Legato note stack
    struct NoteInfo
    {
        int noteNumber;
        float velocity;
    };
    std::vector<NoteInfo> heldNotes;
    int currentLegatoNote = -1;
    PluggedVoice* monoVoice = nullptr;
};

} // namespace Plugged1
