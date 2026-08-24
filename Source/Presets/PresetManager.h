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
        // 16 MAIN REFERENCE BENCHMARK PRESETS
        Ref1_PolishedCrisp = 0,    // 01. POLISHED / CRISP (Modern broadcast rap lead, expensive top end)
        Ref2_DarkUnderground,      // 02. DARK / UNDERGROUND (Moody, thick low-mids, tape head bump)
        Ref3_DemonDeep,            // 03. DEMON / DEEP (Parallel -12st formant undertone + Germanium drive)
        Ref4_WideFloating,         // 04. WIDE / FLOATING (Dimension D chorus + ducked Dattorro space)
        Ref5_DestroyedCrushed,     // 05. DESTROYED / CRUSHED (+20dB PUNISH mode + transient preservation)
        Ref6_TelephoneDevice,      // 06. TELEPHONE / DEVICE (Resonant transducer body + diaphragm drive)
        Ref7_FuturisticAlien,      // 07. FUTURISTIC / ALIEN (Formant micro-pitch + exciter + alien motion)
        Ref8_Subterranean808,      // 08. SUBTERRANEAN 808 (Massive 808 sub chest body + crisp consonant snap)
        Ref9_VintageTapeMellotron, // 09. VINTAGE TAPE / MELLOTRON (Ampex 350 tape drive + optical flutter)
        Ref10_HyperpopWarp,        // 10. HYPERPOP / WARP (High-pitched +12st formant warp + Fresh Air)
        Ref11_TravisSubDemon,      // 11. TRAVIS SUB-DEMON (-12st Pitch + -5st Formant + Squeeze)
        Ref12_EvilDrill5th,        // 12. EVIL DRILL 5TH (-7st Fifth Down + Hard Tune + Tube Grit)
        Ref13_AbyssMonster,        // 13. ABYSS MONSTER (-24st Subterranean + Deep Throat Formant)
        Ref14_CyborgDrone,         // 14. CYBORG DRONE (Robot Monotone Mode + Modulated Chorus)
        Ref15_ChipmunkAnimeLead,   // 15. CHIPMUNK / ANIME LEAD (+12st Pitch + +8st Formant + Air Top)
        Ref16_GhostHarmonyBed,     // 16. GHOST HARMONY BED (-5st Fourth Down + Ducked Lexicon Space)

        Custom
    };

    static constexpr int NUM_REFERENCE_PRESETS = 16;

    static void applyPreset(juce::AudioProcessorValueTreeState& apvts, PresetIndex preset);
    static const char* getPresetName(PresetIndex preset);
    
    // Automated Preset Recall & Verification Test
    static bool testPresetRecall(juce::AudioProcessorValueTreeState& apvts);
};
