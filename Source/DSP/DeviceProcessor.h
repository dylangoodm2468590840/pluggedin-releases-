#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "../Utils/AudioUtils.h"

/**
 * @class DeviceProcessor
 * @brief Studio-Grade Acoustic Transducer, Mic Diaphragm & Enclosure Emulation.
 * 
 * Features:
 * - Cell Phone: Narrowband 380Hz-3.4kHz response + 1.8kHz speech formant resonance + ear speaker non-linearity.
 * - Webcam Mic: Boundary acoustic dip + 2.2kHz room reflection ring + preamp overload crunch.
 * - Earbuds: Plastic resonance at 4.2kHz + low-frequency seal loss.
 * - Laptop Mic: Built-in chassis comb resonance + aggressive high-frequency clip.
 * - Voice Memo: Vintage cassette tape head curve + dynamic lo-fi compression.
 */
class DeviceProcessor
{
public:
    enum DeviceType
    {
        Off = 0,
        CellPhone,
        Webcam,
        Earbuds,
        Laptop,
        VoiceMemo
    };

    DeviceProcessor() = default;
    ~DeviceProcessor() = default;

    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    void setDeviceType(DeviceType newType) { currentDevice = newType; }
    void setDrive(float newDrive)          { drive = newDrive; }

    void process(const juce::dsp::ProcessContextReplacing<float>& context);

private:
    DeviceType currentDevice = Off;
    float drive = 0.25f;

    double sampleRate = 44100.0;

    // Multi-stage acoustic filters per channel
    juce::dsp::StateVariableTPTFilter<float> hpFilter[2];
    juce::dsp::StateVariableTPTFilter<float> lpFilter[2];
    juce::dsp::StateVariableTPTFilter<float> resonancePeak[2];
    juce::dsp::StateVariableTPTFilter<float> bodyNotch[2];

    AudioUtils::DCBlocker dcBlocker[2];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeviceProcessor)
};
