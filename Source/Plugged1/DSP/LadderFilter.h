#pragma once

#include <juce_dsp/juce_dsp.h>
#include <algorithm>

namespace Plugged1
{

class LadderFilter
{
public:
    enum class FilterMode
    {
        LP24 = 0,
        LP12,
        HP12,
        BP12
    };

    LadderFilter() = default;

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        ladder.prepare(spec);
        ladder.setMode(juce::dsp::LadderFilterMode::LPF24);
        ladder.reset();
    }

    void reset()
    {
        ladder.reset();
    }

    void setCutoff(float cutoffHz)
    {
        cutoff = std::clamp(cutoffHz, 20.0f, 20000.0f);
        ladder.setCutoffFrequencyHz(cutoff);
    }

    void setResonance(float res)
    {
        resonance = std::clamp(res, 0.0f, 1.0f);
        ladder.setResonance(resonance);
    }

    void setDrive(float d)
    {
        ladder.setDrive(1.0f + std::clamp(d, 0.0f, 1.0f) * 3.0f);
    }

    void process(juce::AudioBuffer<float>& buffer)
    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        ladder.process(context);
    }

private:
    juce::dsp::LadderFilter<float> ladder;
    float cutoff = 20000.0f;
    float resonance = 0.0f;
};

} // namespace Plugged1
