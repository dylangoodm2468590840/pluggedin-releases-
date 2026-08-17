#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

/**
 * @class PresetManager
 * @brief Master Sound Design Vault — 7 Flagship Benchmark Reference Presets for the Underground DSP Engine.
 */
class PresetManager
{
public:
    enum PresetIndex
    {
        // 7 MAIN REFERENCE BENCHMARK PRESETS
        Ref1_PolishedCrisp = 0,    // 01. POLISHED / CRISP (Modern broadcast rap lead, expensive top end)
        Ref2_DarkUnderground,      // 02. DARK / UNDERGROUND (Moody, thick low-mids, tape head bump)
        Ref3_DemonDeep,            // 03. DEMON / DEEP (Parallel -12st formant undertone + Germanium drive)
        Ref4_WideFloating,         // 04. WIDE / FLOATING (Dimension D chorus + ducked Dattorro space)
        Ref5_DestroyedCrushed,     // 05. DESTROYED / CRUSHED (+20dB PUNISH mode + transient preservation)
        Ref6_TelephoneDevice,      // 06. TELEPHONE / DEVICE (Resonant transducer body + diaphragm drive)
        Ref7_FuturisticAlien,      // 07. FUTURISTIC / ALIEN (Formant micro-pitch + exciter + alien motion)

        Custom
    };

    static constexpr int NUM_REFERENCE_PRESETS = 7;

    static void applyPreset(juce::AudioProcessorValueTreeState& apvts, PresetIndex preset);
    static const char* getPresetName(PresetIndex preset);
    
    // Automated Preset Recall & Verification Test
    static bool testPresetRecall(juce::AudioProcessorValueTreeState& apvts);
};
