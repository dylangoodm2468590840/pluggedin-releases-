#include "FormantFilter.h"
#include <cmath>
#include <algorithm>

namespace PlugTuneDSP
{

void FormantFilter::Biquad::setPeak(float freqHz, float Q, float gainDb, double sRate)
{
    float A = std::pow(10.0f, gainDb / 40.0f);
    float w0 = 2.0f * 3.14159265358979323846f * (freqHz / static_cast<float>(sRate));
    float alpha = std::sin(w0) / (2.0f * std::max(0.1f, Q));
    float cosw0 = std::cos(w0);

    float a0 = 1.0f + alpha / A;
    b0 = (1.0f + alpha * A) / a0;
    b1 = (-2.0f * cosw0) / a0;
    b2 = (1.0f - alpha * A) / a0;
    a1 = (-2.0f * cosw0) / a0;
    a2 = (1.0f - alpha / A) / a0;
}

FormantFilter::FormantFilter()
{
    mFormantBands.resize(4); // F1, F2, F3, F4
}

void FormantFilter::prepare(double sampleRate)
{
    mSampleRate = (sampleRate > 8000.0) ? sampleRate : 44100.0;
    reset();
}

void FormantFilter::reset()
{
    for (auto& band : mFormantBands)
    {
        band.reset();
    }
    updateFilters();
}

void FormantFilter::setFormantShiftSemitones(float semitones)
{
    float clamped = std::clamp(semitones, -12.0f, 12.0f);
    if (std::abs(clamped - mFormantShift) > 0.05f)
    {
        mFormantShift = clamped;
        updateFilters();
    }
}

void FormantFilter::updateFilters()
{
    if (std::abs(mFormantShift) < 0.05f)
    {
        return; // Transparent bypass
    }

    float shiftRatio = std::pow(2.0f, mFormantShift / 12.0f);

    // Standard human vocal formant center frequencies:
    // F1: ~500 Hz, F2: ~1500 Hz, F3: ~2500 Hz, F4: ~3500 Hz
    float f1 = std::clamp(500.0f * shiftRatio, 150.0f, 1500.0f);
    float f2 = std::clamp(1500.0f * shiftRatio, 600.0f, 3500.0f);
    float f3 = std::clamp(2500.0f * shiftRatio, 1200.0f, 5000.0f);
    float f4 = std::clamp(3500.0f * shiftRatio, 2000.0f, 8000.0f);

    float gain = std::clamp(std::abs(mFormantShift) * 0.5f, 0.0f, 6.0f);
    if (mFormantShift < 0) gain = -gain;

    mFormantBands[0].setPeak(f1, 2.5f, gain * 0.8f, mSampleRate);
    mFormantBands[1].setPeak(f2, 3.0f, gain * 1.0f, mSampleRate);
    mFormantBands[2].setPeak(f3, 3.5f, gain * 0.6f, mSampleRate);
    mFormantBands[3].setPeak(f4, 4.0f, gain * 0.4f, mSampleRate);
}

void FormantFilter::process(float* buffer, int numSamples)
{
    if (std::abs(mFormantShift) < 0.05f)
    {
        return; // 100% transparent bypass
    }

    for (int i = 0; i < numSamples; ++i)
    {
        float s = buffer[i];
        for (auto& band : mFormantBands)
        {
            s = band.process(s);
        }
        buffer[i] = s;
    }
}

} // namespace PlugTuneDSP
