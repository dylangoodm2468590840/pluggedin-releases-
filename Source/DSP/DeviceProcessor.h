#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "../Utils/AudioUtils.h"

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
    float drive = 0.2f;

    double sampleRate = 44100.0;
    juce::dsp::IIR::Filter<float> hpFilterL, hpFilterR;
    juce::dsp::IIR::Filter<float> lpFilterL, lpFilterR;
};
