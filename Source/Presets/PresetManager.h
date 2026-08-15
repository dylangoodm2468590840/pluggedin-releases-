#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class PresetManager
{
public:
    enum PresetIndex
    {
        // SIGNATURE MACRO PRESETS
        DegenerateRage = 0,
        SludgePlugg,
        CyberAdlib,
        GlitchMonster,
        UndergroundTube,

        // DARK / DEMON
        DemonBelow,
        Underworld,
        Possessed,
        DeepShadow,
        HellLayer,

        // VOCAL WIDTH
        WideLead,
        FloatingHook,
        HeadphoneWide,
        CyberDouble,
        StereoAura,

        // PITCH / FORMANT
        DeepChest,
        Goblin,
        TinyVoice,
        FormantShifter,
        OctaveStack,

        // SPACE
        Cathedral,
        SlapRoom,
        FloatingEcho,
        DarkVoid,
        WashedVocal,

        // DISTORTION / TEXTURE
        TapeWarmth,
        CellPhone,
        FriedMic,
        BrokenIntercom,
        RadioStatic,

        // MOTION
        CyberChorus,
        UnstablePitch,
        Underwater,
        SpinningVocal,
        PulsingGate,

        // DELAY
        DarkSlap,
        ThrowEcho,
        PingPongSpace,
        FilteredTapeDelay,
        PitchEcho,

        // RAP LEADS
        ModernRapLead,
        RageVocal,
        DarkPluggLead,
        IntimateTrap,
        RawUnderground,

        // AD-LIBS
        MonsterAdlib,
        TelephoneShout,
        DistanceAdlib,
        GhostLayer,
        SpectralGhost,

        Custom
    };

    static void applyPreset(juce::AudioProcessorValueTreeState& apvts, PresetIndex preset);
    static const char* getPresetName(PresetIndex preset);
    
    // Intelligent Constrained Randomizer ("INJECT / INSPIRATION")
    static void generateRandomPreset(juce::AudioProcessorValueTreeState& apvts);

    // Automated Preset Recall & Verification Test
    static bool testPresetRecall(juce::AudioProcessorValueTreeState& apvts);
};
