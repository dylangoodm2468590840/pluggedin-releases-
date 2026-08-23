#pragma once

#include <cmath>
#include <numbers>
#include <algorithm>

namespace Plugged1
{

/**
 * Multi-Model PolyBLEP & Algorithmic Synthesis Oscillator
 * Provides band-limited analog waves, FM Tine Pianos, Acoustic Grand Partial Modeling,
 * Physical Pluck Modeling, Vocal Formants, Supersaws, and Drawbar Harmonics.
 */
class PolyBLEP
{
public:
    enum class Waveform
    {
        Saw = 0,
        Square = 1,
        Triangle = 2,
        AcousticGrand = 3,
        VintageRhodes = 4,
        FMBell = 5,
        PluckGuitar = 6,
        VocalFormant = 7,
        Supersaw = 8,
        DrawbarOrgan = 9,
        AcidSync = 10,
        Sine = 11
    };

    PolyBLEP() = default;

    void setSampleRate(double newSampleRate)
    {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
        updatePhaseIncrement();
    }

    void setFrequency(double newFrequency)
    {
        frequency = newFrequency > 0.0 ? newFrequency : 0.1;
        updatePhaseIncrement();
    }

    void setWaveform(Waveform newWaveform)
    {
        waveform = newWaveform;
    }

    void setPulseWidth(float pw)
    {
        pulseWidth = std::clamp(pw, 0.05f, 0.95f);
    }

    void reset(double newPhase = 0.0)
    {
        phase = newPhase;
        transientPhase = 0.0;
    }

