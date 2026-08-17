#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <cmath>
#include <algorithm>
#include "../Utils/AudioUtils.h"

/**
 * @class AnalogTransformerCore
 * @brief Neve 1073 / Tube-Tech Vintage Transformer Iron Core & Preamp Emulation.
 * 
 * Features:
 * - Chest Resonance Saturation (180 Hz - 450 Hz) injecting expensive Neumann/Neve vocal weight.
 * - Dynamic Slew-Rate Limiter (rounds off cheap USB/dynamic mic brittle top-end fizz).
 * - Asymmetric 2nd-Order Harmonic Generation with DC blocking.
 */
class AnalogTransformerCore
{
public:
    AnalogTransformerCore();
    ~AnalogTransformerCore() = default;

    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    void setWarmth(float newWarmth) noexcept { warmthSmoother.setTargetValue(std::clamp(newWarmth, 0.0f, 1.0f)); }
    float getWarmth() const noexcept { return warmthSmoother.getTargetValue(); }

    void setIronCoreDrive(float drive) noexcept { ironDrive.store(std::clamp(drive, 0.0f, 1.0f)); }

    void process(juce::AudioBuffer<float>& buffer);

private:
    double sampleRate { 44100.0 };

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> warmthSmoother;
    std::atomic<float> ironDrive { 0.35f };

    // Chest Resonance Bandpass Filter (260Hz center, Q=1.4)
    juce::dsp::StateVariableTPTFilter<float> chestFilter[2];
    
    // Analog Transformer Low-Pass Slew Filter
    juce::dsp::StateVariableTPTFilter<float> slewFilter[2];

    AudioUtils::DCBlocker dcBlocker[2];

    float prevSample[2] { 0.0f, 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnalogTransformerCore)
};
