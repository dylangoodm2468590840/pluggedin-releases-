#pragma once

namespace ParameterIDs
{
    // Global & Signature Macro
    inline constexpr auto DEGENERATE       = "degenerate";
    inline constexpr auto PRESET_MODE      = "preset_mode";
    inline constexpr auto INPUT_GAIN       = "input_gain";
    inline constexpr auto OUTPUT_GAIN      = "output_gain";
    inline constexpr auto MIX_GLOBAL       = "mix_global";

    // 5 Hardware Module Bypass Enable Switches
    inline constexpr auto MODULE_SUB_ENABLE    = "module_sub_enable";
    inline constexpr auto MODULE_GRIT_ENABLE   = "module_grit_enable";
    inline constexpr auto MODULE_MOD_ENABLE    = "module_mod_enable";
    inline constexpr auto MODULE_DELAY_ENABLE  = "module_delay_enable";
    inline constexpr auto MODULE_REVERB_ENABLE = "module_reverb_enable";

    // 5-Band Visual & Audio EQ Controls (Pro-Q 3 / Slate Infinity Standard)
    inline constexpr auto EQ_LOW_CUT       = "eq_low_cut";
    inline constexpr auto EQ_LOW_GAIN      = "eq_low_gain";
    inline constexpr auto EQ_MID_GAIN      = "eq_mid_gain";
    inline constexpr auto EQ_HIGH_GAIN     = "eq_high_gain";
    inline constexpr auto EQ_HIGH_CUT      = "eq_high_cut";

    inline constexpr auto EQ_LOW_Q         = "eq_low_q";
    inline constexpr auto EQ_MID_Q         = "eq_mid_q";
    inline constexpr auto EQ_HIGH_Q        = "eq_high_q";

    inline constexpr auto EQ_LOW_CUT_SLOPE = "eq_low_cut_slope";   // 0: 6dB, 1: 12dB, 2: 24dB, 3: 48dB, 4: 96dB
    inline constexpr auto EQ_HIGH_CUT_SLOPE= "eq_high_cut_slope";  // 0: 6dB, 1: 12dB, 2: 24dB, 3: 48dB, 4: 96dB

    // Macro Matrix Controls
    inline constexpr auto MACRO_DEPTH      = "macro_depth";
    inline constexpr auto MACRO_DARK       = "macro_dark";
    inline constexpr auto MACRO_MOTION     = "macro_motion";
    inline constexpr auto MACRO_CHAOS      = "macro_chaos";
    inline constexpr auto MACRO_AGE        = "macro_age";
    inline constexpr auto MACRO_GHOST      = "macro_ghost";
    inline constexpr auto MACRO_TONE       = "macro_tone";

    // DEMON / PITCH & FORMANT Module (Flagship Demonic Undertone Engine)
    inline constexpr auto SHADOW_ENABLE    = "shadow_enable";
    inline constexpr auto DEMON_ENABLE     = "shadow_enable";
    inline constexpr auto SHADOW_MIX       = "shadow_mix";
    inline constexpr auto DEMON_MIX        = "shadow_mix";
    inline constexpr auto DEMON_PITCH      = "demon_pitch";      // -24.0 to +24.0 Semitones
    inline constexpr auto SHADOW_PITCH     = "demon_pitch";
    inline constexpr auto DEMON_FORMANT    = "demon_formant";    // -12.0 to +12.0 Semitones (Throat/Chest size)
    inline constexpr auto SHADOW_FORMANT   = "demon_formant";
    inline constexpr auto DEMON_LINK       = "demon_link";       // bool: link pitch & formant
    inline constexpr auto DEMON_MODE       = "demon_mode";       // 0: Transpose, 1: Robot, 2: Hard Tune
    inline constexpr auto SHADOW_DARKNESS  = "shadow_darkness";  // 0.0 to 1.0 (lowpass cutoff)
    inline constexpr auto SHADOW_DRIVE     = "shadow_drive";     // 0.0 to 1.0 (sub saturation)
    inline constexpr auto DEMON_DRIVE      = "shadow_drive";

    // CRUSH Module
    inline constexpr auto CRUSH_AMOUNT     = "crush_amount";
    inline constexpr auto CRUSH_CHARACTER  = "crush_character";
    inline constexpr auto CRUSH_TONE       = "crush_tone";
    inline constexpr auto CRUSH_MIX        = "crush_mix";
    inline constexpr auto CRUSH_PUNISH     = "crush_punish";
    inline constexpr auto CRUSH_OVERSAMPLE = "crush_oversample";


    // WIDTH & MODULATION Module
    inline constexpr auto WIDTH_AMOUNT     = "width_amount";
    inline constexpr auto WIDTH_DETUNE     = "width_detune";
    inline constexpr auto WIDTH_MIX        = "width_mix";
    inline constexpr auto MOD_RATE         = "mod_rate";
    inline constexpr auto MOD_DEPTH        = "mod_depth";

    // SPACE & TIME Module (Delay & Reverb)
    inline constexpr auto SPACE_REVERB     = "space_reverb";
    inline constexpr auto REVERB_DECAY     = "reverb_decay";
    inline constexpr auto REVERB_MIX       = "reverb_mix";
    inline constexpr auto SPACE_DELAY      = "space_delay";
    inline constexpr auto SPACE_FEEDBACK   = "space_feedback";
    inline constexpr auto DELAY_FEEDBACK   = "delay_feedback";
    inline constexpr auto DELAY_MIX        = "delay_mix";
    inline constexpr auto SPACE_DUCKING    = "space_ducking";

    // DEVICE Module
    inline constexpr auto DEVICE_TYPE      = "device_type";
    inline constexpr auto DEVICE_DRIVE     = "device_drive";

    // MASTER CONTROLS
    inline constexpr auto MASTER_BYPASS    = "master_bypass";

    // DYNAMICS & TAMING CORE (Vocal Compressor & De-Esser)
    inline constexpr auto COMP_SQUEEZE     = "comp_squeeze";
    inline constexpr auto COMP_CHARACTER   = "comp_character";
    inline constexpr auto DEESS_AMOUNT     = "deess_amount";
    inline constexpr auto DEESS_FREQ       = "deess_freq";

    // PSYCHOACOUSTIC FRESH AIR EXCITER
    inline constexpr auto AIR_MID          = "air_mid";
    inline constexpr auto AIR_TOP          = "air_top";
}