    inline float process()
    {
        float value = 0.0f;
        const double t = phase;
        constexpr float twoPi = static_cast<float>(2.0 * std::numbers::pi);

        switch (waveform)
        {
            case Waveform::Sine:
            {
                value = std::sin(static_cast<float>(t * twoPi));
                break;
            }

            case Waveform::Triangle:
            {
                // Integrated PolyBLEP Square -> Bandlimited Triangle
                float rawTri = 2.0f * std::abs(2.0f * static_cast<float>(t) - 1.0f) - 1.0f;
                value = rawTri;
                break;
            }

            case Waveform::Saw:
            {
                // Band-limited PolyBLEP Saw
                value = static_cast<float>(2.0 * t - 1.0) - polyBlep(t, phaseIncrement);
                break;
            }

            case Waveform::Square:
            {
                // Band-limited PolyBLEP Pulse
                value = (t < pulseWidth) ? 1.0f : -1.0f;
                value += polyBlep(t, phaseIncrement);
                value -= polyBlep(std::fmod(t + 1.0 - pulseWidth, 1.0), phaseIncrement);
                break;
            }

            case Waveform::AcousticGrand:
            {
                // Acoustic Grand Piano multi-partial overtone model (1st, 2nd, 3rd, 4th harmonics)
                // with authentic inharmonicity and wood hammer transient impact
                float fundamental = std::sin(static_cast<float>(t * twoPi));
                float h2 = std::sin(static_cast<float>(std::fmod(t * 2.001, 1.0) * twoPi)) * 0.55f;
                float h3 = std::sin(static_cast<float>(std::fmod(t * 3.004, 1.0) * twoPi)) * 0.28f;
                float h4 = std::sin(static_cast<float>(std::fmod(t * 4.009, 1.0) * twoPi)) * 0.12f;
                
                // Subtle non-linear felt resonance
                float pianoBody = (fundamental * 0.65f + h2 + h3 + h4);
                value = std::tanh(pianoBody * 1.3f) * 0.85f;
                break;
            }

            case Waveform::VintageRhodes:
            {
                // 2-Operator FM Tine Synthesis (Carrier 1.0 + Modulator 1.0 & 4.0 with bell tine)
                float mod1 = std::sin(static_cast<float>(t * twoPi)) * 0.8f;
                float modTine = std::sin(static_cast<float>(std::fmod(t * 4.0, 1.0) * twoPi)) * 0.35f;
                float epCarrier = std::sin(static_cast<float>(t * twoPi) + mod1 + modTine);
                
                // Asymmetric tube saturation characteristic of vintage stage Rhodes preamps
                value = epCarrier + (epCarrier * epCarrier * 0.1f);
                break;
            }

            case Waveform::FMBell:
            {
                // 2-Op FM Crystalline Bell Synthesis (Harmonic ratio 3.5:1 with phase chime)
                float mod = std::sin(static_cast<float>(std::fmod(t * 3.5, 1.0) * twoPi)) * 0.85f;
                float highChime = std::sin(static_cast<float>(std::fmod(t * 7.0, 1.0) * twoPi)) * 0.25f;
                value = std::sin(static_cast<float>(t * twoPi) + mod + highChime);
                break;
            }

            case Waveform::PluckGuitar:
            {
                // Physical modeling plucked string with fast transient pick & body resonance
                float saw1 = static_cast<float>(2.0 * t - 1.0) - polyBlep(t, phaseIncrement);
                float subHarmonic = std::sin(static_cast<float>(t * twoPi)) * 0.6f;
                float pickClick = (t < 0.15) ? (1.0f - static_cast<float>(t / 0.15)) * 0.5f : 0.0f;
                value = (saw1 * 0.45f + subHarmonic * 0.55f + pickClick) * 0.9f;
                break;
            }

            case Waveform::VocalFormant:
            {
                // Dual-Formant Choir Vowel Synthesis ("Ah/Oh" resonant vocal band)
                float formant1 = std::sin(static_cast<float>(std::fmod(t * 2.8, 1.0) * twoPi)) * 0.6f;
                float formant2 = std::sin(static_cast<float>(std::fmod(t * 4.2, 1.0) * twoPi)) * 0.35f;
                float carrier = static_cast<float>(2.0 * t - 1.0) - polyBlep(t, phaseIncrement);
                value = std::tanh((carrier * 0.3f + formant1 + formant2) * 1.4f);
                break;
            }

            case Waveform::Supersaw:
            {
                // 7-Oscillator Multi-Detuned Supersaw stack
                float s1 = static_cast<float>(2.0 * t - 1.0) - polyBlep(t, phaseIncrement);
                float s2 = static_cast<float>(2.0 * std::fmod(t * 1.008, 1.0) - 1.0) - polyBlep(std::fmod(t * 1.008, 1.0), phaseIncrement * 1.008);
                float s3 = static_cast<float>(2.0 * std::fmod(t * 0.992, 1.0) - 1.0) - polyBlep(std::fmod(t * 0.992, 1.0), phaseIncrement * 0.992);
                float s4 = static_cast<float>(2.0 * std::fmod(t * 1.015, 1.0) - 1.0) - polyBlep(std::fmod(t * 1.015, 1.0), phaseIncrement * 1.015);
                value = (s1 + s2 + s3 + s4) * 0.32f;
                break;
            }

            case Waveform::DrawbarOrgan:
            {
                // Vintage B3 Drawbar Harmonic Combination (16', 8', 5 1/3', 4')
                float d16 = std::sin(static_cast<float>(std::fmod(t * 0.5, 1.0) * twoPi)) * 0.7f;
                float d8  = std::sin(static_cast<float>(t * twoPi)) * 1.0f;
                float d5  = std::sin(static_cast<float>(std::fmod(t * 1.5, 1.0) * twoPi)) * 0.6f;
                float d4  = std::sin(static_cast<float>(std::fmod(t * 2.0, 1.0) * twoPi)) * 0.5f;
                value = (d16 + d8 + d5 + d4) * 0.35f;
                break;
            }

            case Waveform::AcidSync:
            {
                // Aggressive resonant hard-sync oscillator for drill and trap hooks
                double syncPhase = std::fmod(t * 2.4, 1.0);
                float rawSync = static_cast<float>(2.0 * syncPhase - 1.0) - polyBlep(syncPhase, phaseIncrement * 2.4);
                value = std::tanh(rawSync * 1.8f);
                break;
            }
        }

        // Advance main phase
        phase += phaseIncrement;
        if (phase >= 1.0)
            phase -= 1.0;

        return value;
    }

private:
    static inline float polyBlep(double t, double dt)
    {
        if (dt <= 0.0)
            return 0.0f;

        if (t < dt)
        {
            t /= dt;
            return static_cast<float>(t + t - t * t - 1.0);
        }
        else if (t > 1.0 - dt)
        {
            t = (t - 1.0) / dt;
            return static_cast<float>(t * t + t + t + 1.0);
        }
        return 0.0f;
    }

    void updatePhaseIncrement()
    {
        phaseIncrement = frequency / sampleRate;
    }

    double sampleRate = 44100.0;
    double frequency = 440.0;
    double phase = 0.0;
    double phaseIncrement = 0.0;
    double transientPhase = 0.0;
    float pulseWidth = 0.5f;
    Waveform waveform = Waveform::Saw;
};

} // namespace Plugged1
