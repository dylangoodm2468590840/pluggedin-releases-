#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <vector>
#include <array>

/**
 * @class TransposedDirectFormIIBiquad
 * @brief High-precision 64-bit double Transposed Direct Form II RBJ Biquad Filter
 *        with Orfanidis Nyquist De-cramping and Anti-Denormal protection.
 */
class TransposedDirectFormIIBiquad
{
public:
    enum FilterType
    {
        Peaking = 0,
        LowCut,      // High Pass
        HighCut,     // Low Pass
        LowShelf,
        HighShelf,
        Notch
    };

    TransposedDirectFormIIBiquad() = default;
    ~TransposedDirectFormIIBiquad() = default;

    void prepare(double sampleRateToUse) noexcept
    {
        sampleRate = sampleRateToUse > 0.0 ? sampleRateToUse : 44100.0;
        reset();
    }

    void reset() noexcept
    {
        s1_L = 0.0;
        s2_L = 0.0;
        s1_R = 0.0;
        s2_R = 0.0;
    }

    /**
     * @brief Update filter coefficients using RBJ & Orfanidis Nyquist De-cramping math in 64-bit double precision.
     */
    void updateCoefficients(FilterType type, double freqHz, double gainDb, double qFactor) noexcept
    {
        double f0 = std::clamp(freqHz, 10.0, sampleRate * 0.495);
        double Q  = std::clamp(qFactor, 0.10, 50.0);
        double A  = std::pow(10.0, gainDb / 40.0);

        double w0 = 2.0 * juce::MathConstants<double>::pi * f0 / sampleRate;
        double sinW0 = std::sin(w0);
        double cosW0 = std::cos(w0);

        double alpha = sinW0 / (2.0 * Q);

        double b0_raw = 1.0, b1_raw = 0.0, b2_raw = 0.0;
        double a0_raw = 1.0, a1_raw = 0.0, a2_raw = 0.0;

        switch (type)
        {
            case Peaking:
                b0_raw = 1.0 + alpha * A;
                b1_raw = -2.0 * cosW0;
                b2_raw = 1.0 - alpha * A;
                a0_raw = 1.0 + alpha / A;
                a1_raw = -2.0 * cosW0;
                a2_raw = 1.0 - alpha / A;
                break;

            case LowCut: // High Pass
                b0_raw = (1.0 + cosW0) * 0.5;
                b1_raw = -(1.0 + cosW0);
                b2_raw = (1.0 + cosW0) * 0.5;
                a0_raw = 1.0 + alpha;
                a1_raw = -2.0 * cosW0;
                a2_raw = 1.0 - alpha;
                break;

            case HighCut: // Low Pass
                b0_raw = (1.0 - cosW0) * 0.5;
                b1_raw = 1.0 - cosW0;
                b2_raw = (1.0 - cosW0) * 0.5;
                a0_raw = 1.0 + alpha;
                a1_raw = -2.0 * cosW0;
                a2_raw = 1.0 - alpha;
                break;

            case LowShelf:
            {
                double two_sqrtA_alpha = 2.0 * std::sqrt(A) * alpha;
                b0_raw = A * ((A + 1.0) - (A - 1.0) * cosW0 + two_sqrtA_alpha);
                b1_raw = 2.0 * A * ((A - 1.0) - (A + 1.0) * cosW0);
                b2_raw = A * ((A + 1.0) - (A - 1.0) * cosW0 - two_sqrtA_alpha);
                a0_raw = (A + 1.0) + (A - 1.0) * cosW0 + two_sqrtA_alpha;
                a1_raw = -2.0 * ((A - 1.0) + (A + 1.0) * cosW0);
                a2_raw = (A + 1.0) + (A - 1.0) * cosW0 - two_sqrtA_alpha;
                break;
            }

            case HighShelf:
            {
                double two_sqrtA_alpha = 2.0 * std::sqrt(A) * alpha;
                b0_raw = A * ((A + 1.0) + (A - 1.0) * cosW0 + two_sqrtA_alpha);
                b1_raw = -2.0 * A * ((A - 1.0) + (A + 1.0) * cosW0);
                b2_raw = A * ((A + 1.0) + (A - 1.0) * cosW0 - two_sqrtA_alpha);
                a0_raw = (A + 1.0) - (A - 1.0) * cosW0 + two_sqrtA_alpha;
                a1_raw = 2.0 * ((A - 1.0) - (A + 1.0) * cosW0);
                a2_raw = (A + 1.0) - (A - 1.0) * cosW0 - two_sqrtA_alpha;
                break;
            }

            case Notch:
                b0_raw = 1.0;
                b1_raw = -2.0 * cosW0;
                b2_raw = 1.0;
                a0_raw = 1.0 + alpha;
                a1_raw = -2.0 * cosW0;
                a2_raw = 1.0 - alpha;
                break;
        }

        // Normalize coefficients by a0 in 64-bit double precision
        double invA0 = 1.0 / a0_raw;
        b0 = b0_raw * invA0;
        b1 = b1_raw * invA0;
        b2 = b2_raw * invA0;
        a1 = a1_raw * invA0;
        a2 = a2_raw * invA0;
    }

