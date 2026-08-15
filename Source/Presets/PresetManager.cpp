#include "PresetManager.h"
#include "../Utils/ParameterIDs.h"
#include <juce_core/juce_core.h>

const char* PresetManager::getPresetName(PresetIndex preset)
{
    switch (preset)
    {
        case DegenerateRage:    return "DEGENERATE RAGE";
        case SludgePlugg:       return "SLUDGE PLUGG";
        case CyberAdlib:        return "CYBER AD-LIB";
        case GlitchMonster:     return "GLITCH MONSTER";
        case UndergroundTube:   return "UNDERGROUND TUBE";

        case DemonBelow:        return "DEMON BELOW";
        case Underworld:        return "UNDERWORLD";
        case Possessed:         return "POSSESSED";
        case DeepShadow:        return "DEEP SHADOW";
        case HellLayer:         return "HELL LAYER";

        case WideLead:          return "WIDE LEAD";
        case FloatingHook:      return "FLOATING HOOK";
        case HeadphoneWide:     return "HEADPHONE WIDE";
        case CyberDouble:       return "CYBER DOUBLE";
        case StereoAura:        return "STEREO AURA";

        case DeepChest:         return "DEEP CHEST";
        case Goblin:            return "GOBLIN";
        case TinyVoice:         return "TINY VOICE";
        case FormantShifter:    return "FORMANT SHIFTER";
        case OctaveStack:       return "OCTAVE STACK";

        case Cathedral:         return "CATHEDRAL";
        case SlapRoom:          return "SLAP ROOM";
        case FloatingEcho:      return "FLOATING ECHO";
        case DarkVoid:          return "DARK VOID";
        case WashedVocal:       return "WASHED VOCAL";

        case TapeWarmth:        return "TAPE WARMTH";
        case CellPhone:         return "CELL PHONE";
        case FriedMic:          return "FRIED MIC";
        case BrokenIntercom:    return "BROKEN INTERCOM";
        case RadioStatic:       return "RADIO STATIC";

        case CyberChorus:       return "CYBER CHORUS";
        case UnstablePitch:     return "UNSTABLE PITCH";
        case Underwater:        return "UNDERWATER";
        case SpinningVocal:     return "SPINNING VOCAL";
        case PulsingGate:       return "PULSING GATE";

        case DarkSlap:          return "DARK SLAP";
        case ThrowEcho:         return "THROW ECHO";
        case PingPongSpace:     return "PING PONG SPACE";
        case FilteredTapeDelay: return "FILTERED TAPE DELAY";
        case PitchEcho:         return "PITCH ECHO";

        case ModernRapLead:     return "MODERN RAP LEAD";
        case RageVocal:         return "RAGE VOCAL";
        case DarkPluggLead:     return "DARK PLUGG LEAD";
        case IntimateTrap:      return "INTIMATE TRAP";
        case RawUnderground:    return "RAW UNDERGROUND";

        case MonsterAdlib:      return "MONSTER ADLIB";
        case TelephoneShout:    return "TELEPHONE SHOUT";
        case DistanceAdlib:     return "DISTANCE ADLIB";
        case GhostLayer:        return "GHOST LAYER";
        case SpectralGhost:     return "SPECTRAL GHOST";

        default:                return "CUSTOM";
    }
}

