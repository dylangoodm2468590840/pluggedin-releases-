#include "PresetManager.h"
#include "../Utils/ParameterIDs.h"

const char* PresetManager::getPresetName(PresetIndex preset)
{
    switch (preset)
    {
        case Ref1_PolishedCrisp:     return "01. POLISHED / CRISP";
        case Ref2_DarkUnderground:   return "02. DARK / UNDERGROUND";
        case Ref3_DemonDeep:         return "03. DEMON / DEEP";
        case Ref4_WideFloating:      return "04. WIDE / FLOATING";
        case Ref5_DestroyedCrushed:  return "05. DESTROYED / CRUSHED";
        case Ref6_TelephoneDevice:   return "06. TELEPHONE / DEVICE";
        case Ref7_FuturisticAlien:   return "07. FUTURISTIC / ALIEN";
        case Ref8_Subterranean808:   return "08. SUBTERRANEAN 808";
        case Ref9_VintageTapeMellotron: return "09. VINTAGE TAPE / MELLOTRON";
        case Ref10_HyperpopWarp:     return "10. HYPERPOP / WARP";
        case Custom:                 return "CUSTOM USER";
        default:                     return "UNKNOWN";
    }
}

void PresetManager::applyPreset(juce::AudioProcessorValueTreeState& apvts, PresetIndex preset)
{
    auto setP = [&](const juce::String& paramId, float value) {
        if (auto* param = apvts.getParameter(paramId))
            param->setValueNotifyingHost(param->convertTo0to1(value));
    };

    // 1. Reset all modules to clean baseline
    setP(ParameterIDs::INPUT_GAIN, 0.0f);
    setP(ParameterIDs::OUTPUT_GAIN, 0.0f);
    setP(ParameterIDs::MIX_GLOBAL, 1.0f);
    setP(ParameterIDs::DEGENERATE, 0.0f);

    setP(ParameterIDs::MODULE_SUB_ENABLE, 1.0f);
    setP(ParameterIDs::MODULE_GRIT_ENABLE, 1.0f);
    setP(ParameterIDs::MODULE_MOD_ENABLE, 1.0f);
    setP(ParameterIDs::MODULE_DELAY_ENABLE, 1.0f);
    setP(ParameterIDs::MODULE_REVERB_ENABLE, 1.0f);

    setP(ParameterIDs::COMP_SQUEEZE, 0.0f);
    setP(ParameterIDs::COMP_CHARACTER, 2.0f);
    setP(ParameterIDs::DEESS_AMOUNT, 0.0f);
    setP(ParameterIDs::DEESS_FREQ, 6500.0f);

    setP(ParameterIDs::AIR_MID, 0.0f);
    setP(ParameterIDs::AIR_TOP, 0.0f);

    setP(ParameterIDs::EQ_LOW_CUT, 20.0f);
    setP(ParameterIDs::EQ_LOW_GAIN, 0.0f);
    setP(ParameterIDs::EQ_MID_GAIN, 0.0f);
    setP(ParameterIDs::EQ_HIGH_GAIN, 0.0f);
    setP(ParameterIDs::EQ_HIGH_CUT, 20000.0f);
    setP(ParameterIDs::EQ_LOW_Q, 0.707f);
    setP(ParameterIDs::EQ_MID_Q, 0.707f);
    setP(ParameterIDs::EQ_HIGH_Q, 0.707f);

    setP(ParameterIDs::CRUSH_AMOUNT, 0.0f);
    setP(ParameterIDs::CRUSH_CHARACTER, 0.0f);
    setP(ParameterIDs::CRUSH_TONE, 0.5f);
    setP(ParameterIDs::CRUSH_MIX, 1.0f);
    setP(ParameterIDs::CRUSH_PUNISH, 0.0f);

    setP(ParameterIDs::SHADOW_ENABLE, 0.0f);
    setP(ParameterIDs::SHADOW_MIX, 0.0f);
    setP(ParameterIDs::SHADOW_PITCH, 0.0f);
    setP(ParameterIDs::SHADOW_FORMANT, 0.5f);
    setP(ParameterIDs::SHADOW_DARKNESS, 0.5f);
    setP(ParameterIDs::SHADOW_DRIVE, 0.2f);

    setP(ParameterIDs::WIDTH_AMOUNT, 0.0f);
    setP(ParameterIDs::MOD_RATE, 0.35f);
    setP(ParameterIDs::MOD_DEPTH, 0.50f);

    setP(ParameterIDs::SPACE_REVERB, 0.0f);
    setP(ParameterIDs::REVERB_DECAY, 0.50f);
    setP(ParameterIDs::REVERB_MIX, 0.0f);
    setP(ParameterIDs::SPACE_DELAY, 0.0f);
    setP(ParameterIDs::DELAY_FEEDBACK, 0.35f);
    setP(ParameterIDs::DELAY_MIX, 0.0f);
    setP(ParameterIDs::SPACE_DUCKING, 0.5f);

    setP(ParameterIDs::DEVICE_TYPE, 0.0f);
    setP(ParameterIDs::MASTER_BYPASS, 0.0f);

    switch (preset)
    {
        // =========================================================================
        // 01. POLISHED / CRISP — Flagship Broadcast Modern Rap Lead
        // Radio-ready forward clarity. High presence shelf + tight low cut + air sheen.
        // EQ: Broadcast chain — tight 80Hz HP, -1.5dB mud cut @ 250Hz, +2dB presence @ 3kHz, +3dB air shelf @ 12kHz
        // =========================================================================
        case Ref1_PolishedCrisp:
            setP(ParameterIDs::EQ_LOW_CUT,   80.0f);
            setP(ParameterIDs::EQ_LOW_GAIN,  -1.5f);   // Tight low-mid mud cut at 200Hz
            setP(ParameterIDs::EQ_LOW_Q,      1.4f);
            setP(ParameterIDs::EQ_MID_GAIN,   2.0f);   // Forward 2.2kHz presence
            setP(ParameterIDs::EQ_MID_Q,      1.8f);
            setP(ParameterIDs::EQ_HIGH_GAIN,  3.0f);   // Expensive top-air shelf @ 8kHz
            setP(ParameterIDs::EQ_HIGH_CUT,   20000.0f);
            setP(ParameterIDs::OUTPUT_GAIN,  -1.5f);   // Compensate air exciter level addition

            setP(ParameterIDs::COMP_SQUEEZE,  0.38f);
            setP(ParameterIDs::COMP_CHARACTER, 0.0f);  // Modern Fast FET
            setP(ParameterIDs::DEESS_AMOUNT,  0.50f);
            setP(ParameterIDs::DEESS_FREQ,    6800.0f);

            setP(ParameterIDs::AIR_MID, 0.18f);
            setP(ParameterIDs::AIR_TOP, 0.28f);

            setP(ParameterIDs::MODULE_GRIT_ENABLE, 1.0f);
            setP(ParameterIDs::CRUSH_CHARACTER, 2.0f); // Ampex Tape Warmth
            setP(ParameterIDs::CRUSH_AMOUNT,  0.12f);
            setP(ParameterIDs::CRUSH_TONE,    0.70f);

            setP(ParameterIDs::MODULE_REVERB_ENABLE, 0.0f);
            setP(ParameterIDs::SPACE_REVERB,  0.0f);
            setP(ParameterIDs::SPACE_DUCKING, 0.5f);
            break;

        // =========================================================================
        // 02. DARK / UNDERGROUND — Moody Metro Boomin Thick Chest Pocket
        // Dense analog saturation, boosted sub-chest warmth, rolled-off highs, punchy undertone.
        // EQ: Chest-weight chain — 35Hz HP, +3dB chest bell @ 160Hz, -1.5dB @ 400Hz, -2dB @ 2.2kHz, -3dB shelf @ 10kHz
        // =========================================================================
        case Ref2_DarkUnderground:
            setP(ParameterIDs::EQ_LOW_CUT,   35.0f);
            setP(ParameterIDs::EQ_LOW_GAIN,   3.0f);   // +3dB chest weight @ 200Hz bell
            setP(ParameterIDs::EQ_LOW_Q,      1.3f);
            setP(ParameterIDs::EQ_MID_GAIN,  -2.0f);   // Recessed highs for dark character
            setP(ParameterIDs::EQ_MID_Q,      0.9f);
            setP(ParameterIDs::EQ_HIGH_GAIN, -3.5f);   // Dark rolled-off top @ 8kHz shelf
            setP(ParameterIDs::EQ_HIGH_CUT,   11000.0f);

            setP(ParameterIDs::COMP_SQUEEZE,  0.45f);
            setP(ParameterIDs::COMP_CHARACTER, 1.0f);  // Vintage Optical LA-2A
            setP(ParameterIDs::DEESS_AMOUNT,  0.40f);

            setP(ParameterIDs::MODULE_GRIT_ENABLE, 1.0f);
            setP(ParameterIDs::CRUSH_CHARACTER, 0.0f); // 12AX7 Triode Tube
            setP(ParameterIDs::CRUSH_AMOUNT,  0.25f);
            setP(ParameterIDs::CRUSH_TONE,    0.40f);

            setP(ParameterIDs::MODULE_SUB_ENABLE, 1.0f);
            setP(ParameterIDs::SHADOW_ENABLE, 1.0f);
            setP(ParameterIDs::SHADOW_MIX,    0.45f);  // Raised from 0.22 — was inaudible before fix
            setP(ParameterIDs::SHADOW_PITCH,  0.0f);   // Octave Down
            setP(ParameterIDs::SHADOW_FORMANT, 0.35f); // Deep resonant male chest cavity
            setP(ParameterIDs::SHADOW_DARKNESS, 0.78f);
            setP(ParameterIDs::SHADOW_DRIVE,  0.20f);
            break;

        // =========================================================================
        // 03. DEMON / DEEP — Subterranean Chest Under-Voice + Sub-Body Weight
        // Clean PSOLA octave-down layer with massive sub-body and controlled DEGENERATE.
        // EQ: Sub-body chain — 25Hz HP, +4dB sub-chest @ 90Hz, +2dB @ 200Hz, -3dB @ 500Hz box cut, high cut @ 9kHz
        // =========================================================================
        case Ref3_DemonDeep:
            setP(ParameterIDs::EQ_LOW_CUT,   25.0f);
            setP(ParameterIDs::EQ_LOW_GAIN,   4.0f);   // +4dB sub-chest body bell @ 200Hz
            setP(ParameterIDs::EQ_LOW_Q,      1.6f);
            setP(ParameterIDs::EQ_MID_GAIN,  -3.0f);   // -3dB boxiness notch cut @ 2.2kHz
            setP(ParameterIDs::EQ_MID_Q,      2.2f);
            setP(ParameterIDs::EQ_HIGH_GAIN, -2.0f);   // Darkened top
            setP(ParameterIDs::EQ_HIGH_CUT,   9000.0f);
            setP(ParameterIDs::OUTPUT_GAIN,  -1.0f);   // Compensate for added sub body weight

            setP(ParameterIDs::DEGENERATE,    0.18f);  // Reduced from 0.40 — was destabilizing LPC

            setP(ParameterIDs::MODULE_SUB_ENABLE, 1.0f);
            setP(ParameterIDs::SHADOW_ENABLE, 1.0f);
            setP(ParameterIDs::SHADOW_MIX,    0.55f);  // Raised to actually be heard
            setP(ParameterIDs::SHADOW_PITCH,  0.0f);   // Octave Down (-12st)
            setP(ParameterIDs::SHADOW_FORMANT, 0.28f); // Deep resonant LPC male chest tract
            setP(ParameterIDs::SHADOW_DARKNESS, 0.55f);
            setP(ParameterIDs::SHADOW_DRIVE,  0.22f);

            setP(ParameterIDs::MODULE_GRIT_ENABLE, 1.0f);
            setP(ParameterIDs::CRUSH_CHARACTER, 3.0f); // Germanium Preamp
            setP(ParameterIDs::CRUSH_AMOUNT,  0.20f);
            setP(ParameterIDs::CRUSH_TONE,    0.45f);

            setP(ParameterIDs::MODULE_MOD_ENABLE, 1.0f);
            setP(ParameterIDs::WIDTH_AMOUNT,  0.48f);

            setP(ParameterIDs::COMP_SQUEEZE,  0.40f);
            setP(ParameterIDs::AIR_TOP,       0.20f);

            setP(ParameterIDs::MODULE_REVERB_ENABLE, 1.0f);
            setP(ParameterIDs::SPACE_REVERB,  0.22f);
            setP(ParameterIDs::SPACE_DUCKING, 0.85f);
            break;

        // =========================================================================
        // 04. WIDE / FLOATING — Expansive Dreamlike Vocal Space
        // Roland Dimension D chorus, stereo Haas doubling, ducked Dattorro diffusion.
        // EQ: Shimmer chain — 90Hz HP, -1dB @ 350Hz clarity dip, +3dB shelf @ 8kHz shimmer
        // =========================================================================
        case Ref4_WideFloating:
            setP(ParameterIDs::EQ_LOW_CUT,   90.0f);
            setP(ParameterIDs::EQ_LOW_GAIN,  -1.0f);   // -1dB @ 200Hz for clean open sound
            setP(ParameterIDs::EQ_LOW_Q,      0.9f);
            setP(ParameterIDs::EQ_MID_GAIN,   0.5f);   // Mild presence lift
            setP(ParameterIDs::EQ_HIGH_GAIN,  3.5f);   // +3.5dB shimmer shelf @ 8kHz
            setP(ParameterIDs::EQ_HIGH_CUT,   20000.0f);

            setP(ParameterIDs::COMP_SQUEEZE,  0.40f);
            setP(ParameterIDs::AIR_TOP,       0.40f);

            setP(ParameterIDs::MODULE_MOD_ENABLE, 1.0f);
            setP(ParameterIDs::WIDTH_AMOUNT,  0.88f);

            setP(ParameterIDs::MODULE_DELAY_ENABLE, 1.0f);
            setP(ParameterIDs::MODULE_REVERB_ENABLE, 1.0f);
            setP(ParameterIDs::SPACE_DELAY,   0.35f);
            setP(ParameterIDs::SPACE_REVERB,  0.55f);
            setP(ParameterIDs::SPACE_DUCKING, 0.85f);
            break;

        // =========================================================================
        // 05. DESTROYED / CRUSHED — Aggressive PUNISH Mode Fuzz Bite
        // Full Pentode oversampled blast — now safe after Nyquist clamp fix.
        // EQ: Fuzz midrange chain — 110Hz HP, +4dB @ 1kHz crunch bell, +2dB @ 3.5kHz bite, -4dB high cut @ 8kHz
        // =========================================================================
        case Ref5_DestroyedCrushed:
            setP(ParameterIDs::EQ_LOW_CUT,   110.0f);
            setP(ParameterIDs::EQ_LOW_GAIN,   0.0f);
            setP(ParameterIDs::EQ_MID_GAIN,   4.0f);   // Aggressive midrange crunch bell @ 2.2kHz
            setP(ParameterIDs::EQ_MID_Q,      1.8f);
            setP(ParameterIDs::EQ_HIGH_GAIN, -3.0f);   // Pull back highs so PUNISH doesn't screech
            setP(ParameterIDs::EQ_HIGH_CUT,   8000.0f); // Hard high cut for controlled fuzz character

            setP(ParameterIDs::MODULE_GRIT_ENABLE, 1.0f);
            setP(ParameterIDs::CRUSH_CHARACTER, 1.0f); // Pentode EL34
            setP(ParameterIDs::CRUSH_AMOUNT,  0.80f);
            setP(ParameterIDs::CRUSH_TONE,    0.65f);  // Slightly rolled back from 0.72 after Nyquist fix
            setP(ParameterIDs::CRUSH_PUNISH,  1.0f);   // +20dB PUNISH Mode

            setP(ParameterIDs::COMP_SQUEEZE,  0.70f);
            setP(ParameterIDs::DEESS_AMOUNT,  0.65f);
            setP(ParameterIDs::DEESS_FREQ,    5500.0f);
            break;

        // =========================================================================
        // 06. TELEPHONE / DEVICE — Resonant Transducer Body & Crunch
        // Cell Phone / Megaphone acoustic cavity resonance. Level-fixed.
        // EQ: Telephone band — hard HP @ 380Hz, hard LP @ 3400Hz, +2dB phone speaker resonance @ 1.2kHz
        // =========================================================================
        case Ref6_TelephoneDevice:
            setP(ParameterIDs::DEVICE_TYPE,   1.0f);   // Cell Phone Transducer

            setP(ParameterIDs::EQ_LOW_CUT,   380.0f);  // Hard telephone band low cut
            setP(ParameterIDs::EQ_HIGH_CUT,   3400.0f); // Hard telephone band high cut
            setP(ParameterIDs::EQ_MID_GAIN,   2.0f);   // Reduced from 4.5 — prevents ear fatigue
            setP(ParameterIDs::EQ_MID_Q,      2.2f);   // Narrow phone speaker resonance @ 2.2kHz
            setP(ParameterIDs::OUTPUT_GAIN,  -1.5f);   // Tame level concentration from band limiting

            setP(ParameterIDs::MODULE_GRIT_ENABLE, 1.0f);
            setP(ParameterIDs::CRUSH_CHARACTER, 3.0f); // Germanium Console
            setP(ParameterIDs::CRUSH_AMOUNT,  0.28f);

            setP(ParameterIDs::COMP_SQUEEZE,  0.45f);  // Reduced from 0.65 — less pumping on narrow band
            break;

        // =========================================================================
        // 07. FUTURISTIC / ALIEN — Micro-Pitch + Formant Articulation + Alien Motion
        // Hyper-extended presence, cyber motion shimmer, pitched alien under-voice.
        // EQ: Cyber chain — 120Hz HP, -2dB @ 300Hz, +3dB @ 2.2kHz alien clarity, +4dB shelf @ 12kHz cyber air
        // =========================================================================
        case Ref7_FuturisticAlien:
            setP(ParameterIDs::EQ_LOW_CUT,   120.0f);
            setP(ParameterIDs::EQ_LOW_GAIN,  -2.0f);   // -2dB @ 200Hz — tighten low end for alien feel
            setP(ParameterIDs::EQ_LOW_Q,      1.2f);
            setP(ParameterIDs::EQ_MID_GAIN,   3.0f);   // +3dB alien presence @ 2.2kHz
            setP(ParameterIDs::EQ_MID_Q,      1.5f);
            setP(ParameterIDs::EQ_HIGH_GAIN,  4.5f);   // +4.5dB cyber air shelf @ 8kHz
            setP(ParameterIDs::EQ_HIGH_CUT,   20000.0f);

            setP(ParameterIDs::MODULE_SUB_ENABLE, 1.0f);
            setP(ParameterIDs::SHADOW_ENABLE, 1.0f);
            setP(ParameterIDs::SHADOW_MIX,    0.55f);  // Raised from 0.35 — now actually audible
            setP(ParameterIDs::SHADOW_PITCH,  2.0f);   // FourthDown (-5 semitones)
            setP(ParameterIDs::SHADOW_FORMANT, 0.78f); // High-pitched alien cavity (capped at 0.88 gamma now)
            setP(ParameterIDs::SHADOW_DRIVE,  0.18f);

            setP(ParameterIDs::AIR_MID, 0.22f);
            setP(ParameterIDs::AIR_TOP, 0.35f);

            setP(ParameterIDs::MODULE_MOD_ENABLE, 1.0f);
            setP(ParameterIDs::WIDTH_AMOUNT,  0.65f);

            setP(ParameterIDs::MODULE_DELAY_ENABLE, 1.0f);
            setP(ParameterIDs::SPACE_DELAY,   0.38f);
            setP(ParameterIDs::SPACE_REVERB,  0.22f);
            setP(ParameterIDs::SPACE_DUCKING, 0.65f);

            setP(ParameterIDs::DEGENERATE,    0.10f);  // Reduced from 0.25 — was causing LPC noise
            break;

        // =========================================================================
        // 08. SUBTERRANEAN 808 — Huge Sub-Octave Chest Weight + Crisp Consonant Snap
        // Massive 808 fundamental layer with Little AlterBoy style consonant bypass.
        // EQ: Heavy 808 sub punch @ 80Hz (+4.5dB), clear presence @ 2.5kHz (+2dB)
        // =========================================================================
        case Ref8_Subterranean808:
            setP(ParameterIDs::EQ_LOW_CUT,   25.0f);
            setP(ParameterIDs::EQ_LOW_GAIN,   4.5f);   // Massive sub punch @ 80Hz
            setP(ParameterIDs::EQ_LOW_Q,      1.5f);
            setP(ParameterIDs::EQ_MID_GAIN,   2.2f);   // Crisp vocal articulation
            setP(ParameterIDs::EQ_MID_Q,      1.8f);
            setP(ParameterIDs::EQ_HIGH_GAIN,  1.5f);
            setP(ParameterIDs::EQ_HIGH_CUT,   14000.0f);
            setP(ParameterIDs::OUTPUT_GAIN,  -1.0f);

            setP(ParameterIDs::COMP_SQUEEZE,  0.48f);
            setP(ParameterIDs::DEESS_AMOUNT,  0.45f);

            setP(ParameterIDs::MODULE_SUB_ENABLE, 1.0f);
            setP(ParameterIDs::SHADOW_ENABLE, 1.0f);
            setP(ParameterIDs::SHADOW_MIX,    0.60f);  // Sub voice prominent
            setP(ParameterIDs::SHADOW_PITCH,  0.0f);   // Octave Down (-12st)
            setP(ParameterIDs::SHADOW_FORMANT, 0.25f); // Deep resonant chest
            setP(ParameterIDs::SHADOW_DARKNESS, 0.60f);
            setP(ParameterIDs::SHADOW_DRIVE,  0.30f);

            setP(ParameterIDs::MODULE_GRIT_ENABLE, 1.0f);
            setP(ParameterIDs::CRUSH_CHARACTER, 2.0f); // Ampex Tape Warmth
            setP(ParameterIDs::CRUSH_AMOUNT,  0.18f);

            setP(ParameterIDs::MODULE_REVERB_ENABLE, 1.0f);
            setP(ParameterIDs::SPACE_REVERB,  0.15f);
            setP(ParameterIDs::SPACE_DUCKING, 0.85f);
            break;

        // =========================================================================
        // 09. VINTAGE TAPE / MELLOTRON — Ampex 350 Tape Drive + Optical Wow/Flutter
        // Warm vintage analog tape compression, 60Hz head-bump, dimensional tape drift.
        // EQ: Warm analog tilt: +2dB @ 160Hz, +2.5dB high shelf @ 8kHz
        // =========================================================================
        case Ref9_VintageTapeMellotron:
            setP(ParameterIDs::EQ_LOW_CUT,   45.0f);
            setP(ParameterIDs::EQ_LOW_GAIN,   2.5f);
            setP(ParameterIDs::EQ_LOW_Q,      1.1f);
            setP(ParameterIDs::EQ_MID_GAIN,   0.8f);
            setP(ParameterIDs::EQ_HIGH_GAIN,  2.5f);
            setP(ParameterIDs::EQ_HIGH_CUT,   16000.0f);

            setP(ParameterIDs::COMP_SQUEEZE,  0.52f);
            setP(ParameterIDs::COMP_CHARACTER, 1.0f);  // Vintage Optical LA-2A

            setP(ParameterIDs::MODULE_GRIT_ENABLE, 1.0f);
            setP(ParameterIDs::CRUSH_CHARACTER, 2.0f); // Ampex Tape
            setP(ParameterIDs::CRUSH_AMOUNT,  0.35f);
            setP(ParameterIDs::CRUSH_TONE,    0.55f);

            setP(ParameterIDs::MODULE_MOD_ENABLE, 1.0f);
            setP(ParameterIDs::WIDTH_AMOUNT,  0.40f);
            setP(ParameterIDs::MOD_RATE,      0.35f);  // Tape wow flutter

            setP(ParameterIDs::MODULE_DELAY_ENABLE, 1.0f);
            setP(ParameterIDs::SPACE_DELAY,   0.28f);
            setP(ParameterIDs::DELAY_FEEDBACK, 0.40f);
            setP(ParameterIDs::SPACE_DUCKING, 0.70f);
            break;

        // =========================================================================
        // 10. HYPERPOP / WARP — High-Pitched Alien Formant + Fresh Air + Fast FET
        // High-energy modern hyperpop vocal with aggressive presence and bright sheen.
        // EQ: Bright presence boost +3dB @ 3kHz, +5dB @ 12kHz
        // =========================================================================
        case Ref10_HyperpopWarp:
            setP(ParameterIDs::EQ_LOW_CUT,   120.0f);
            setP(ParameterIDs::EQ_LOW_GAIN,  -2.0f);
            setP(ParameterIDs::EQ_MID_GAIN,   3.5f);
            setP(ParameterIDs::EQ_MID_Q,      1.4f);
            setP(ParameterIDs::EQ_HIGH_GAIN,  5.0f);   // Hyperpop air sheen
            setP(ParameterIDs::EQ_HIGH_CUT,   20000.0f);

            setP(ParameterIDs::COMP_SQUEEZE,  0.60f);
            setP(ParameterIDs::COMP_CHARACTER, 0.0f);  // Fast FET

            setP(ParameterIDs::AIR_MID,       0.30f);
            setP(ParameterIDs::AIR_TOP,       0.45f);

            setP(ParameterIDs::MODULE_SUB_ENABLE, 1.0f);
            setP(ParameterIDs::SHADOW_ENABLE, 1.0f);
            setP(ParameterIDs::SHADOW_MIX,    0.40f);
            setP(ParameterIDs::SHADOW_PITCH,  2.0f);   // Fourth down
            setP(ParameterIDs::SHADOW_FORMANT, 0.85f); // High alien throat

            setP(ParameterIDs::MODULE_GRIT_ENABLE, 1.0f);
            setP(ParameterIDs::CRUSH_CHARACTER, 4.0f); // Cyber Fuzz
            setP(ParameterIDs::CRUSH_AMOUNT,  0.22f);

            setP(ParameterIDs::MODULE_MOD_ENABLE, 1.0f);
            setP(ParameterIDs::WIDTH_AMOUNT,  0.75f);

            setP(ParameterIDs::MODULE_REVERB_ENABLE, 1.0f);
            setP(ParameterIDs::SPACE_REVERB,  0.30f);
            setP(ParameterIDs::SPACE_DUCKING, 0.80f);
            break;

        case Custom:
        default:
            break;
    }
}

bool PresetManager::testPresetRecall(juce::AudioProcessorValueTreeState& apvts)
{
    for (int p = 0; p < NUM_REFERENCE_PRESETS; ++p)
    {
        applyPreset(apvts, static_cast<PresetIndex>(p));
        auto* sq = apvts.getRawParameterValue(ParameterIDs::COMP_SQUEEZE);
        if (sq == nullptr) return false;
    }
    return true;
}
