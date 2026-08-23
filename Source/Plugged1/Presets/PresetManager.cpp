#include "PresetManager.h"
#include <algorithm>

namespace Plugged1
{

PresetManager::PresetManager(juce::AudioProcessorValueTreeState& state)
    : apvts(state)
{
    initFactoryPresets();
}

void PresetManager::initFactoryPresets()
{
    factoryPresets.clear();

    // ==========================================
    // 1. 808s & SUBS
    // ==========================================
    factoryPresets.push_back({
        "Sub Zero Drill 808",
        "808s & Subs",
        "Classic aggressive saturated drill 808 with snappy pitch dive and foldback distortion.",
        {
            {"voice_mode", 2.0f}, // Legato Glide
            {"glide_time", 60.0f},
            {"sub_enabled", 1.0f},
            {"sub_waveform", 3.0f}, // Drill Distort Saw
            {"sub_octave", -1.0f},
            {"sub_punch_amount", 0.85f},
            {"sub_punch_decay", 35.0f},
            {"sub_drive", 0.75f},
            {"sub_gain", 0.9f},
            {"synth_enabled", 0.0f},
            {"amp_attack", 1.0f},
            {"amp_decay", 650.0f},
            {"amp_sustain", 0.4f},
            {"amp_release", 120.0f},
            {"filter_type", 0.0f},
            {"filter_cutoff", 16000.0f},
            {"filter_resonance", 0.1f},
            {"fx_drive_amount", 0.6f},
            {"fx_drive_type", 2.0f}, // Hard Clip
            {"fx_delay_mix", 0.0f},
            {"fx_reverb_mix", 0.0f},
            {"macro_punch", 0.6f},
            {"macro_dirt", 0.5f}
        }
    });

    factoryPresets.push_back({
        "Heavy 808 Rumble",
        "808s & Subs",
        "Heavy rumbling sub bass designed for fast hi-hat patterns and low-end pressure.",
        {
            {"voice_mode", 2.0f},
            {"glide_time", 45.0f},
            {"sub_enabled", 1.0f},
            {"sub_waveform", 2.0f}, // Tube Saturated
            {"sub_octave", -1.0f},
            {"sub_punch_amount", 0.45f},
            {"sub_punch_decay", 50.0f},
            {"sub_drive", 0.5f},
            {"sub_gain", 0.95f},
            {"synth_enabled", 0.0f},
            {"amp_attack", 2.0f},
            {"amp_decay", 800.0f},
            {"amp_sustain", 0.6f},
            {"amp_release", 150.0f},
            {"filter_cutoff", 7000.0f},
            {"fx_drive_amount", 0.35f},
            {"fx_drive_type", 1.0f} // Tube Warmth
        }
    });

    factoryPresets.push_back({
        "Slap Trap 808",
        "808s & Subs",
        "Balanced mid warmth and punchy transient slap attack.",
        {
            {"voice_mode", 2.0f},
            {"glide_time", 40.0f},
            {"sub_enabled", 1.0f},
            {"sub_waveform", 4.0f}, // Punch Transient
            {"sub_octave", -1.0f},
            {"sub_punch_amount", 0.75f},
            {"sub_punch_decay", 40.0f},
            {"sub_drive", 0.45f},
            {"sub_gain", 0.92f},
            {"synth_enabled", 0.0f},
            {"amp_attack", 1.0f},
            {"amp_decay", 850.0f},
            {"amp_sustain", 0.5f},
            {"amp_release", 140.0f},
            {"filter_cutoff", 12000.0f},
            {"fx_drive_amount", 0.4f},
            {"fx_drive_type", 0.0f} // Soft Clip
        }
    });

    factoryPresets.push_back({
        "Ghost Glide Sub",
        "808s & Subs",
        "Ultra-smooth exponential legato glide 808 for dark melodic slides.",
        {
            {"voice_mode", 2.0f},
            {"glide_time", 75.0f},
            {"sub_enabled", 1.0f},
            {"sub_waveform", 0.0f}, // Pure Sine
            {"sub_octave", -1.0f},
            {"sub_punch_amount", 0.3f},
            {"sub_punch_decay", 50.0f},
            {"sub_drive", 0.25f},
            {"sub_gain", 0.95f},
            {"synth_enabled", 0.0f},
            {"amp_attack", 2.0f},
            {"amp_decay", 1200.0f},
            {"amp_sustain", 0.75f},
            {"amp_release", 180.0f}
        }
    });

    factoryPresets.push_back({
        "Distorted Drill Beast",
        "808s & Subs",
        "Upper harmonic drill distortion that cuts through mobile and laptop speakers.",
        {
            {"voice_mode", 2.0f},
            {"glide_time", 55.0f},
            {"sub_enabled", 1.0f},
            {"sub_waveform", 3.0f}, // Drill Distort Saw
            {"sub_octave", -1.0f},
            {"sub_punch_amount", 0.9f},
            {"sub_punch_decay", 30.0f},
            {"sub_drive", 0.85f},
            {"sub_gain", 0.88f},
            {"synth_enabled", 0.0f},
            {"amp_attack", 1.0f},
            {"amp_decay", 700.0f},
            {"amp_sustain", 0.4f},
            {"amp_release", 100.0f},
            {"fx_drive_amount", 0.75f},
            {"fx_drive_type", 2.0f} // Hard Clip
        }
    });

    factoryPresets.push_back({
        "Chop Block Lows",
        "808s & Subs",
        "Short punchy knock 808 for fast rhythm grooves and heavy drop sections.",
        {
            {"voice_mode", 2.0f},
            {"glide_time", 35.0f},
            {"sub_enabled", 1.0f},
            {"sub_waveform", 1.0f}, // Warm Triangle
            {"sub_octave", -1.0f},
            {"sub_punch_amount", 0.7f},
            {"sub_punch_decay", 35.0f},
            {"sub_drive", 0.4f},
            {"sub_gain", 0.92f},
            {"synth_enabled", 0.0f},
            {"amp_attack", 1.0f},
            {"amp_decay", 500.0f},
            {"amp_sustain", 0.3f},
            {"amp_release", 100.0f}
        }
    });

    factoryPresets.push_back({
        "Pure Sine Pressure",
        "808s & Subs",
        "Clean sub pressure sine wave for sub bass foundations and club systems.",
        {
            {"voice_mode", 0.0f}, // Poly
            {"sub_enabled", 1.0f},
            {"sub_waveform", 0.0f}, // Pure Sine
            {"sub_octave", -1.0f},
            {"sub_punch_amount", 0.0f},
            {"sub_drive", 0.0f},
            {"sub_gain", 1.0f},
            {"synth_enabled", 0.0f},
            {"amp_attack", 3.0f},
            {"amp_decay", 1000.0f},
            {"amp_sustain", 1.0f},
            {"amp_release", 120.0f}
        }
    });

    factoryPresets.push_back({
        "Analog Moog Bass",
        "808s & Subs",
        "Fat warm analog low pass filter bass with snappy envelope bite.",
        {
            {"voice_mode", 1.0f}, // Mono
            {"sub_enabled", 1.0f},
            {"sub_waveform", 1.0f},
            {"sub_octave", -1.0f},
            {"sub_gain", 0.6f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 0.0f}, // Saw
            {"synth_unison", 2.0f},
            {"synth_detune", 0.08f},
            {"synth_octave", -1.0f},
            {"synth_gain", 0.7f},
            {"amp_attack", 2.0f},
            {"amp_decay", 450.0f},
            {"amp_sustain", 0.5f},
            {"amp_release", 100.0f},
            {"filter_cutoff", 1400.0f},
            {"filter_resonance", 2.5f},
            {"filter_env_amount", 0.45f},
            {"filt_attack", 2.0f},
            {"filt_decay", 280.0f}
        }
    });

    factoryPresets.push_back({
        "Drill Stomp Sub",
        "808s & Subs",
        "Heavy percussive 808 stomp with rapid transient drop.",
        {
            {"voice_mode", 2.0f},
            {"glide_time", 50.0f},
            {"sub_enabled", 1.0f},
            {"sub_waveform", 3.0f},
            {"sub_octave", -1.0f},
            {"sub_punch_amount", 0.95f},
            {"sub_punch_decay", 28.0f},
            {"sub_drive", 0.65f},
            {"sub_gain", 0.9f},
            {"synth_enabled", 0.0f},
            {"amp_attack", 1.0f},
            {"amp_decay", 600.0f},
            {"amp_sustain", 0.35f},
            {"amp_release", 90.0f}
        }
    });

    factoryPresets.push_back({
        "Midnight Sub Boom",
        "808s & Subs",
        "Warm low-mid sub saturation with smooth decay.",
        {
            {"voice_mode", 2.0f},
            {"glide_time", 60.0f},
            {"sub_enabled", 1.0f},
            {"sub_waveform", 2.0f},
            {"sub_octave", -1.0f},
            {"sub_punch_amount", 0.5f},
            {"sub_punch_decay", 45.0f},
            {"sub_drive", 0.55f},
            {"sub_gain", 0.95f},
            {"synth_enabled", 0.0f},
            {"amp_attack", 2.0f},
            {"amp_decay", 1100.0f},
            {"amp_sustain", 0.65f},
            {"amp_release", 160.0f}
        }
    });

    // ==========================================
    // 2. KEYS & PIANOS
    // ==========================================
    factoryPresets.push_back({
        "Velvet Concert Grand",
        "Keys & Pianos",
        "Multi-partial dynamic acoustic grand piano with natural felt resonance and wood body.",
        {
            {"voice_mode", 0.0f}, // Poly
            {"sub_enabled", 0.0f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 3.0f}, // Acoustic Grand
            {"synth_unison", 2.0f},
            {"synth_detune", 0.05f},
            {"synth_spread", 0.4f},
            {"synth_octave", 0.0f},
            {"synth_gain", 0.85f},
            {"amp_attack", 1.0f},
            {"amp_decay", 2600.0f},
            {"amp_sustain", 0.3f},
            {"amp_release", 450.0f},
            {"filter_cutoff", 14000.0f},
            {"filter_resonance", 0.2f},
            {"fx_reverb_mix", 0.24f},
            {"fx_reverb_size", 0.65f},
            {"fx_delay_mix", 0.0f}
        }
    });

    factoryPresets.push_back({
        "Vintage 1973 Rhodes",
        "Keys & Pianos",
        "Warm vintage stage electric piano with metallic tine attack and tube preamp saturation.",
        {
            {"voice_mode", 0.0f},
            {"sub_enabled", 0.0f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 4.0f}, // Vintage Rhodes
            {"synth_unison", 1.0f},
            {"synth_gain", 0.85f},
            {"amp_attack", 1.0f},
            {"amp_decay", 1800.0f},
            {"amp_sustain", 0.4f},
            {"amp_release", 350.0f},
            {"filter_cutoff", 11000.0f},
            {"fx_drive_amount", 0.28f},
            {"fx_drive_type", 1.0f}, // Tube Warmth
            {"fx_reverb_mix", 0.26f},
            {"fx_delay_mix", 0.15f}
        }
    });

    factoryPresets.push_back({
        "Dark Felt Piano",
        "Keys & Pianos",
        "Muted felt piano with intimate low-pass tone for emotional trap and cinematic progressions.",
        {
            {"voice_mode", 0.0f},
            {"sub_enabled", 0.0f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 3.0f},
            {"synth_unison", 1.0f},
            {"synth_gain", 0.9f},
            {"amp_attack", 2.0f},
            {"amp_decay", 1600.0f},
            {"amp_sustain", 0.35f},
            {"amp_release", 380.0f},
            {"filter_cutoff", 3200.0f},
            {"filter_resonance", 0.3f},
            {"fx_reverb_mix", 0.38f},
            {"fx_reverb_size", 0.75f},
            {"fx_reverb_decay", 3.2f}
        }
    });

    factoryPresets.push_back({
        "Lo-Fi Wurlitzer",
        "Keys & Pianos",
        "Vintage reed electric piano with subtle tape grit and chorus shimmer.",
        {
            {"voice_mode", 0.0f},
            {"sub_enabled", 0.0f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 4.0f},
            {"synth_unison", 2.0f},
            {"synth_detune", 0.08f},
            {"synth_gain", 0.8f},
            {"amp_attack", 1.0f},
            {"amp_decay", 1400.0f},
            {"amp_sustain", 0.45f},
            {"amp_release", 280.0f},
            {"filter_cutoff", 4800.0f},
            {"fx_drive_amount", 0.38f},
            {"fx_drive_type", 1.0f},
            {"fx_delay_mix", 0.2f},
            {"fx_reverb_mix", 0.25f}
        }
    });

    factoryPresets.push_back({
        "Crystal FM Electric Piano",
        "Keys & Pianos",
        "Bright 80s FM digital tine piano with crystalline stereo bell overtones.",
        {
            {"voice_mode", 0.0f},
            {"sub_enabled", 0.0f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 4.0f},
            {"synth_unison", 2.0f},
            {"synth_detune", 0.1f},
            {"synth_spread", 0.7f},
            {"synth_gain", 0.8f},
            {"amp_attack", 0.5f},
            {"amp_decay", 2200.0f},
            {"amp_sustain", 0.35f},
            {"amp_release", 400.0f},
            {"filter_cutoff", 16000.0f},
            {"fx_reverb_mix", 0.35f},
            {"fx_delay_mix", 0.28f}
        }
    });

    factoryPresets.push_back({
        "Neo Soul Tine Keys",
        "Keys & Pianos",
        "Lush electric piano tines with smooth dynamic release.",
        {
            {"voice_mode", 0.0f},
            {"sub_enabled", 0.0f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 4.0f},
            {"synth_unison", 1.0f},
            {"synth_gain", 0.88f},
            {"amp_attack", 1.0f},
            {"amp_decay", 1700.0f},
            {"amp_sustain", 0.5f},
            {"amp_release", 320.0f},
            {"filter_cutoff", 6800.0f},
            {"fx_reverb_mix", 0.3f}
        }
    });

    factoryPresets.push_back({
        "Moody Minor Grand",
        "Keys & Pianos",
        "Acoustic piano layered with subtle sub warmth for dark melodic themes.",
        {
            {"voice_mode", 0.0f},
            {"sub_enabled", 1.0f},
            {"sub_waveform", 0.0f},
            {"sub_octave", -1.0f},
            {"sub_gain", 0.35f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 3.0f},
            {"synth_gain", 0.85f},
            {"amp_attack", 1.5f},
            {"amp_decay", 2200.0f},
            {"amp_sustain", 0.3f},
            {"amp_release", 420.0f},
            {"filter_cutoff", 5800.0f},
            {"fx_reverb_mix", 0.42f}
        }
    });

    factoryPresets.push_back({
        "Tape Saturated Keys",
        "Keys & Pianos",
        "Analog-warmed keys with subtle tape saturation and flutter space.",
        {
            {"voice_mode", 0.0f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 4.0f},
            {"synth_gain", 0.85f},
            {"amp_attack", 1.0f},
            {"amp_decay", 1500.0f},
            {"amp_sustain", 0.45f},
            {"amp_release", 300.0f},
            {"filter_cutoff", 5200.0f},
            {"fx_drive_amount", 0.45f},
            {"fx_drive_type", 1.0f},
            {"fx_delay_mix", 0.22f},
            {"fx_reverb_mix", 0.28f}
        }
    });

    // ==========================================
    // 3. PLUCKS & MALLETS
    // ==========================================
    factoryPresets.push_back({
        "Opal Drop Pluck",
        "Plucks & Mallets",
        "Fast crystalline pluck with stereo ping-pong space delay.",
        {
            {"voice_mode", 0.0f},
            {"sub_enabled", 0.0f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 6.0f}, // Pluck Guitar
            {"synth_unison", 1.0f},
            {"synth_gain", 0.85f},
            {"amp_attack", 0.5f},
            {"amp_decay", 420.0f},
            {"amp_sustain", 0.0f},
            {"amp_release", 120.0f},
            {"filter_cutoff", 11000.0f},
            {"fx_delay_mix", 0.32f},
            {"fx_delay_time", 250.0f},
            {"fx_reverb_mix", 0.35f}
        }
    });

    factoryPresets.push_back({
        "Hyperpop Neon Stab",
        "Plucks & Mallets",
        "Ultra-bright snappy supersaw stab for high-energy hooks.",
        {
            {"voice_mode", 0.0f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 8.0f}, // Supersaw
            {"synth_unison", 3.0f},
            {"synth_detune", 0.2f},
            {"synth_gain", 0.8f},
            {"amp_attack", 0.5f},
            {"amp_decay", 260.0f},
            {"amp_sustain", 0.0f},
            {"amp_release", 80.0f},
            {"filter_cutoff", 16000.0f},
            {"fx_drive_amount", 0.4f}
        }
    });

    factoryPresets.push_back({
        "Metallic Drill Pluck",
        "Plucks & Mallets",
        "Sharp metallic FM pluck designed for dark drill melodies.",
        {
            {"voice_mode", 0.0f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 5.0f}, // FM Bell
            {"synth_unison", 1.0f},
            {"synth_gain", 0.85f},
            {"amp_attack", 0.5f},
            {"amp_decay", 340.0f},
            {"amp_sustain", 0.0f},
            {"amp_release", 100.0f},
            {"filter_cutoff", 8500.0f},
            {"fx_delay_mix", 0.26f},
            {"fx_reverb_mix", 0.22f}
        }
    });

    factoryPresets.push_back({
        "Celeste Glockenspiel",
        "Plucks & Mallets",
        "Pure melodic chime bells with wide shimmer reverb.",
        {
            {"voice_mode", 0.0f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 5.0f},
            {"synth_octave", 1.0f},
            {"synth_gain", 0.75f},
            {"amp_attack", 0.5f},
            {"amp_decay", 800.0f},
            {"amp_sustain", 0.0f},
            {"amp_release", 400.0f},
            {"filter_cutoff", 18000.0f},
            {"fx_reverb_mix", 0.45f},
            {"fx_reverb_size", 0.8f}
        }
    });

    factoryPresets.push_back({
        "Ice Crystal Bells",
        "Plucks & Mallets",
        "Shimmering ice bells with rich stereo detune.",
        {
            {"voice_mode", 0.0f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 5.0f},
            {"synth_unison", 2.0f},
            {"synth_detune", 0.12f},
            {"synth_gain", 0.75f},
            {"amp_attack", 0.5f},
            {"amp_decay", 1200.0f},
            {"amp_sustain", 0.1f},
            {"amp_release", 500.0f},
            {"fx_reverb_mix", 0.5f},
            {"fx_delay_mix", 0.35f}
        }
    });

    factoryPresets.push_back({
        "Starlight Vibraphone",
        "Plucks & Mallets",
        "Warm acoustic vibraphone mallet with organic decay.",
        {
            {"voice_mode", 0.0f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 5.0f},
            {"synth_unison", 1.0f},
            {"synth_gain", 0.85f},
            {"amp_attack", 1.0f},
            {"amp_decay", 1400.0f},
            {"amp_sustain", 0.15f},
            {"amp_release", 350.0f},
            {"filter_cutoff", 9500.0f},
            {"fx_reverb_mix", 0.38f}
        }
    });

    factoryPresets.push_back({
        "Music Box Dream",
        "Plucks & Mallets",
        "Charming vintage mechanical music box tone.",
        {
            {"voice_mode", 0.0f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 5.0f},
            {"synth_octave", 1.0f},
            {"synth_gain", 0.75f},
            {"amp_attack", 0.5f},
            {"amp_decay", 600.0f},
            {"amp_sustain", 0.0f},
            {"amp_release", 250.0f},
            {"fx_reverb_mix", 0.5f}
        }
    });

    factoryPresets.push_back({
        "Tokyo Koto Pluck",
        "Plucks & Mallets",
        "East Asian acoustic plucked string with sharp pick snap.",
        {
            {"voice_mode", 0.0f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 6.0f},
            {"synth_gain", 0.85f},
            {"amp_attack", 0.5f},
            {"amp_decay", 480.0f},
            {"amp_sustain", 0.0f},
            {"amp_release", 120.0f},
            {"filter_cutoff", 10000.0f},
            {"fx_delay_mix", 0.28f},
            {"fx_reverb_mix", 0.3f}
        }
    });

    // ==========================================
    // 4. GUITARS & STRINGS
    // ==========================================
    factoryPresets.push_back({
        "Acoustic Nylon Guitar",
        "Guitars & Strings",
        "Warm acoustic nylon string guitar with realistic finger pick transient.",
        {
            {"voice_mode", 0.0f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 6.0f}, // Pluck Guitar
            {"synth_unison", 1.0f},
            {"synth_gain", 0.85f},
            {"amp_attack", 1.0f},
            {"amp_decay", 950.0f},
            {"amp_sustain", 0.1f},
            {"amp_release", 220.0f},
            {"filter_cutoff", 7800.0f},
            {"fx_reverb_mix", 0.25f}
        }
    });

    factoryPresets.push_back({
        "Muted Electric Riff",
        "Guitars & Strings",
        "Palm-muted clean electric guitar with snappy punch.",
        {
            {"voice_mode", 0.0f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 6.0f},
            {"synth_gain", 0.88f},
            {"amp_attack", 0.5f},
            {"amp_decay", 220.0f},
            {"amp_sustain", 0.0f},
            {"amp_release", 60.0f},
            {"filter_cutoff", 4600.0f},
            {"fx_drive_amount", 0.35f},
            {"fx_drive_type", 0.0f}
        }
    });

    factoryPresets.push_back({
        "Clean Strat Chord Pluck",
        "Guitars & Strings",
        "Lush stereo electric guitar plucks with stereo chorus space.",
        {
            {"voice_mode", 0.0f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 6.0f},
            {"synth_unison", 2.0f},
            {"synth_detune", 0.06f},
            {"synth_spread", 0.6f},
            {"synth_gain", 0.82f},
            {"amp_attack", 1.0f},
            {"amp_decay", 1100.0f},
            {"amp_sustain", 0.2f},
            {"amp_release", 250.0f},
            {"filter_cutoff", 9000.0f},
            {"fx_delay_mix", 0.25f},
            {"fx_reverb_mix", 0.3f}
        }
    });

    factoryPresets.push_back({
        "Pizzicato Chamber Strings",
        "Guitars & Strings",
        "Short orchestral pizzicato string stabs for rhythmic bounce.",
        {
            {"voice_mode", 0.0f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 6.0f},
            {"synth_unison", 2.0f},
            {"synth_detune", 0.08f},
            {"synth_gain", 0.82f},
            {"amp_attack", 0.5f},
            {"amp_decay", 380.0f},
            {"amp_sustain", 0.0f},
            {"amp_release", 100.0f},
            {"filter_cutoff", 11000.0f},
            {"fx_reverb_mix", 0.35f}
        }
    });

    factoryPresets.push_back({
        "Dark Drill Violin Pluck",
        "Guitars & Strings",
        "Dark violin plucks with tape delay space.",
        {
            {"voice_mode", 0.0f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 6.0f},
            {"synth_gain", 0.85f},
            {"amp_attack", 1.0f},
            {"amp_decay", 420.0f},
            {"amp_sustain", 0.0f},
            {"amp_release", 120.0f},
            {"filter_cutoff", 6200.0f},
            {"fx_drive_amount", 0.25f},
            {"fx_delay_mix", 0.3f}
        }
    });

    factoryPresets.push_back({
        "Soul Electric Guitar",
        "Guitars & Strings",
        "Warm soul electric guitar with sustained body.",
        {
            {"voice_mode", 0.0f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 6.0f},
            {"synth_gain", 0.85f},
            {"amp_attack", 2.0f},
            {"amp_decay", 1400.0f},
            {"amp_sustain", 0.3f},
            {"amp_release", 280.0f},
            {"filter_cutoff", 8000.0f},
            {"fx_drive_amount", 0.3f},
            {"fx_reverb_mix", 0.32f}
        }
    });

    // ==========================================
    // 5. SYNTHS & LEADS
    // ==========================================
    factoryPresets.push_back({
        "Drill Glide Solo Lead",
        "Synths & Leads",
        "Aggressive resonant drill lead with smooth legato portamento glide.",
        {
            {"voice_mode", 2.0f}, // Legato
            {"glide_time", 65.0f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 10.0f}, // Acid Sync
            {"synth_gain", 0.8f},
            {"amp_attack", 2.0f},
            {"amp_decay", 1200.0f},
            {"amp_sustain", 0.85f},
            {"amp_release", 150.0f},
            {"filter_cutoff", 9000.0f},
            {"filter_resonance", 2.2f},
            {"fx_drive_amount", 0.45f},
            {"fx_delay_mix", 0.3f},
            {"fx_reverb_mix", 0.25f}
        }
    });

    factoryPresets.push_back({
        "Hyperpop Supersaw Lead",
        "Synths & Leads",
        "7-voice wide detuned supersaw for huge modern anthems.",
        {
            {"voice_mode", 0.0f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 8.0f}, // Supersaw
            {"synth_unison", 4.0f},
            {"synth_detune", 0.25f},
            {"synth_spread", 0.85f},
            {"synth_gain", 0.75f},
            {"amp_attack", 1.0f},
            {"amp_sustain", 0.9f},
            {"amp_release", 200.0f},
            {"filter_cutoff", 18000.0f},
            {"fx_drive_amount", 0.4f},
            {"fx_reverb_mix", 0.35f}
        }
    });

    factoryPresets.push_back({
        "Analog Brass Lead",
        "Synths & Leads",
        "Warm vintage poly synth brass with dynamic filter envelope sweep.",
        {
            {"voice_mode", 0.0f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 0.0f}, // Saw
            {"synth_unison", 3.0f},
            {"synth_detune", 0.15f},
            {"synth_spread", 0.6f},
            {"synth_gain", 0.8f},
            {"amp_attack", 12.0f},
            {"amp_decay", 800.0f},
            {"amp_sustain", 0.8f},
            {"amp_release", 250.0f},
            {"filter_cutoff", 4500.0f},
            {"filt_attack", 15.0f},
            {"filt_decay", 450.0f},
            {"filter_env_amount", 0.6f}
        }
    });

    factoryPresets.push_back({
        "Acid Resonance Screamer",
        "Synths & Leads",
        "High-resonance screaming lead with aggressive bite.",
        {
            {"voice_mode", 2.0f},
            {"glide_time", 40.0f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 10.0f},
            {"synth_gain", 0.8f},
            {"filter_cutoff", 3500.0f},
            {"filter_resonance", 4.5f},
            {"filter_env_amount", 0.7f},
            {"fx_drive_amount", 0.65f}
        }
    });

    factoryPresets.push_back({
        "Vocal Formant Lead",
        "Synths & Leads",
        "Human vocal formant vowel resonance choir lead.",
        {
            {"voice_mode", 2.0f},
            {"glide_time", 50.0f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 7.0f}, // Vocal Formant
            {"synth_unison", 2.0f},
            {"synth_detune", 0.1f},
            {"synth_gain", 0.82f},
            {"amp_attack", 8.0f},
            {"amp_sustain", 0.8f},
            {"amp_release", 200.0f},
            {"filter_cutoff", 12000.0f},
            {"fx_delay_mix", 0.35f},
            {"fx_reverb_mix", 0.4f}
        }
    });

    factoryPresets.push_back({
        "Cyber Synth Hook",
        "Synths & Leads",
        "Punchy square lead with stereo unison width.",
        {
            {"voice_mode", 0.0f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 1.0f}, // Square
            {"synth_unison", 3.0f},
            {"synth_detune", 0.18f},
            {"synth_gain", 0.78f},
            {"amp_attack", 1.0f},
            {"amp_decay", 400.0f},
            {"amp_sustain", 0.5f},
            {"amp_release", 120.0f},
            {"filter_cutoff", 11000.0f},
            {"fx_delay_mix", 0.25f}
        }
    });

    factoryPresets.push_back({
        "Retro Poly 80s",
        "Synths & Leads",
        "Classic 1980s polyphonic analog synthesizer synth sound.",
        {
            {"voice_mode", 0.0f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 0.0f},
            {"synth_unison", 2.0f},
            {"synth_detune", 0.1f},
            {"synth_gain", 0.85f},
            {"amp_attack", 5.0f},
            {"amp_decay", 800.0f},
            {"amp_sustain", 0.6f},
            {"amp_release", 200.0f},
            {"filter_cutoff", 6000.0f},
            {"fx_reverb_mix", 0.3f}
        }
    });

    factoryPresets.push_back({
        "Future Trap Wave",
        "Synths & Leads",
        "Modern melodic trap lead with atmospheric delay space.",
        {
            {"voice_mode", 2.0f},
            {"glide_time", 55.0f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 8.0f},
            {"synth_gain", 0.78f},
            {"amp_attack", 5.0f},
            {"amp_sustain", 0.75f},
            {"amp_release", 220.0f},
            {"filter_cutoff", 14000.0f},
            {"fx_delay_mix", 0.35f},
            {"fx_reverb_mix", 0.4f}
        }
    });

    // ==========================================
    // 6. PADS & ATMOSPHERES
    // ==========================================
    factoryPresets.push_back({
        "Atmospheric Aura Pad",
        "Pads & Atmospheres",
        "Lush ethereal vocal choir pad with deep cinematic reverb tail.",
        {
            {"voice_mode", 0.0f},
            {"sub_enabled", 0.0f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 7.0f}, // Vocal Formant
            {"synth_unison", 4.0f},
            {"synth_detune", 0.2f},
            {"synth_spread", 0.9f},
            {"synth_gain", 0.75f},
            {"amp_attack", 350.0f},
            {"amp_decay", 2500.0f},
            {"amp_sustain", 0.85f},
            {"amp_release", 1200.0f},
            {"filter_cutoff", 6500.0f},
            {"fx_reverb_mix", 0.55f},
            {"fx_reverb_size", 0.85f},
            {"fx_delay_mix", 0.3f}
        }
    });

    factoryPresets.push_back({
        "Underground Noir Pad",
        "Pads & Atmospheres",
        "Dark filtered atmospheric pad for underground trap and drill intros.",
        {
            {"voice_mode", 0.0f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 0.0f},
            {"synth_unison", 3.0f},
            {"synth_detune", 0.15f},
            {"synth_gain", 0.78f},
            {"amp_attack", 450.0f},
            {"amp_sustain", 0.9f},
            {"amp_release", 1400.0f},
            {"filter_cutoff", 2400.0f},
            {"fx_reverb_mix", 0.6f}
        }
    });

    factoryPresets.push_back({
        "Tape Warmth Lush Pad",
        "Pads & Atmospheres",
        "Warm vintage analog pad with tape saturation.",
        {
            {"voice_mode", 0.0f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 2.0f}, // Triangle
            {"synth_unison", 4.0f},
            {"synth_detune", 0.18f},
            {"synth_gain", 0.8f},
            {"amp_attack", 250.0f},
            {"amp_sustain", 0.85f},
            {"amp_release", 900.0f},
            {"filter_cutoff", 4000.0f},
            {"fx_drive_amount", 0.3f},
            {"fx_reverb_mix", 0.45f}
        }
    });

    factoryPresets.push_back({
        "Cyberpunk Space Drone",
        "Pads & Atmospheres",
        "Massive cinematic drone with low sub foundation and shimmering top.",
        {
            {"voice_mode", 0.0f},
            {"sub_enabled", 1.0f},
            {"sub_waveform", 2.0f},
            {"sub_gain", 0.5f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 8.0f},
            {"synth_unison", 3.0f},
            {"synth_detune", 0.25f},
            {"synth_gain", 0.7f},
            {"amp_attack", 600.0f},
            {"amp_sustain", 1.0f},
            {"amp_release", 2000.0f},
            {"filter_cutoff", 3500.0f},
            {"fx_reverb_mix", 0.65f}
        }
    });

    factoryPresets.push_back({
        "Silk Cloud Ambient",
        "Pads & Atmospheres",
        "Glassy ambient pad with floating stereo delay reflections.",
        {
            {"voice_mode", 0.0f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 5.0f},
            {"synth_unison", 2.0f},
            {"synth_detune", 0.1f},
            {"synth_gain", 0.75f},
            {"amp_attack", 400.0f},
            {"amp_sustain", 0.8f},
            {"amp_release", 1100.0f},
            {"filter_cutoff", 8000.0f},
            {"fx_reverb_mix", 0.55f},
            {"fx_delay_mix", 0.35f}
        }
    });

    factoryPresets.push_back({
        "Ethereal Vocal Pad",
        "Pads & Atmospheres",
        "Smooth vowel singing choir pad with wide spatial aura.",
        {
            {"voice_mode", 0.0f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 7.0f},
            {"synth_unison", 3.0f},
            {"synth_detune", 0.15f},
            {"synth_gain", 0.78f},
            {"amp_attack", 300.0f},
            {"amp_sustain", 0.85f},
            {"amp_release", 1000.0f},
            {"filter_cutoff", 7500.0f},
            {"fx_reverb_mix", 0.5f}
        }
    });

    // ==========================================
    // 7. BRASS & HITS
    // ==========================================
    factoryPresets.push_back({
        "Trap Stabs Brass",
        "Brass & Hits",
        "High-impact aggressive brass stabs for trap and drill drops.",
        {
            {"voice_mode", 0.0f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 0.0f}, // Saw
            {"synth_unison", 4.0f},
            {"synth_detune", 0.2f},
            {"synth_spread", 0.8f},
            {"synth_gain", 0.82f},
            {"amp_attack", 5.0f},
            {"amp_decay", 450.0f},
            {"amp_sustain", 0.4f},
            {"amp_release", 150.0f},
            {"filter_cutoff", 8500.0f},
            {"filt_attack", 5.0f},
            {"filt_decay", 380.0f},
            {"filter_env_amount", 0.65f},
            {"fx_drive_amount", 0.4f}
        }
    });

    factoryPresets.push_back({
        "Low End Orchestral Horns",
        "Brass & Hits",
        "Deep cinematic horns reinforced with clean sub low end.",
        {
            {"voice_mode", 0.0f},
            {"sub_enabled", 1.0f},
            {"sub_waveform", 1.0f},
            {"sub_octave", -1.0f},
            {"sub_gain", 0.45f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 0.0f},
            {"synth_octave", -1.0f},
            {"synth_unison", 3.0f},
            {"synth_detune", 0.15f},
            {"synth_gain", 0.8f},
            {"amp_attack", 25.0f},
            {"amp_sustain", 0.75f},
            {"amp_release", 300.0f},
            {"filter_cutoff", 3500.0f},
            {"fx_reverb_mix", 0.35f}
        }
    });

    factoryPresets.push_back({
        "Drill Impact Hit",
        "Brass & Hits",
        "Sub-heavy impact hit with explosive punch transient.",
        {
            {"voice_mode", 0.0f},
            {"sub_enabled", 1.0f},
            {"sub_waveform", 3.0f},
            {"sub_punch_amount", 0.9f},
            {"sub_gain", 0.7f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 8.0f},
            {"synth_gain", 0.8f},
            {"amp_attack", 0.5f},
            {"amp_decay", 350.0f},
            {"amp_sustain", 0.0f},
            {"amp_release", 80.0f},
            {"filter_cutoff", 16000.0f},
            {"fx_drive_amount", 0.55f}
        }
    });

    factoryPresets.push_back({
        "Cinematic Brass Swell",
        "Brass & Hits",
        "Slow building orchestral brass swell for tension and energy risers.",
        {
            {"voice_mode", 0.0f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 0.0f},
            {"synth_unison", 4.0f},
            {"synth_detune", 0.22f},
            {"synth_gain", 0.8f},
            {"amp_attack", 180.0f},
            {"amp_sustain", 0.85f},
            {"amp_release", 600.0f},
            {"filter_cutoff", 5500.0f},
            {"filter_env_amount", 0.5f},
            {"fx_reverb_mix", 0.45f}
        }
    });

    factoryPresets.push_back({
        "Synthetic Horn Section",
        "Brass & Hits",
        "Bright synthetic horns with rhythmic decay.",
        {
            {"voice_mode", 0.0f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 0.0f},
            {"synth_unison", 3.0f},
            {"synth_detune", 0.16f},
            {"synth_gain", 0.82f},
            {"amp_attack", 15.0f},
            {"amp_decay", 700.0f},
            {"amp_sustain", 0.6f},
            {"amp_release", 200.0f},
            {"filter_cutoff", 6500.0f},
            {"fx_delay_mix", 0.2f},
            {"fx_reverb_mix", 0.3f}
        }
    });

    // ==========================================
    // 8. ORGANS & VINTAGE
    // ==========================================
    factoryPresets.push_back({
        "B3 Gospel Drawbar",
        "Organs & Vintage",
        "Full harmonic drawbar tonewheel organ with warm tube saturation.",
        {
            {"voice_mode", 0.0f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 9.0f}, // Drawbar Organ
            {"synth_gain", 0.88f},
            {"amp_attack", 1.0f},
            {"amp_sustain", 0.95f},
            {"amp_release", 80.0f},
            {"filter_cutoff", 16000.0f},
            {"fx_drive_amount", 0.25f},
            {"fx_drive_type", 1.0f}, // Tube Warmth
            {"fx_reverb_mix", 0.25f}
        }
    });

    factoryPresets.push_back({
        "Dark Cathedral Organ",
        "Organs & Vintage",
        "Monumental pipe organ with spacious cathedral reverberation.",
        {
            {"voice_mode", 0.0f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 9.0f},
            {"synth_unison", 3.0f},
            {"synth_detune", 0.12f},
            {"synth_gain", 0.8f},
            {"amp_attack", 15.0f},
            {"amp_sustain", 0.9f},
            {"amp_release", 350.0f},
            {"filter_cutoff", 9000.0f},
            {"fx_reverb_mix", 0.55f},
            {"fx_reverb_decay", 4.5f}
        }
    });

    factoryPresets.push_back({
        "Retro Combo Organ",
        "Organs & Vintage",
        "1960s combo organ with bright upper drawbars for energetic chops.",
        {
            {"voice_mode", 0.0f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 9.0f},
            {"synth_gain", 0.85f},
            {"amp_attack", 2.0f},
            {"amp_sustain", 0.9f},
            {"amp_release", 60.0f},
            {"filter_cutoff", 8000.0f},
            {"fx_drive_amount", 0.35f},
            {"fx_delay_mix", 0.25f}
        }
    });

    factoryPresets.push_back({
        "Dub Reggae Organ Chop",
        "Organs & Vintage",
        "Short percussive organ stab with long feedback dub tape echo.",
        {
            {"voice_mode", 0.0f},
            {"synth_enabled", 1.0f},
            {"synth_shape", 9.0f},
            {"synth_gain", 0.85f},
            {"amp_attack", 1.0f},
            {"amp_decay", 180.0f},
            {"amp_sustain", 0.0f},
            {"amp_release", 50.0f},
            {"filter_cutoff", 12000.0f},
            {"fx_delay_mix", 0.4f},
            {"fx_delay_feedback", 0.55f},
            {"fx_reverb_mix", 0.3f}
        }
    });
}

std::vector<std::string> PresetManager::getCategories() const
{
    std::vector<std::string> categories;
    for (const auto& preset : factoryPresets)
    {
        if (std::find(categories.begin(), categories.end(), preset.category) == categories.end())
        {
            categories.push_back(preset.category);
        }
    }
    return categories;
}

std::vector<int> PresetManager::getPresetIndicesForCategory(const std::string& category) const
{
    std::vector<int> indices;
    for (size_t i = 0; i < factoryPresets.size(); ++i)
    {
        if (factoryPresets[i].category == category)
            indices.push_back(static_cast<int>(i));
    }
    return indices;
}

void PresetManager::loadPreset(int index)
{
    if (index < 0 || index >= static_cast<int>(factoryPresets.size()))
        return;

    currentPresetIndex = index;
    const auto& preset = factoryPresets[index];

    for (const auto& [paramId, value] : preset.parameters)
    {
        if (auto* param = apvts.getParameter(paramId))
        {
            float normalized = param->getNormalisableRange().convertTo0to1(value);
            param->setValueNotifyingHost(normalized);
        }
    }
}

void PresetManager::loadPresetByName(const std::string& name)
{
    for (size_t i = 0; i < factoryPresets.size(); ++i)
    {
        if (factoryPresets[i].name == name)
        {
            loadPreset(static_cast<int>(i));
            return;
        }
    }
}

} // namespace Plugged1
