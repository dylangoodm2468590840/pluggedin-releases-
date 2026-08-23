#pragma once

#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <algorithm>

namespace Plugged1
{

class StereoSaturator
{
public:
    enum class DriveType
    {
        SoftClip = 0,
        TubeWarmth,
        HardClip,
        Bitcrush
    };

    StereoSaturator() = default;

    void prepare(const juce::dsp::ProcessSpec& /*spec*/)
    {
        reset();
    }

    void reset()
    {
        lastSampleL = 0.0f;
        lastSampleR = 0.0f;
    }

    void setDrive(float newDrive)
    {
        drive = std::clamp(newDrive, 0.0f, 1.0f);
    }

    void setDriveType(DriveType newType)
    {
        driveType = newType;
    }

    void process(juce::AudioBuffer<float>& buffer)
    {
        if (drive < 0.001f)
            return;

        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();

        float* left = buffer.getWritePointer(0);
        float* right = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;

        const float gain = 1.0f + drive * 8.0f;
        const float outNorm = 1.0f / std::sqrt(1.0f + drive * 4.0f);

        for (int i = 0; i < numSamples; ++i)
        {
            left[i] = processSample(left[i] * gain, driveType) * outNorm;
            if (right != nullptr)
                right[i] = processSample(right[i] * gain, driveType) * outNorm;
        }
    }

private:
    float processSample(float in, DriveType type)
    {
        switch (type)
        {
            case DriveType::SoftClip:
            {
                // Smooth hyperbolic tangent with odd harmonics
                return std::tanh(in);
            }

            case DriveType::TubeWarmth:
            {
                // Asymmetric curve with 2nd & 3rd order tube harmonics
                if (in > 0.0f)
                    return std::tanh(in) + 0.1f * (in * in);
                else
                    return std::tanh(in * 1.1f);
            }

            case DriveType::HardClip:
            {
                // Aggressive drill style square-clipping
                return std::clamp(in, -1.0f, 1.0f);
            }

            case DriveType::Bitcrush:
            {
                // Lo-Fi bit reduction
                float bits = 8.0f - (drive * 5.0f);
                float step = std::pow(2.0f, bits);
                return std::round(in * step) / step;
            }
        }
        return in;
    }

    float drive = 0.0f;
    DriveType driveType = DriveType::SoftClip;
    float lastSampleL = 0.0f;
    float lastSampleR = 0.0f;
};

} // namespace Plugged1
