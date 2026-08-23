#pragma once
#include <cmath>
#include <algorithm>

namespace AudioUtils
{
    // Sanitize audio samples: eliminate NaNs, Infs, and denormal numbers
    inline float sanitize(float sample) noexcept
    {
        if (!std::isfinite(sample))
            return 0.0f;
        
        // Flush subnormal floats to zero to avoid CPU denormal performance hits
        if (std::abs(sample) < 1.0e-15f)
            return 0.0f;

        return std::clamp(sample, -4.0f, 4.0f);
    }

    inline float dbToGain(float db) noexcept
    {
        return std::pow(10.0f, db * 0.05f);
    }

    inline float gainToDb(float gain) noexcept
    {
        return 20.0f * std::log10(std::max(gain, 1.0e-5f));
    }

    // Tanh soft clipping with drive (normalized peak output to +-1.0)
    inline float tanhSoftClip(float sample, float drive) noexcept
    {
        float x = sample * drive;
        return std::tanh(x);
    }

    // Monotonic cubic polynomial soft clipping with smooth saturation curve
    // Prevents curve foldback by strictly clamping input drive range to [-1.0, 1.0]
    inline float cubicSoftClip(float sample, float drive) noexcept
    {
        float x = std::clamp(sample * drive, -1.0f, 1.0f);
        // x - x^3/3 peaks at 2/3 at x=1.0. Scale by 1.5 to normalize output to +-1.0.
        return 1.5f * (x - (x * x * x) / 3.0f);
    }

    // Smooth parallel fuzz saturation without crossover dead-zones or step discontinuities
    inline float fuzzDistortion(float sample, float drive) noexcept
    {
        float x = sample * drive;
        // Smooth continuous saturation curve with rich harmonic saturation
        return std::tanh(x + 0.3f * std::tanh(x * 2.0f));
    }

    // Fast 1-pole DC Blocker filter for asymmetric saturation
    class DCBlocker
    {
    public:
        void reset() noexcept { x1 = 0.0f; y1 = 0.0f; }
        inline float process(float x) noexcept
        {
            float y = x - x1 + 0.995f * y1;
            x1 = x;
            y1 = y;
            return sanitize(y);
        }
    private:
        float x1 { 0.0f };
        float y1 { 0.0f };
    };
}