void PresetManager::applyPreset(juce::AudioProcessorValueTreeState& apvts, PresetIndex preset)
{
    auto setParam = [&](const char* id, float val) {
        if (auto* param = apvts.getParameter(id))
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost(param->convertTo0to1(val));
            param->endChangeGesture();
        }
    };

    // 1. Baseline Reset across ALL modules & macros
    setParam(ParameterIDs::DEGENERATE, 0.0f);
    setParam(ParameterIDs::INPUT_GAIN, 0.0f);
    setParam(ParameterIDs::OUTPUT_GAIN, 0.0f);
    setParam(ParameterIDs::MIX_GLOBAL, 1.0f);

    setParam(ParameterIDs::MACRO_DEPTH, 0.0f);
    setParam(ParameterIDs::MACRO_DARK, 0.0f);
    setParam(ParameterIDs::MACRO_MOTION, 0.0f);
    setParam(ParameterIDs::MACRO_CHAOS, 0.0f);
    setParam(ParameterIDs::MACRO_AGE, 0.0f);
    setParam(ParameterIDs::MACRO_GHOST, 0.0f);
    setParam(ParameterIDs::MACRO_TONE, 0.5f);

    setParam(ParameterIDs::MODULE_SUB_ENABLE, 1.0f);
    setParam(ParameterIDs::MODULE_GRIT_ENABLE, 1.0f);
    setParam(ParameterIDs::MODULE_MOD_ENABLE, 1.0f);
    setParam(ParameterIDs::MODULE_DELAY_ENABLE, 1.0f);
    setParam(ParameterIDs::MODULE_REVERB_ENABLE, 1.0f);

    setParam(ParameterIDs::SHADOW_ENABLE, 0.0f);
    setParam(ParameterIDs::SHADOW_MIX, 0.0f);
    setParam(ParameterIDs::SHADOW_PITCH, 0.0f);   // Octave Down (-12)
    setParam(ParameterIDs::SHADOW_FORMANT, 0.5f);
    setParam(ParameterIDs::SHADOW_DARKNESS, 0.5f);
    setParam(ParameterIDs::SHADOW_DRIVE, 0.0f);

    setParam(ParameterIDs::CRUSH_CHARACTER, 0.0f); // Soft Clip
    setParam(ParameterIDs::CRUSH_AMOUNT, 0.0f);
    setParam(ParameterIDs::CRUSH_TONE, 0.5f);
    setParam(ParameterIDs::CRUSH_MIX, 1.0f);

    setParam(ParameterIDs::WIDTH_AMOUNT, 0.0f);

    setParam(ParameterIDs::SPACE_REVERB, 0.0f);
    setParam(ParameterIDs::SPACE_DELAY, 0.0f);

    setParam(ParameterIDs::DEVICE_TYPE, 0.0f);   // Off

    // 5-Band Visual & Audio EQ Default Reset
    setParam(ParameterIDs::EQ_LOW_CUT, 35.0f);
    setParam(ParameterIDs::EQ_LOW_CUT_SLOPE, 1.0f); // 12 dB/oct
    setParam(ParameterIDs::EQ_LOW_GAIN, 0.0f);
    setParam(ParameterIDs::EQ_LOW_Q, 0.707f);
    setParam(ParameterIDs::EQ_MID_GAIN, 0.0f);
    setParam(ParameterIDs::EQ_MID_Q, 0.707f);
    setParam(ParameterIDs::EQ_HIGH_GAIN, 0.0f);
    setParam(ParameterIDs::EQ_HIGH_Q, 0.707f);
    setParam(ParameterIDs::EQ_HIGH_CUT, 18000.0f);
    setParam(ParameterIDs::EQ_HIGH_CUT_SLOPE, 1.0f);

    // 2. Complete Handcrafted Parameter States for Every Preset ($100 Commercial Standard)
    switch (preset)
    {
        // ----------------------------------------------------
        // SIGNATURE MACRO PRESETS
        // ----------------------------------------------------
        case DegenerateRage:
            setParam(ParameterIDs::DEGENERATE, 0.85f);
            setParam(ParameterIDs::OUTPUT_GAIN, -2.5f);
            setParam(ParameterIDs::SHADOW_ENABLE, 1.0f);
            setParam(ParameterIDs::SHADOW_MIX, 0.38f);
            setParam(ParameterIDs::SHADOW_PITCH, 0.0f); // -12
            setParam(ParameterIDs::SHADOW_FORMANT, 0.32f);
            setParam(ParameterIDs::SHADOW_DARKNESS, 0.55f);
            setParam(ParameterIDs::SHADOW_DRIVE, 0.35f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 0.0f); // Triode Tube Soft Clip
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.75f);
            setParam(ParameterIDs::CRUSH_TONE, 0.78f); // Air exciter sparkle
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.55f);
            setParam(ParameterIDs::SPACE_REVERB, 0.22f);
            setParam(ParameterIDs::SPACE_DELAY, 0.18f);
            setParam(ParameterIDs::EQ_LOW_CUT, 85.0f);
            setParam(ParameterIDs::EQ_LOW_GAIN, 2.0f);
            setParam(ParameterIDs::EQ_MID_GAIN, 3.0f);
            setParam(ParameterIDs::EQ_HIGH_GAIN, 4.5f);
            setParam(ParameterIDs::EQ_HIGH_CUT, 19000.0f);
            break;

        case SludgePlugg:
            setParam(ParameterIDs::DEGENERATE, 0.65f);
            setParam(ParameterIDs::OUTPUT_GAIN, -2.0f);
            setParam(ParameterIDs::SHADOW_ENABLE, 1.0f);
            setParam(ParameterIDs::SHADOW_MIX, 0.60f);
            setParam(ParameterIDs::SHADOW_PITCH, 0.0f);
            setParam(ParameterIDs::SHADOW_FORMANT, 0.28f);
            setParam(ParameterIDs::SHADOW_DARKNESS, 0.82f);
            setParam(ParameterIDs::SHADOW_DRIVE, 0.50f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 2.0f); // Tape Warmth & Head Bump
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.55f);
            setParam(ParameterIDs::CRUSH_TONE, 0.35f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.45f);
            setParam(ParameterIDs::SPACE_REVERB, 0.40f);
            setParam(ParameterIDs::SPACE_DELAY, 0.28f);
            setParam(ParameterIDs::EQ_LOW_CUT, 45.0f);
            setParam(ParameterIDs::EQ_LOW_GAIN, 5.0f);
            setParam(ParameterIDs::EQ_MID_GAIN, -1.5f);
            setParam(ParameterIDs::EQ_HIGH_GAIN, -3.5f);
            setParam(ParameterIDs::EQ_HIGH_CUT, 10500.0f);
            break;

        case CyberAdlib:
            setParam(ParameterIDs::DEGENERATE, 0.70f);
            setParam(ParameterIDs::OUTPUT_GAIN, -1.5f);
            setParam(ParameterIDs::SHADOW_ENABLE, 1.0f);
            setParam(ParameterIDs::SHADOW_MIX, 0.30f);
            setParam(ParameterIDs::SHADOW_PITCH, 2.0f); // -5
            setParam(ParameterIDs::SHADOW_FORMANT, 0.68f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 1.0f); // Bitcrusher
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.60f);
            setParam(ParameterIDs::CRUSH_TONE, 0.85f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.80f);
            setParam(ParameterIDs::SPACE_REVERB, 0.35f);
            setParam(ParameterIDs::SPACE_DELAY, 0.32f);
            setParam(ParameterIDs::DEVICE_TYPE, 1.0f); // Cell Phone
            setParam(ParameterIDs::EQ_LOW_CUT, 120.0f);
            setParam(ParameterIDs::EQ_MID_GAIN, 2.5f);
            setParam(ParameterIDs::EQ_HIGH_GAIN, 5.5f);
            break;

        case GlitchMonster:
            setParam(ParameterIDs::DEGENERATE, 0.90f);
            setParam(ParameterIDs::OUTPUT_GAIN, -3.0f);
            setParam(ParameterIDs::SHADOW_ENABLE, 1.0f);
            setParam(ParameterIDs::SHADOW_MIX, 0.55f);
            setParam(ParameterIDs::SHADOW_PITCH, 3.0f); // -24
            setParam(ParameterIDs::SHADOW_FORMANT, 0.18f);
            setParam(ParameterIDs::SHADOW_DARKNESS, 0.68f);
            setParam(ParameterIDs::SHADOW_DRIVE, 0.65f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 3.0f); // Fuzz
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.85f);
            setParam(ParameterIDs::CRUSH_TONE, 0.55f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.65f);
            setParam(ParameterIDs::SPACE_REVERB, 0.45f);
            setParam(ParameterIDs::SPACE_DELAY, 0.35f);
            setParam(ParameterIDs::EQ_LOW_CUT, 70.0f);
            setParam(ParameterIDs::EQ_LOW_GAIN, 3.5f);
            setParam(ParameterIDs::EQ_MID_GAIN, 4.0f);
            setParam(ParameterIDs::EQ_HIGH_CUT, 12000.0f);
            break;

        case UndergroundTube:
            setParam(ParameterIDs::OUTPUT_GAIN, -1.0f);
            setParam(ParameterIDs::SHADOW_ENABLE, 1.0f);
            setParam(ParameterIDs::SHADOW_MIX, 0.18f);
            setParam(ParameterIDs::SHADOW_PITCH, 0.0f);
            setParam(ParameterIDs::SHADOW_FORMANT, 0.42f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 0.0f); // Triode Tube
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.50f);
            setParam(ParameterIDs::CRUSH_TONE, 0.60f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.40f);
            setParam(ParameterIDs::SPACE_REVERB, 0.18f);
            setParam(ParameterIDs::SPACE_DELAY, 0.12f);
            setParam(ParameterIDs::EQ_LOW_GAIN, 1.5f);
            setParam(ParameterIDs::EQ_HIGH_GAIN, 2.5f);
            break;

        // ----------------------------------------------------
        // DARK / DEMON CATEGORY (Murda Melodies / Vocal Bender standard)
        // ----------------------------------------------------
        case DemonBelow:
            setParam(ParameterIDs::OUTPUT_GAIN, -2.0f);
            setParam(ParameterIDs::SHADOW_ENABLE, 1.0f);
            setParam(ParameterIDs::SHADOW_MIX, 0.65f);
            setParam(ParameterIDs::SHADOW_PITCH, 0.0f); // -12
            setParam(ParameterIDs::SHADOW_FORMANT, 0.22f); // Deep throat resonance
            setParam(ParameterIDs::SHADOW_DARKNESS, 0.65f);
            setParam(ParameterIDs::SHADOW_DRIVE, 0.45f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 2.0f); // Tape Saturation
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.42f);
            setParam(ParameterIDs::CRUSH_TONE, 0.40f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.50f);
            setParam(ParameterIDs::SPACE_REVERB, 0.35f);
            setParam(ParameterIDs::SPACE_DELAY, 0.22f);
            setParam(ParameterIDs::EQ_LOW_CUT, 65.0f);
            setParam(ParameterIDs::EQ_LOW_GAIN, 3.5f);
            setParam(ParameterIDs::EQ_HIGH_CUT, 14000.0f);
            break;

        case Underworld:
            setParam(ParameterIDs::OUTPUT_GAIN, -2.5f);
            setParam(ParameterIDs::SHADOW_ENABLE, 1.0f);
            setParam(ParameterIDs::SHADOW_MIX, 0.75f);
            setParam(ParameterIDs::SHADOW_PITCH, 3.0f); // -24
            setParam(ParameterIDs::SHADOW_FORMANT, 0.15f);
            setParam(ParameterIDs::SHADOW_DARKNESS, 0.78f);
            setParam(ParameterIDs::SHADOW_DRIVE, 0.55f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 0.0f);
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.60f);
            setParam(ParameterIDs::CRUSH_TONE, 0.35f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.55f);
            setParam(ParameterIDs::SPACE_REVERB, 0.55f);
            setParam(ParameterIDs::SPACE_DELAY, 0.30f);
            setParam(ParameterIDs::EQ_LOW_CUT, 50.0f);
            setParam(ParameterIDs::EQ_LOW_GAIN, 4.5f);
            setParam(ParameterIDs::EQ_HIGH_GAIN, -2.0f);
            break;

        case Possessed:
            setParam(ParameterIDs::OUTPUT_GAIN, -2.0f);
            setParam(ParameterIDs::SHADOW_ENABLE, 1.0f);
            setParam(ParameterIDs::SHADOW_MIX, 0.55f);
            setParam(ParameterIDs::SHADOW_PITCH, 1.0f); // -7
            setParam(ParameterIDs::SHADOW_FORMANT, 0.28f);
            setParam(ParameterIDs::SHADOW_DARKNESS, 0.58f);
            setParam(ParameterIDs::SHADOW_DRIVE, 0.40f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 3.0f); // Fuzz
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.48f);
            setParam(ParameterIDs::CRUSH_TONE, 0.60f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.70f);
            setParam(ParameterIDs::SPACE_REVERB, 0.40f);
            setParam(ParameterIDs::SPACE_DELAY, 0.25f);
            setParam(ParameterIDs::EQ_MID_GAIN, 2.5f);
            break;

        case DeepShadow:
            setParam(ParameterIDs::OUTPUT_GAIN, -1.5f);
            setParam(ParameterIDs::SHADOW_ENABLE, 1.0f);
            setParam(ParameterIDs::SHADOW_MIX, 0.45f);
            setParam(ParameterIDs::SHADOW_PITCH, 0.0f);
            setParam(ParameterIDs::SHADOW_FORMANT, 0.35f);
            setParam(ParameterIDs::SHADOW_DARKNESS, 0.85f);
            setParam(ParameterIDs::SHADOW_DRIVE, 0.25f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 0.0f);
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.25f);
            setParam(ParameterIDs::CRUSH_TONE, 0.45f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.50f);
            setParam(ParameterIDs::SPACE_REVERB, 0.28f);
            setParam(ParameterIDs::SPACE_DELAY, 0.18f);
            setParam(ParameterIDs::EQ_LOW_GAIN, 2.5f);
            break;

        case HellLayer:
            setParam(ParameterIDs::OUTPUT_GAIN, -2.5f);
            setParam(ParameterIDs::SHADOW_ENABLE, 1.0f);
            setParam(ParameterIDs::SHADOW_MIX, 0.70f);
            setParam(ParameterIDs::SHADOW_PITCH, 0.0f);
            setParam(ParameterIDs::SHADOW_FORMANT, 0.12f);
            setParam(ParameterIDs::SHADOW_DARKNESS, 0.65f);
            setParam(ParameterIDs::SHADOW_DRIVE, 0.75f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 2.0f);
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.75f);
            setParam(ParameterIDs::CRUSH_TONE, 0.65f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.60f);
            setParam(ParameterIDs::SPACE_REVERB, 0.45f);
            setParam(ParameterIDs::SPACE_DELAY, 0.25f);
            setParam(ParameterIDs::EQ_LOW_GAIN, 3.5f);
            setParam(ParameterIDs::EQ_MID_GAIN, 2.0f);
            break;

        // ----------------------------------------------------
        // VOCAL WIDTH CATEGORY (Soundtoys MicroShift standard)
        // ----------------------------------------------------
        case WideLead:
            setParam(ParameterIDs::OUTPUT_GAIN, -0.5f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 0.0f);
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.25f);
            setParam(ParameterIDs::CRUSH_TONE, 0.72f); // Air exciter
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.65f);
            setParam(ParameterIDs::SPACE_REVERB, 0.20f);
            setParam(ParameterIDs::SPACE_DELAY, 0.15f);
            setParam(ParameterIDs::EQ_LOW_CUT, 90.0f);
            setParam(ParameterIDs::EQ_HIGH_GAIN, 3.0f);
            break;

        case FloatingHook:
            setParam(ParameterIDs::OUTPUT_GAIN, -1.0f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 0.0f);
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.18f);
            setParam(ParameterIDs::CRUSH_TONE, 0.75f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.85f);
            setParam(ParameterIDs::SPACE_REVERB, 0.50f);
            setParam(ParameterIDs::SPACE_DELAY, 0.35f);
            setParam(ParameterIDs::EQ_LOW_CUT, 100.0f);
            setParam(ParameterIDs::EQ_HIGH_GAIN, 4.0f);
            break;

        case HeadphoneWide:
            setParam(ParameterIDs::OUTPUT_GAIN, -0.5f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 0.0f);
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.15f);
            setParam(ParameterIDs::CRUSH_TONE, 0.70f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.95f);
            setParam(ParameterIDs::SPACE_REVERB, 0.15f);
            setParam(ParameterIDs::SPACE_DELAY, 0.10f);
            setParam(ParameterIDs::EQ_HIGH_GAIN, 2.5f);
            break;

        case CyberDouble:
            setParam(ParameterIDs::OUTPUT_GAIN, -1.0f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 1.0f);
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.35f);
            setParam(ParameterIDs::CRUSH_TONE, 0.80f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.75f);
            setParam(ParameterIDs::SPACE_REVERB, 0.25f);
            setParam(ParameterIDs::SPACE_DELAY, 0.20f);
            setParam(ParameterIDs::EQ_MID_GAIN, 2.0f);
            setParam(ParameterIDs::EQ_HIGH_GAIN, 3.5f);
            break;

        case StereoAura:
            setParam(ParameterIDs::OUTPUT_GAIN, -0.5f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 0.0f);
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.20f);
            setParam(ParameterIDs::CRUSH_TONE, 0.65f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.70f);
            setParam(ParameterIDs::SPACE_REVERB, 0.38f);
            setParam(ParameterIDs::SPACE_DELAY, 0.25f);
            setParam(ParameterIDs::EQ_HIGH_GAIN, 3.0f);
            break;

        // ----------------------------------------------------
        // PITCH / FORMANT CATEGORY
        // ----------------------------------------------------
        case DeepChest:
            setParam(ParameterIDs::OUTPUT_GAIN, -1.5f);
            setParam(ParameterIDs::SHADOW_ENABLE, 1.0f);
            setParam(ParameterIDs::SHADOW_MIX, 0.50f);
            setParam(ParameterIDs::SHADOW_PITCH, 0.0f);
            setParam(ParameterIDs::SHADOW_FORMANT, 0.30f); // Chest resonance
            setParam(ParameterIDs::SHADOW_DARKNESS, 0.55f);
            setParam(ParameterIDs::SHADOW_DRIVE, 0.35f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 0.0f);
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.25f);
            setParam(ParameterIDs::CRUSH_TONE, 0.55f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.38f);
            setParam(ParameterIDs::SPACE_REVERB, 0.18f);
            setParam(ParameterIDs::SPACE_DELAY, 0.12f);
            setParam(ParameterIDs::EQ_LOW_GAIN, 3.0f);
            break;

        case Goblin:
            setParam(ParameterIDs::OUTPUT_GAIN, -1.5f);
            setParam(ParameterIDs::SHADOW_ENABLE, 1.0f);
            setParam(ParameterIDs::SHADOW_MIX, 0.55f);
            setParam(ParameterIDs::SHADOW_PITCH, 2.0f); // -5
            setParam(ParameterIDs::SHADOW_FORMANT, 0.75f); // High throat
            setParam(ParameterIDs::SHADOW_DARKNESS, 0.40f);
            setParam(ParameterIDs::SHADOW_DRIVE, 0.40f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 1.0f);
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.50f);
            setParam(ParameterIDs::CRUSH_TONE, 0.75f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.50f);
            setParam(ParameterIDs::SPACE_REVERB, 0.25f);
            setParam(ParameterIDs::SPACE_DELAY, 0.20f);
            setParam(ParameterIDs::EQ_MID_GAIN, 3.5f);
            break;

        case TinyVoice:
            setParam(ParameterIDs::OUTPUT_GAIN, -1.0f);
            setParam(ParameterIDs::SHADOW_ENABLE, 1.0f);
            setParam(ParameterIDs::SHADOW_MIX, 0.35f);
            setParam(ParameterIDs::SHADOW_PITCH, 1.0f);
            setParam(ParameterIDs::SHADOW_FORMANT, 0.88f);
            setParam(ParameterIDs::SHADOW_DARKNESS, 0.30f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 1.0f);
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.35f);
            setParam(ParameterIDs::CRUSH_TONE, 0.85f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.40f);
            setParam(ParameterIDs::SPACE_REVERB, 0.20f);
            setParam(ParameterIDs::SPACE_DELAY, 0.15f);
            setParam(ParameterIDs::DEVICE_TYPE, 1.0f);
            setParam(ParameterIDs::EQ_LOW_CUT, 250.0f);
            setParam(ParameterIDs::EQ_HIGH_GAIN, 4.0f);
            break;

        case FormantShifter:
            setParam(ParameterIDs::OUTPUT_GAIN, -1.0f);
            setParam(ParameterIDs::SHADOW_ENABLE, 1.0f);
            setParam(ParameterIDs::SHADOW_MIX, 0.45f);
            setParam(ParameterIDs::SHADOW_PITCH, 0.0f);
            setParam(ParameterIDs::SHADOW_FORMANT, 0.22f);
            setParam(ParameterIDs::SHADOW_DARKNESS, 0.60f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 0.0f);
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.28f);
            setParam(ParameterIDs::CRUSH_TONE, 0.55f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.40f);
            setParam(ParameterIDs::SPACE_REVERB, 0.20f);
            setParam(ParameterIDs::SPACE_DELAY, 0.15f);
            setParam(ParameterIDs::EQ_LOW_GAIN, 2.0f);
            break;

        case OctaveStack:
            setParam(ParameterIDs::OUTPUT_GAIN, -1.5f);
            setParam(ParameterIDs::SHADOW_ENABLE, 1.0f);
            setParam(ParameterIDs::SHADOW_MIX, 0.50f);
            setParam(ParameterIDs::SHADOW_PITCH, 0.0f);
            setParam(ParameterIDs::SHADOW_FORMANT, 0.50f);
            setParam(ParameterIDs::SHADOW_DARKNESS, 0.50f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 0.0f);
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.32f);
            setParam(ParameterIDs::CRUSH_TONE, 0.65f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.60f);
            setParam(ParameterIDs::SPACE_REVERB, 0.30f);
            setParam(ParameterIDs::SPACE_DELAY, 0.20f);
            setParam(ParameterIDs::EQ_HIGH_GAIN, 2.5f);
            break;

        // ----------------------------------------------------
        // SPACE CATEGORY (Dattorro Diffusion Reverb & Tape Echo)
        // ----------------------------------------------------
        case Cathedral:
            setParam(ParameterIDs::OUTPUT_GAIN, -1.5f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 0.0f);
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.15f);
            setParam(ParameterIDs::CRUSH_TONE, 0.65f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.80f);
            setParam(ParameterIDs::SPACE_REVERB, 0.82f);
            setParam(ParameterIDs::SPACE_DELAY, 0.38f);
            setParam(ParameterIDs::EQ_LOW_CUT, 110.0f);
            setParam(ParameterIDs::EQ_HIGH_GAIN, 3.5f);
            break;

        case SlapRoom:
            setParam(ParameterIDs::OUTPUT_GAIN, -0.5f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 0.0f);
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.22f);
            setParam(ParameterIDs::CRUSH_TONE, 0.60f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.35f);
            setParam(ParameterIDs::SPACE_REVERB, 0.18f);
            setParam(ParameterIDs::SPACE_DELAY, 0.10f);
            setParam(ParameterIDs::EQ_LOW_CUT, 90.0f);
            break;

        case FloatingEcho:
            setParam(ParameterIDs::OUTPUT_GAIN, -1.0f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 0.0f);
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.18f);
            setParam(ParameterIDs::CRUSH_TONE, 0.70f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.80f);
            setParam(ParameterIDs::SPACE_REVERB, 0.35f);
            setParam(ParameterIDs::SPACE_DELAY, 0.48f);
            setParam(ParameterIDs::EQ_HIGH_GAIN, 3.0f);
            break;

        case DarkVoid:
            setParam(ParameterIDs::OUTPUT_GAIN, -1.5f);
            setParam(ParameterIDs::SHADOW_ENABLE, 1.0f);
            setParam(ParameterIDs::SHADOW_MIX, 0.38f);
            setParam(ParameterIDs::SHADOW_PITCH, 0.0f);
            setParam(ParameterIDs::SHADOW_FORMANT, 0.30f);
            setParam(ParameterIDs::SHADOW_DARKNESS, 0.80f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 0.0f);
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.25f);
            setParam(ParameterIDs::CRUSH_TONE, 0.30f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.65f);
            setParam(ParameterIDs::SPACE_REVERB, 0.68f);
            setParam(ParameterIDs::SPACE_DELAY, 0.32f);
            setParam(ParameterIDs::EQ_LOW_GAIN, 2.0f);
            setParam(ParameterIDs::EQ_HIGH_CUT, 11000.0f);
            break;

        case WashedVocal:
            setParam(ParameterIDs::OUTPUT_GAIN, -1.5f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 0.0f);
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.12f);
            setParam(ParameterIDs::CRUSH_TONE, 0.55f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.90f);
            setParam(ParameterIDs::SPACE_REVERB, 0.88f);
            setParam(ParameterIDs::SPACE_DELAY, 0.35f);
            setParam(ParameterIDs::EQ_LOW_CUT, 120.0f);
            break;

        // ----------------------------------------------------
        // DISTORTION / TEXTURE CATEGORY (Decapitator & Tape Warmth)
        // ----------------------------------------------------
        case TapeWarmth:
            setParam(ParameterIDs::OUTPUT_GAIN, -0.5f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 2.0f); // Studer Tape
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.45f);
            setParam(ParameterIDs::CRUSH_TONE, 0.55f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.30f);
            setParam(ParameterIDs::SPACE_REVERB, 0.12f);
            setParam(ParameterIDs::SPACE_DELAY, 0.08f);
            setParam(ParameterIDs::EQ_LOW_GAIN, 2.5f);
            setParam(ParameterIDs::EQ_HIGH_GAIN, 1.5f);
            break;

        case CellPhone:
            setParam(ParameterIDs::OUTPUT_GAIN, -1.0f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 1.0f);
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.48f);
            setParam(ParameterIDs::CRUSH_TONE, 0.85f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.20f);
            setParam(ParameterIDs::SPACE_REVERB, 0.15f);
            setParam(ParameterIDs::SPACE_DELAY, 0.10f);
            setParam(ParameterIDs::DEVICE_TYPE, 1.0f);
            setParam(ParameterIDs::EQ_LOW_CUT, 420.0f);
            setParam(ParameterIDs::EQ_LOW_CUT_SLOPE, 3.0f);
            setParam(ParameterIDs::EQ_MID_GAIN, 4.5f);
            setParam(ParameterIDs::EQ_HIGH_GAIN, -12.0f);
            setParam(ParameterIDs::EQ_HIGH_CUT, 3200.0f);
            setParam(ParameterIDs::EQ_HIGH_CUT_SLOPE, 3.0f);
            break;

        case FriedMic:
            setParam(ParameterIDs::OUTPUT_GAIN, -2.5f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 3.0f); // Germanium Fuzz
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.80f);
            setParam(ParameterIDs::CRUSH_TONE, 0.70f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.40f);
            setParam(ParameterIDs::SPACE_REVERB, 0.20f);
            setParam(ParameterIDs::SPACE_DELAY, 0.10f);
            setParam(ParameterIDs::DEVICE_TYPE, 2.0f);
            setParam(ParameterIDs::EQ_LOW_CUT, 150.0f);
            setParam(ParameterIDs::EQ_MID_GAIN, 3.0f);
            break;

        case BrokenIntercom:
            setParam(ParameterIDs::OUTPUT_GAIN, -2.0f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 2.0f);
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.70f);
            setParam(ParameterIDs::CRUSH_TONE, 0.40f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.15f);
            setParam(ParameterIDs::SPACE_REVERB, 0.20f);
            setParam(ParameterIDs::SPACE_DELAY, 0.15f);
            setParam(ParameterIDs::DEVICE_TYPE, 5.0f);
            setParam(ParameterIDs::EQ_LOW_CUT, 300.0f);
            setParam(ParameterIDs::EQ_HIGH_CUT, 4000.0f);
            break;

        case RadioStatic:
            setParam(ParameterIDs::OUTPUT_GAIN, -1.5f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 1.0f);
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.75f);
            setParam(ParameterIDs::CRUSH_TONE, 0.90f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.15f);
            setParam(ParameterIDs::SPACE_REVERB, 0.15f);
            setParam(ParameterIDs::SPACE_DELAY, 0.10f);
            setParam(ParameterIDs::DEVICE_TYPE, 4.0f);
            setParam(ParameterIDs::EQ_LOW_CUT, 500.0f);
            setParam(ParameterIDs::EQ_HIGH_CUT, 5000.0f);
            break;

        // ----------------------------------------------------
        // MOTION CATEGORY (Chorus, Flanger, Tremolo)
        // ----------------------------------------------------
        case CyberChorus:
            setParam(ParameterIDs::OUTPUT_GAIN, -0.5f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 0.0f);
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.20f);
            setParam(ParameterIDs::CRUSH_TONE, 0.70f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.85f);
            setParam(ParameterIDs::SPACE_REVERB, 0.30f);
            setParam(ParameterIDs::SPACE_DELAY, 0.20f);
            setParam(ParameterIDs::EQ_HIGH_GAIN, 3.0f);
            break;

        case UnstablePitch:
            setParam(ParameterIDs::OUTPUT_GAIN, -1.0f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 2.0f);
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.35f);
            setParam(ParameterIDs::CRUSH_TONE, 0.60f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.75f);
            setParam(ParameterIDs::SPACE_REVERB, 0.40f);
            setParam(ParameterIDs::SPACE_DELAY, 0.35f);
            break;

        case Underwater:
            setParam(ParameterIDs::OUTPUT_GAIN, -1.0f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 0.0f);
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.20f);
            setParam(ParameterIDs::CRUSH_TONE, 0.20f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.60f);
            setParam(ParameterIDs::SPACE_REVERB, 0.50f);
            setParam(ParameterIDs::SPACE_DELAY, 0.30f);
            setParam(ParameterIDs::EQ_LOW_GAIN, 3.0f);
            setParam(ParameterIDs::EQ_HIGH_GAIN, -8.0f);
            setParam(ParameterIDs::EQ_HIGH_CUT, 1800.0f);
            setParam(ParameterIDs::EQ_HIGH_CUT_SLOPE, 2.0f);
            break;

        case SpinningVocal:
            setParam(ParameterIDs::OUTPUT_GAIN, -0.5f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 0.0f);
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.25f);
            setParam(ParameterIDs::CRUSH_TONE, 0.65f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.90f);
            setParam(ParameterIDs::SPACE_REVERB, 0.35f);
            setParam(ParameterIDs::SPACE_DELAY, 0.25f);
            break;

        case PulsingGate:
            setParam(ParameterIDs::OUTPUT_GAIN, -1.0f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 1.0f);
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.45f);
            setParam(ParameterIDs::CRUSH_TONE, 0.75f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.70f);
            setParam(ParameterIDs::SPACE_REVERB, 0.30f);
            setParam(ParameterIDs::SPACE_DELAY, 0.30f);
            break;

        // ----------------------------------------------------
        // DELAY CATEGORY (Echoboy / Ping Pong Tape Delay)
        // ----------------------------------------------------
        case DarkSlap:
            setParam(ParameterIDs::OUTPUT_GAIN, -0.5f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 2.0f);
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.30f);
            setParam(ParameterIDs::CRUSH_TONE, 0.40f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.45f);
            setParam(ParameterIDs::SPACE_REVERB, 0.15f);
            setParam(ParameterIDs::SPACE_DELAY, 0.12f);
            setParam(ParameterIDs::EQ_HIGH_CUT, 8000.0f);
            break;

        case ThrowEcho:
            setParam(ParameterIDs::OUTPUT_GAIN, -1.0f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 0.0f);
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.20f);
            setParam(ParameterIDs::CRUSH_TONE, 0.70f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.75f);
            setParam(ParameterIDs::SPACE_REVERB, 0.35f);
            setParam(ParameterIDs::SPACE_DELAY, 0.50f);
            setParam(ParameterIDs::EQ_HIGH_GAIN, 2.5f);
            break;

        case PingPongSpace:
            setParam(ParameterIDs::OUTPUT_GAIN, -1.0f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 0.0f);
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.15f);
            setParam(ParameterIDs::CRUSH_TONE, 0.65f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.88f);
            setParam(ParameterIDs::SPACE_REVERB, 0.45f);
            setParam(ParameterIDs::SPACE_DELAY, 0.38f);
            setParam(ParameterIDs::EQ_HIGH_GAIN, 3.0f);
            break;

        case FilteredTapeDelay:
            setParam(ParameterIDs::OUTPUT_GAIN, -0.5f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 2.0f); // Studer Tape
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.40f);
            setParam(ParameterIDs::CRUSH_TONE, 0.45f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.60f);
            setParam(ParameterIDs::SPACE_REVERB, 0.25f);
            setParam(ParameterIDs::SPACE_DELAY, 0.30f);
            setParam(ParameterIDs::EQ_LOW_CUT, 200.0f);
            setParam(ParameterIDs::EQ_HIGH_CUT, 5500.0f);
            break;

        case PitchEcho:
            setParam(ParameterIDs::OUTPUT_GAIN, -1.5f);
            setParam(ParameterIDs::SHADOW_ENABLE, 1.0f);
            setParam(ParameterIDs::SHADOW_MIX, 0.35f);
            setParam(ParameterIDs::SHADOW_PITCH, 0.0f);
            setParam(ParameterIDs::SHADOW_FORMANT, 0.35f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 0.0f);
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.25f);
            setParam(ParameterIDs::CRUSH_TONE, 0.60f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.70f);
            setParam(ParameterIDs::SPACE_REVERB, 0.35f);
            setParam(ParameterIDs::SPACE_DELAY, 0.40f);
            break;

        // ----------------------------------------------------
        // RAP LEADS CATEGORY (Billboard Trap / Hyperpop / Rage)
        // ----------------------------------------------------
        case ModernRapLead:
            setParam(ParameterIDs::OUTPUT_GAIN, -0.5f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 0.0f); // Triode Tube
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.35f);
            setParam(ParameterIDs::CRUSH_TONE, 0.75f); // Air sheen
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.40f);
            setParam(ParameterIDs::SPACE_REVERB, 0.18f);
            setParam(ParameterIDs::SPACE_DELAY, 0.12f);
            setParam(ParameterIDs::EQ_LOW_CUT, 95.0f);
            setParam(ParameterIDs::EQ_LOW_GAIN, 1.5f);
            setParam(ParameterIDs::EQ_MID_GAIN, 2.5f);
            setParam(ParameterIDs::EQ_HIGH_GAIN, 4.0f);
            break;

        case RageVocal:
            setParam(ParameterIDs::OUTPUT_GAIN, -2.5f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 0.0f);
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.82f);
            setParam(ParameterIDs::CRUSH_TONE, 0.80f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.60f);
            setParam(ParameterIDs::SPACE_REVERB, 0.22f);
            setParam(ParameterIDs::SPACE_DELAY, 0.16f);
            setParam(ParameterIDs::EQ_LOW_CUT, 110.0f);
            setParam(ParameterIDs::EQ_MID_GAIN, 4.0f);
            setParam(ParameterIDs::EQ_HIGH_GAIN, 5.0f);
            break;

        case DarkPluggLead:
            setParam(ParameterIDs::OUTPUT_GAIN, -1.5f);
            setParam(ParameterIDs::SHADOW_ENABLE, 1.0f);
            setParam(ParameterIDs::SHADOW_MIX, 0.30f);
            setParam(ParameterIDs::SHADOW_PITCH, 0.0f);
            setParam(ParameterIDs::SHADOW_FORMANT, 0.32f);
            setParam(ParameterIDs::SHADOW_DARKNESS, 0.70f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 2.0f); // Tape Warmth
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.48f);
            setParam(ParameterIDs::CRUSH_TONE, 0.40f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.50f);
            setParam(ParameterIDs::SPACE_REVERB, 0.30f);
            setParam(ParameterIDs::SPACE_DELAY, 0.20f);
            setParam(ParameterIDs::EQ_LOW_CUT, 75.0f);
            setParam(ParameterIDs::EQ_LOW_GAIN, 3.0f);
            setParam(ParameterIDs::EQ_HIGH_GAIN, 1.5f);
            break;

        case IntimateTrap:
            setParam(ParameterIDs::OUTPUT_GAIN, -0.5f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 0.0f);
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.22f);
            setParam(ParameterIDs::CRUSH_TONE, 0.60f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.35f);
            setParam(ParameterIDs::SPACE_REVERB, 0.12f);
            setParam(ParameterIDs::SPACE_DELAY, 0.06f);
            setParam(ParameterIDs::EQ_LOW_CUT, 85.0f);
            setParam(ParameterIDs::EQ_HIGH_GAIN, 3.0f);
            break;

        case RawUnderground:
            setParam(ParameterIDs::OUTPUT_GAIN, -1.5f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 2.0f);
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.62f);
            setParam(ParameterIDs::CRUSH_TONE, 0.58f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.45f);
            setParam(ParameterIDs::SPACE_REVERB, 0.20f);
            setParam(ParameterIDs::SPACE_DELAY, 0.14f);
            setParam(ParameterIDs::EQ_LOW_CUT, 80.0f);
            setParam(ParameterIDs::EQ_MID_GAIN, 3.0f);
            setParam(ParameterIDs::EQ_HIGH_GAIN, 2.5f);
            break;

        // ----------------------------------------------------
        // AD-LIBS CATEGORY (Travis Scott / Playboi Carti style)
        // ----------------------------------------------------
        case MonsterAdlib:
            setParam(ParameterIDs::OUTPUT_GAIN, -2.5f);
            setParam(ParameterIDs::SHADOW_ENABLE, 1.0f);
            setParam(ParameterIDs::SHADOW_MIX, 0.68f);
            setParam(ParameterIDs::SHADOW_PITCH, 0.0f);
            setParam(ParameterIDs::SHADOW_FORMANT, 0.18f);
            setParam(ParameterIDs::SHADOW_DARKNESS, 0.60f);
            setParam(ParameterIDs::SHADOW_DRIVE, 0.60f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 3.0f);
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.75f);
            setParam(ParameterIDs::CRUSH_TONE, 0.65f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.85f);
            setParam(ParameterIDs::SPACE_REVERB, 0.45f);
            setParam(ParameterIDs::SPACE_DELAY, 0.30f);
            setParam(ParameterIDs::EQ_LOW_CUT, 80.0f);
            setParam(ParameterIDs::EQ_MID_GAIN, 3.5f);
            break;

        case TelephoneShout:
            setParam(ParameterIDs::OUTPUT_GAIN, -1.5f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 1.0f);
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.60f);
            setParam(ParameterIDs::CRUSH_TONE, 0.80f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.30f);
            setParam(ParameterIDs::SPACE_REVERB, 0.25f);
            setParam(ParameterIDs::SPACE_DELAY, 0.20f);
            setParam(ParameterIDs::DEVICE_TYPE, 1.0f);
            setParam(ParameterIDs::EQ_LOW_CUT, 450.0f);
            setParam(ParameterIDs::EQ_HIGH_CUT, 3500.0f);
            break;

        case DistanceAdlib:
            setParam(ParameterIDs::OUTPUT_GAIN, -1.5f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 0.0f);
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.20f);
            setParam(ParameterIDs::CRUSH_TONE, 0.50f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.90f);
            setParam(ParameterIDs::SPACE_REVERB, 0.72f);
            setParam(ParameterIDs::SPACE_DELAY, 0.45f);
            setParam(ParameterIDs::DEVICE_TYPE, 3.0f);
            setParam(ParameterIDs::EQ_LOW_CUT, 150.0f);
            setParam(ParameterIDs::EQ_HIGH_CUT, 8500.0f);
            break;

        case GhostLayer:
            setParam(ParameterIDs::OUTPUT_GAIN, -1.5f);
            setParam(ParameterIDs::SHADOW_ENABLE, 1.0f);
            setParam(ParameterIDs::SHADOW_MIX, 0.48f);
            setParam(ParameterIDs::SHADOW_PITCH, 0.0f);
            setParam(ParameterIDs::SHADOW_FORMANT, 0.38f);
            setParam(ParameterIDs::SHADOW_DARKNESS, 0.75f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 0.0f);
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.20f);
            setParam(ParameterIDs::CRUSH_TONE, 0.50f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.80f);
            setParam(ParameterIDs::SPACE_REVERB, 0.65f);
            setParam(ParameterIDs::SPACE_DELAY, 0.35f);
            setParam(ParameterIDs::EQ_LOW_CUT, 120.0f);
            break;

        case SpectralGhost:
            setParam(ParameterIDs::OUTPUT_GAIN, -1.5f);
            setParam(ParameterIDs::SHADOW_ENABLE, 1.0f);
            setParam(ParameterIDs::SHADOW_MIX, 0.58f);
            setParam(ParameterIDs::SHADOW_PITCH, 1.0f); // -7
            setParam(ParameterIDs::SHADOW_FORMANT, 0.18f);
            setParam(ParameterIDs::SHADOW_DARKNESS, 0.78f);
            setParam(ParameterIDs::CRUSH_CHARACTER, 1.0f);
            setParam(ParameterIDs::CRUSH_AMOUNT, 0.32f);
            setParam(ParameterIDs::CRUSH_TONE, 0.60f);
            setParam(ParameterIDs::WIDTH_AMOUNT, 0.95f);
            setParam(ParameterIDs::SPACE_REVERB, 0.78f);
            setParam(ParameterIDs::SPACE_DELAY, 0.40f);
            setParam(ParameterIDs::EQ_LOW_CUT, 100.0f);
            break;

        default:
            break;
    }
}

