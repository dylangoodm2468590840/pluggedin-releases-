#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>

namespace PlugTuneDSP
{

class StereoDoubler
{
public:
    StereoDoubler();
    ~StereoDoubler() = default;

    void prepare(double sampleRate);
    void reset();

    void setAmount(float amount0to1); // 0.0 (Off) to 1.0 (100%)
    void setWidth(float width0to2);   // 0.0 (Mono) to 2.0 (200% Wide)

    void process(juce::AudioBuffer<float>& buffer);

private:
    double mSampleRate = 44100.0;
    float mAmount = 0.0f;
    float mWidth = 1.0f;

    std::vector<float> mDelayBufferL;
    std::vector<float> mDelayBufferR;
    int mBufSize = 4096;
    int mWritePos = 0;

    int mDelaySamplesL = 485; // ~11ms
    int mDelaySamplesR = 750; // ~17ms
};

} // namespace PlugTuneDSP
