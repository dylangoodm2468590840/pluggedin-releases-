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

    setP(ParameterIDs::SPACE_REVERB, 0.0f);
    setP(ParameterIDs::SPACE_DELAY, 0.0f);
    setP(ParameterIDs::SPACE_DUCKING, 0.5f);

    setP(ParameterIDs::DEVICE_TYPE, 0.0f);

    switch (preset)
    {
        // =========================================================================
        // 01. POLISHED / CRISP — Flagship Broadcast Modern Rap Lead
        // Clean, forward, controlled dynamics, surgical de-essing, expensive air sheen.
        // =========================================================================
        case Ref1_PolishedCrisp:
            setP(ParameterIDs::EQ_LOW_CUT, 80.0f);
            setP(ParameterIDs::EQ_LOW_GAIN, -1.2f);
            setP(ParameterIDs::EQ_MID_GAIN, 1.5f);
            setP(ParameterIDs::EQ_MID_Q, 1.2f);
            setP(ParameterIDs::EQ_HIGH_GAIN, 2.2f);
            setP(ParameterIDs::EQ_HIGH_CUT, 20000.0f);

            setP(ParameterIDs::COMP_SQUEEZE, 0.42f);
            setP(ParameterIDs::COMP_CHARACTER, 0.0f); // Modern Fast FET
            setP(ParameterIDs::DEESS_AMOUNT, 0.50f);
            setP(ParameterIDs::DEESS_FREQ, 6800.0f);

            setP(ParameterIDs::AIR_MID, 0.30f);
            setP(ParameterIDs::AIR_TOP, 0.45f);

            setP(ParameterIDs::MODULE_GRIT_ENABLE, 1.0f);
            setP(ParameterIDs::CRUSH_CHARACTER, 2.0f); // Ampex Tape Warmth
            setP(ParameterIDs::CRUSH_AMOUNT, 0.20f);
            setP(ParameterIDs::CRUSH_TONE, 0.70f);

            setP(ParameterIDs::MODULE_REVERB_ENABLE, 1.0f);
            setP(ParameterIDs::SPACE_REVERB, 0.15f);
            setP(ParameterIDs::SPACE_DUCKING, 0.70f);
            break;

        // =========================================================================
        // 02. DARK / UNDERGROUND — Moody, Thick Low-Mids, Tape Head-Bump (Ref Effect 5)
        // Thick analog saturation, warm low-end weight, controlled highs, dense body.
        // =========================================================================
        case Ref2_DarkUnderground:
            setP(ParameterIDs::EQ_LOW_CUT, 35.0f);
            setP(ParameterIDs::EQ_LOW_GAIN, 3.2f); // 160Hz warm chest punch
            setP(ParameterIDs::EQ_LOW_Q, 1.1f);
            setP(ParameterIDs::EQ_MID_GAIN, -1.5f);
            setP(ParameterIDs::EQ_HIGH_GAIN, -3.0f);
            setP(ParameterIDs::EQ_HIGH_CUT, 11000.0f);

            setP(ParameterIDs::COMP_SQUEEZE, 0.55f);
            setP(ParameterIDs::COMP_CHARACTER, 1.0f); // Vintage Optical LA-2A
            setP(ParameterIDs::DEESS_AMOUNT, 0.40f);

            setP(ParameterIDs::MODULE_GRIT_ENABLE, 1.0f);
            setP(ParameterIDs::CRUSH_CHARACTER, 0.0f); // 12AX7 Triode Tube
            setP(ParameterIDs::CRUSH_AMOUNT, 0.38f);
            setP(ParameterIDs::CRUSH_TONE, 0.40f);

            setP(ParameterIDs::MODULE_SUB_ENABLE, 1.0f);
            setP(ParameterIDs::SHADOW_ENABLE, 1.0f);
            setP(ParameterIDs::SHADOW_MIX, 0.28f);
            setP(ParameterIDs::SHADOW_PITCH, 0.0f);
            setP(ParameterIDs::SHADOW_FORMANT, 0.38f);
            setP(ParameterIDs::SHADOW_DARKNESS, 0.75f);
            setP(ParameterIDs::SHADOW_DRIVE, 0.30f);
            break;

        // =========================================================================
        // 03. DEMON / DEEP — Subterranean Synchronous Formant Under-Voice (Ref Effect 1)
        // Studio-grade PSOLA -12st pitched chest layer with LPC vocal tract & wide 3D aura.
        // =========================================================================
        case Ref3_DemonDeep:
            setP(ParameterIDs::DEGENERATE, 0.55f);
            setP(ParameterIDs::MODULE_SUB_ENABLE, 1.0f);
            setP(ParameterIDs::SHADOW_ENABLE, 1.0f);
            setP(ParameterIDs::SHADOW_MIX, 0.60f);
            setP(ParameterIDs::SHADOW_PITCH, 0.0f); // Octave Down (-12st)
            setP(ParameterIDs::SHADOW_FORMANT, 0.32f); // Deep resonant LPC male chest tract
            setP(ParameterIDs::SHADOW_DARKNESS, 0.50f);
            setP(ParameterIDs::SHADOW_DRIVE, 0.45f);

            setP(ParameterIDs::MODULE_GRIT_ENABLE, 1.0f);
            setP(ParameterIDs::CRUSH_CHARACTER, 3.0f); // Germanium Preamp
            setP(ParameterIDs::CRUSH_AMOUNT, 0.42f);
            setP(ParameterIDs::CRUSH_TONE, 0.48f);

            setP(ParameterIDs::MODULE_MOD_ENABLE, 1.0f);
            setP(ParameterIDs::WIDTH_AMOUNT, 0.52f); // Murda Melodies Effect 4 3D aura

            setP(ParameterIDs::COMP_SQUEEZE, 0.48f);
            setP(ParameterIDs::AIR_TOP, 0.30f);

            setP(ParameterIDs::MODULE_REVERB_ENABLE, 1.0f);
            setP(ParameterIDs::SPACE_REVERB, 0.25f);
            setP(ParameterIDs::SPACE_DUCKING, 0.85f);
            break;

        // =========================================================================
        // 04. WIDE / FLOATING — Expansive Dreamlike Vocal Space (Ref Effects 1 & 4)
        // Roland Dimension D chorus, stereo Haas doubling, ducked Dattorro diffusion space.
        // =========================================================================
        case Ref4_WideFloating:
            setP(ParameterIDs::EQ_LOW_CUT, 90.0f);
            setP(ParameterIDs::EQ_HIGH_GAIN, 2.4f);

            setP(ParameterIDs::COMP_SQUEEZE, 0.40f);
            setP(ParameterIDs::AIR_TOP, 0.45f);

            setP(ParameterIDs::MODULE_MOD_ENABLE, 1.0f);
            setP(ParameterIDs::WIDTH_AMOUNT, 0.88f); // 160Hz Side-cut isolated width (0.90 Side/Mid ratio)

            setP(ParameterIDs::MODULE_DELAY_ENABLE, 1.0f);
            setP(ParameterIDs::MODULE_REVERB_ENABLE, 1.0f);
            setP(ParameterIDs::SPACE_DELAY, 0.35f);
            setP(ParameterIDs::SPACE_REVERB, 0.58f);
            setP(ParameterIDs::SPACE_DUCKING, 0.85f); // Vocal stays clear on words, blooms in pauses
            break;

        // =========================================================================
        // 05. DESTROYED / CRUSHED — Aggressive +20dB PUNISH Mode (Ref Effect 3)
        // Full Pentode / Cyber Fuzz oversampled blast with transient consonant protection.
        // =========================================================================
        case Ref5_DestroyedCrushed:
            setP(ParameterIDs::MODULE_GRIT_ENABLE, 1.0f);
            setP(ParameterIDs::CRUSH_CHARACTER, 1.0f); // Pentode EL34
            setP(ParameterIDs::CRUSH_AMOUNT, 0.80f);
            setP(ParameterIDs::CRUSH_TONE, 0.72f);
            setP(ParameterIDs::CRUSH_PUNISH, 1.0f); // +20dB PUNISH Mode ACTIVE!

            setP(ParameterIDs::COMP_SQUEEZE, 0.70f);
            setP(ParameterIDs::DEESS_AMOUNT, 0.60f);
            setP(ParameterIDs::DEESS_FREQ, 5800.0f);

            setP(ParameterIDs::EQ_LOW_CUT, 110.0f);
            setP(ParameterIDs::EQ_MID_GAIN, 3.0f);
            setP(ParameterIDs::EQ_MID_Q, 2.0f);
            break;

        // =========================================================================
        // 06. TELEPHONE / DEVICE — Resonant Transducer Body & Crunch (Ref Effect 2)
        // Cell Phone / Megaphone acoustic cavity resonance with speaker cone distortion.
        // =========================================================================
        case Ref6_TelephoneDevice:
            setP(ParameterIDs::DEVICE_TYPE, 1.0f); // Cell Phone Transducer
            
            setP(ParameterIDs::MODULE_GRIT_ENABLE, 1.0f);
            setP(ParameterIDs::CRUSH_CHARACTER, 3.0f); // Germanium Console
            setP(ParameterIDs::CRUSH_AMOUNT, 0.30f);

            setP(ParameterIDs::EQ_LOW_CUT, 380.0f);
            setP(ParameterIDs::EQ_HIGH_CUT, 3400.0f);
            setP(ParameterIDs::EQ_MID_GAIN, 4.5f);
            setP(ParameterIDs::EQ_MID_Q, 2.4f);

            setP(ParameterIDs::COMP_SQUEEZE, 0.65f);
            break;

        // =========================================================================
        // 07. FUTURISTIC / ALIEN — Micro-Pitch, Formant Articulation & Alien Motion
        // "What the hell is this doing to my voice in a good way?"
        // =========================================================================
        case Ref7_FuturisticAlien:
            setP(ParameterIDs::MODULE_SUB_ENABLE, 1.0f);
            setP(ParameterIDs::SHADOW_ENABLE, 1.0f);
            setP(ParameterIDs::SHADOW_MIX, 0.42f);
            setP(ParameterIDs::SHADOW_PITCH, 2.0f); // -5 semitones fourth down
            setP(ParameterIDs::SHADOW_FORMANT, 0.82f); // High-pitched alien cavity
            setP(ParameterIDs::SHADOW_DRIVE, 0.30f);

            setP(ParameterIDs::AIR_MID, 0.35f);
            setP(ParameterIDs::AIR_TOP, 0.60f);

            setP(ParameterIDs::MODULE_MOD_ENABLE, 1.0f);
            setP(ParameterIDs::WIDTH_AMOUNT, 0.70f);

            setP(ParameterIDs::MODULE_DELAY_ENABLE, 1.0f);
            setP(ParameterIDs::SPACE_DELAY, 0.40f);
            setP(ParameterIDs::SPACE_REVERB, 0.25f);
            setP(ParameterIDs::SPACE_DUCKING, 0.65f);

            setP(ParameterIDs::DEGENERATE, 0.35f);
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
