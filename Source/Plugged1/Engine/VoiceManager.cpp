#include "VoiceManager.h"
#include <algorithm>

namespace Plugged1
{

constexpr int maxPolyphony = 32;

VoiceManager::VoiceManager()
{
    for (int i = 0; i < maxPolyphony; ++i)
    {
        auto* voice = new PluggedVoice();
        synth.addVoice(voice);
        if (i == 0)
            monoVoice = voice;
    }

    synth.addSound(new PluggedSound());
    heldNotes.reserve(16);
}

void VoiceManager::prepare(double sampleRate, int /*samplesPerBlock*/)
{
    synth.setCurrentPlaybackSampleRate(sampleRate);
    reset();
}

void VoiceManager::reset()
{
    synth.allNotesOff(0, false);
    heldNotes.clear();
    currentLegatoNote = -1;
}

void VoiceManager::updateParams(const VoiceParams& newParams)
{
    currentParams = newParams;
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<PluggedVoice*>(synth.getVoice(i)))
        {
            voice->updateVoiceParams(currentParams);
        }
    }
}

void VoiceManager::handleNoteOn(int midiChannel, int midiNoteNumber, float velocity)
{
    if (currentParams.voiceMode == 0) // Polyphonic
    {
        synth.noteOn(midiChannel, midiNoteNumber, velocity);
    }
    else // Mono or Legato
    {
        bool isLegato = (currentParams.voiceMode == 2 && !heldNotes.empty());

        // Add to held notes stack
        auto it = std::find_if(heldNotes.begin(), heldNotes.end(),
            [midiNoteNumber](const NoteInfo& n) { return n.noteNumber == midiNoteNumber; });
        if (it != heldNotes.end())
            heldNotes.erase(it);
        heldNotes.push_back({ midiNoteNumber, velocity });

        currentLegatoNote = midiNoteNumber;

        if (monoVoice != nullptr)
        {
            if (isLegato && monoVoice->isVoiceActive())
            {
                // Smooth glide to new pitch without retriggering envelope
                monoVoice->setTargetPitch(midiNoteNumber, velocity, true);
            }
            else
            {
                // Clear any other active poly voices and retrigger mono voice
                synth.allNotesOff(midiChannel, false);
                monoVoice->startNote(midiNoteNumber, velocity, synth.getSound(0).get(), 8192);
            }
        }
    }
}

void VoiceManager::handleNoteOff(int midiChannel, int midiNoteNumber, float velocity)
{
    if (currentParams.voiceMode == 0) // Polyphonic
    {
        synth.noteOff(midiChannel, midiNoteNumber, velocity, true);
    }
    else // Mono or Legato
    {
        auto it = std::find_if(heldNotes.begin(), heldNotes.end(),
            [midiNoteNumber](const NoteInfo& n) { return n.noteNumber == midiNoteNumber; });
        if (it != heldNotes.end())
            heldNotes.erase(it);

        if (heldNotes.empty())
        {
            currentLegatoNote = -1;
            if (monoVoice != nullptr)
                monoVoice->stopNote(velocity, true);
        }
        else
        {
            // Fall back to most recently held note
            const auto& previousNote = heldNotes.back();
            currentLegatoNote = previousNote.noteNumber;
            if (monoVoice != nullptr)
            {
                if (currentParams.voiceMode == 2) // Legato Glide back
                    monoVoice->setTargetPitch(previousNote.noteNumber, previousNote.velocity, true);
                else
                    monoVoice->startNote(previousNote.noteNumber, previousNote.velocity, synth.getSound(0).get(), 8192);
            }
        }
    }
}

void VoiceManager::handlePitchWheel(int midiChannel, int pitchWheelValue)
{
    synth.handlePitchWheel(midiChannel, pitchWheelValue);
}

void VoiceManager::handleMidiEvent(const juce::MidiMessage& message)
{
    if (message.isNoteOn())
    {
        handleNoteOn(message.getChannel(), message.getNoteNumber(), message.getFloatVelocity());
    }
    else if (message.isNoteOff())
    {
        handleNoteOff(message.getChannel(), message.getNoteNumber(), message.getFloatVelocity());
    }
    else if (message.isPitchWheel())
    {
        handlePitchWheel(message.getChannel(), message.getPitchWheelValue());
    }
    else if (message.isAllNotesOff() || message.isAllSoundOff())
    {
        reset();
    }
}

void VoiceManager::process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    if (currentParams.voiceMode == 0) // Polyphonic mode uses JUCE Synthesiser standard render
    {
        synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
    }
    else // Custom Mono/Legato rendering with note stack tracking
    {
        int currentSample = 0;
        for (const auto metadata : midiMessages)
        {
            const auto& msg = metadata.getMessage();
            int samplePos = metadata.samplePosition;
            int samplesToProcess = samplePos - currentSample;

            if (samplesToProcess > 0 && monoVoice != nullptr)
            {
                monoVoice->renderNextBlock(buffer, currentSample, samplesToProcess);
                currentSample = samplePos;
            }

            handleMidiEvent(msg);
        }

        int remainingSamples = buffer.getNumSamples() - currentSample;
        if (remainingSamples > 0 && monoVoice != nullptr)
        {
            monoVoice->renderNextBlock(buffer, currentSample, remainingSamples);
        }
    }
}

int VoiceManager::getNumActiveVoices() const
{
    int active = 0;
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (synth.getVoice(i)->isVoiceActive())
            active++;
    }
    return active;
}

} // namespace Plugged1
