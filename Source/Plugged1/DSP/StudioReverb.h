#pragma once

#include <juce_dsp/juce_dsp.h>
#include <algorithm>

namespace Plugged1
{

class StudioReverb
{
public:
    StudioReverb() = default;

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        reverb.prepare(spec);
        updateParams();
        reverb.reset();
    }

    void reset()
    {
        reverb.reset();
    }

    void setParameters(float newRoomSize, float newDamping, float newMix, float newWidth = 1.0f)
    {
        roomSize = std::clamp(newRoomSize, 0.0f, 1.0f);
        damping = std::clamp(newDamping, 0.0f, 1.0f);
        mix = std::clamp(newMix, 0.0f, 1.0f);
        width = std::clamp(newWidth, 0.0f, 1.0f);
        updateParams();
    }

    void process(juce::AudioBuffer<float>& buffer)
    {
        if (mix < 0.001f)
            return;

        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        reverb.process(context);
    }

private:
    void updateParams()
    {
        juce::dsp::Reverb::Parameters rParams;
        rParams.roomSize = roomSize * 0.95f;
        rParams.damping = damping;
        rParams.wetLevel = mix;
        rParams.dryLevel = 1.0f - (mix * 0.5f); // Maintain dry clarity
        rParams.width = width;
        rParams.freezeMode = 0.0f;
        reverb.setParameters(rParams);
    }

    juce::dsp::Reverb reverb;
    float roomSize = 0.5f;
    float damping = 0.5f;
    float mix = 0.0f;
    float width = 1.0f;
};

} // namespace Plugged1