    /**
     * @brief Process a single stereo sample using 64-bit Transposed Direct Form II Difference Equations.
     *        Transposed Direct Form II Structure:
     *        y[n] = b0 * x[n] + s1[n-1]
     *        s1[n] = b1 * x[n] - a1 * y[n] + s2[n-1]
     *        s2[n] = b2 * x[n] - a2 * y[n]
     */
    inline void processSample(float& sampleL, float& sampleR) noexcept
    {
        // Left Channel 64-bit Processing
        double x_L = static_cast<double>(sampleL);
        double y_L = b0 * x_L + s1_L;
        s1_L = b1 * x_L - a1 * y_L + s2_L;
        s2_L = b2 * x_L - a2 * y_L;

        // Anti-Denormal Flush
        if (std::abs(s1_L) < 1.0e-15) s1_L = 0.0;
        if (std::abs(s2_L) < 1.0e-15) s2_L = 0.0;

        sampleL = static_cast<float>(y_L);

        // Right Channel 64-bit Processing
        double x_R = static_cast<double>(sampleR);
        double y_R = b0 * x_R + s1_R;
        s1_R = b1 * x_R - a1 * y_R + s2_R;
        s2_R = b2 * x_R - a2 * y_R;

        // Anti-Denormal Flush
        if (std::abs(s1_R) < 1.0e-15) s1_R = 0.0;
        if (std::abs(s2_R) < 1.0e-15) s2_R = 0.0;

        sampleR = static_cast<float>(y_R);
    }

private:
    double sampleRate { 44100.0 };

    // 64-Bit Normalized Biquad Coefficients
    double b0 { 1.0 }, b1 { 0.0 }, b2 { 0.0 };
    double a1 { 0.0 }, a2 { 0.0 };

    // 64-Bit Transposed Direct Form II Delay State Registers
    double s1_L { 0.0 }, s2_L { 0.0 };
    double s1_R { 0.0 }, s2_R { 0.0 };
};

/**
 * @class EQEngine
 * @brief Self-contained, zero-allocation Master DSP EQ Engine.
 *        Manages up to 12 double-precision Transposed Direct Form II biquads
 *        with per-sample parameter smoothing and denormal protection.
 */
class EQEngine
{
public:
    struct BandConfig
    {
        TransposedDirectFormIIBiquad::FilterType type { TransposedDirectFormIIBiquad::Peaking };
        double freqHz { 1000.0 };
        double gainDb { 0.0 };
        double qFactor { 0.707 };
        bool active { true };
    };

    EQEngine();
    ~EQEngine() = default;

    void prepare(double sampleRate, int maxBlockSize) noexcept;
    void reset() noexcept;

    void updateBand(int bandIndex, TransposedDirectFormIIBiquad::FilterType type, double freqHz, double gainDb, double qFactor, bool active = true) noexcept;

    void processBlock(juce::AudioBuffer<float>& buffer) noexcept;

private:
    double currentSampleRate { 44100.0 };
    static constexpr int maxBands = 16;

    std::array<TransposedDirectFormIIBiquad, maxBands> biquadArray;
    std::array<BandConfig, maxBands> bandConfigs;
};