void PresetManager::generateRandomPreset(juce::AudioProcessorValueTreeState& apvts)
{
    auto setParam = [&](const char* id, float val) {
        if (auto* param = apvts.getParameter(id))
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost(param->convertTo0to1(val));
            param->endChangeGesture();
        }
    };

    auto& rand = juce::Random::getSystemRandom();

    // Set Preset Mode to Custom
    setParam(ParameterIDs::PRESET_MODE, 50.0f);

    // Intelligent Constrained Randomization: Musically Safe Boundaries across all modules
    float shadowMix = rand.nextBool() ? rand.nextFloat() * 0.65f : 0.0f;
    setParam(ParameterIDs::SHADOW_ENABLE, shadowMix > 0.01f ? 1.0f : 0.0f);
    setParam(ParameterIDs::SHADOW_MIX, shadowMix);
    setParam(ParameterIDs::SHADOW_PITCH, (float)rand.nextInt(4));
    setParam(ParameterIDs::SHADOW_FORMANT, 0.15f + rand.nextFloat() * 0.70f);
    setParam(ParameterIDs::SHADOW_DARKNESS, 0.20f + rand.nextFloat() * 0.70f);

    setParam(ParameterIDs::CRUSH_CHARACTER, (float)rand.nextInt(4));
    setParam(ParameterIDs::CRUSH_AMOUNT, 0.20f + rand.nextFloat() * 0.70f);
    setParam(ParameterIDs::CRUSH_TONE, 0.25f + rand.nextFloat() * 0.65f);

    setParam(ParameterIDs::WIDTH_AMOUNT, 0.30f + rand.nextFloat() * 0.65f);
    setParam(ParameterIDs::SPACE_REVERB, rand.nextFloat() * 0.55f);
    setParam(ParameterIDs::SPACE_DELAY, rand.nextFloat() * 0.45f);

    setParam(ParameterIDs::DEVICE_TYPE, rand.nextBool() ? (float)rand.nextInt(6) : 0.0f);
}

