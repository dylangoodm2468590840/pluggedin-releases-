#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>

namespace PlugTuneDSP
{

class FormantFilter
{
public:
    FormantFilter();
    ~FormantFilter() = default;

    void prepare(double sampleRate);
    void reset();

    void setFormantShiftSemitones(float semitones); // -12.0 to +12.0

    void process(float* buffer, int numSamples);

private:
    double mSampleRate = 44100.0;
    float mFormantShift = 0.0f;

    // Resonant formant filter states
    struct Biquad
    {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
        float a1 = 0.0f, a2 = 0.0f;
        float z1 = 0.0f, z2 = 0.0f;

        void reset() { z1 = z2 = 0.0f; }
        inline float process(float in)
        {
            float out = b0 * in + z1;
            z1 = b1 * in - a1 * out + z2;
            z2 = b2 * in - a2 * out;
            return out;
        }
        void setPeak(float freqHz, float Q, float gainDb, double sRate);
    };

    // Standard vocal formant bands (F1, F2, F3, F4)
    std::vector<Biquad> mFormantBands;
    void updateFilters();
};

} // namespace PlugTuneDSP
