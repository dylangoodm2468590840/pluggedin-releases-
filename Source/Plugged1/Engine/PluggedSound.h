#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace Plugged1
{

class PluggedSound : public juce::SynthesiserSound
{
public:
    PluggedSound() = default;

    bool appliesToNote(int /*midiNoteNumber*/) override { return true; }
    bool appliesToChannel(int /*midiChannel*/) override { return true; }
};

} // namespace Plugged1