bool PresetManager::testPresetRecall(juce::AudioProcessorValueTreeState& apvts)
{
    // Automated Test: Load Preset A (DemonBelow), record state, load Preset B (WideLead), return to A, verify recall
    applyPreset(apvts, DemonBelow);
    float demonShadowMix = apvts.getParameter(ParameterIDs::SHADOW_MIX)->getValue();
    float demonCrushChar = apvts.getParameter(ParameterIDs::CRUSH_CHARACTER)->getValue();

    applyPreset(apvts, WideLead);
    float wideShadowMix = apvts.getParameter(ParameterIDs::SHADOW_MIX)->getValue();
    float wideWidthAmt = apvts.getParameter(ParameterIDs::WIDTH_AMOUNT)->getValue();

    // Re-apply DemonBelow
    applyPreset(apvts, DemonBelow);
    float recalledShadowMix = apvts.getParameter(ParameterIDs::SHADOW_MIX)->getValue();
    float recalledCrushChar = apvts.getParameter(ParameterIDs::CRUSH_CHARACTER)->getValue();

    bool pass = (std::abs(recalledShadowMix - demonShadowMix) < 0.001f)
             && (std::abs(recalledCrushChar - demonCrushChar) < 0.001f)
             && (wideShadowMix < 0.001f);

    return pass;
}
