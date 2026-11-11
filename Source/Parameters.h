// Decu6000 - Copyright (C) 2026 LexLongcoding
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once
#include <array>

// Single source of truth for every Decu6000 parameter. The processor builds
// its APVTS layout from this table; both editor views use it for knob labels
// and grouping; MIDI CC learn saves mappings by these IDs.
namespace Params
{
    enum class Unit { Percent, Hz, Time, Semitones, Ratio, Integer, Toggle };

    struct Spec
    {
        const char* id;      // APVTS parameter ID (also the CC-map key)
        const char* name;    // full name shown to the host
        const char* knob;    // short knob label
        float min, max;
        float centre;        // skew centre; <= 0 means linear
        float def;
        Unit unit;
    };

    // Groups (in display order) - the ID prefix before the first '_'.
    struct Group { const char* prefix; const char* title; };
    constexpr std::array<Group, 18> groups {{
        { "kick", "KICK" }, { "snare", "SNARE" }, { "hhc", "CLOSED HH" }, { "hho", "OPEN HH" },
        { "crash", "CRASH" }, { "ride", "RIDE" }, { "lotom", "LO TOM" }, { "midtom", "MID TOM" },
        { "hitom", "HI TOM" }, { "perc1", "PERC 1" }, { "perc2", "PERC 2" },
        { "crystal", "CRYSTAL IMPACT" }, { "liquid", "LIQUID METAL" }, { "breath", "SYNTHETIC BREATH" },
        { "digital", "DIGITAL PERC" }, { "burst", "BIO BURST" },
        { "engine", "ENGINE" }, { "master", "MASTER" } }};

    using U = Unit;
    constexpr Spec specs[] = {
        { "master_level",  "Master Level",   "LEVEL",  0.0f, 1.0f, 0.0f, 0.8f, U::Percent },
        { "master_reverb", "Reverb Return",  "REVERB", 0.0f, 1.0f, 0.0f, 0.5f, U::Percent },
        { "master_gate",   "Reverb Gate Time","GATE T", 0.05f, 1.0f, 0.25f, 0.3f, U::Time },
        { "master_predelay","Reverb Predelay","PREDLY", 0.0f, 0.25f, 0.0f, 0.02f, U::Time },

        // Bio-Neural Engine (global instability; LUCID 0 = asleep / bypassed)
        { "engine_blend",  "Engine Blend",   "BLEND",  0.0f, 1.0f, 0.0f, 1.0f, U::Percent },
        { "engine_lucid",  "Engine Lucid/Unstable", "LUCID / UNSTABLE", 0.0f, 1.0f, 0.0f, 0.0f, U::Percent },
        { "engine_grain",  "Engine Grain",   "GRAIN",  0.0f, 1.0f, 0.0f, 0.3f, U::Percent },
        { "engine_crush",  "Engine Crush",   "CRUSH",  0.0f, 1.0f, 0.0f, 0.3f, U::Percent },
        { "engine_bits",   "Engine Bits",    "BITS",   0.0f, 1.0f, 0.0f, 0.3f, U::Percent },
        { "engine_rate",   "Engine Rate",    "RATE",   0.1f, 12.0f, 1.5f, 1.5f, U::Hz },
        { "engine_microt", "Engine Micro-Timing","MICRO-T", 0.0f, 0.04f, 0.0f, 0.012f, U::Time },
        { "engine_prob",   "Engine Probability","PROB", 0.0f, 1.0f, 0.0f, 1.0f, U::Percent },
        { "engine_seed",   "Engine Seed",    "SEED",   0.0f, 9999.0f, 0.0f, 4096.0f, U::Integer },

        { "kick_pitch",    "Kick Pitch",     "PITCH",  30.0f, 120.0f, 55.0f, 50.0f, U::Hz },
        { "kick_decay",    "Kick Decay",     "DECAY",  0.05f, 2.5f, 0.5f, 0.5f, U::Time },
        { "kick_level",    "Kick Level",     "LEVEL",  0.0f, 1.0f, 0.0f, 0.9f, U::Percent },
        { "kick_drive",    "Kick Drive",     "DRIVE",  0.0f, 1.0f, 0.0f, 0.3f, U::Percent },
        { "kick_drone",    "Kick Drone",     "DRONE",  0.0f, 1.0f, 0.0f, 0.0f, U::Percent },

        { "snare_tune",    "Snare Tune",     "PITCH",  120.0f, 400.0f, 200.0f, 190.0f, U::Hz },
        { "snare_decay",   "Snare Decay",    "DECAY",  0.03f, 1.0f, 0.2f, 0.2f, U::Time },
        { "snare_level",   "Snare Level",    "LEVEL",  0.0f, 1.0f, 0.0f, 0.8f, U::Percent },
        { "snare_snappy",  "Snare Snappy",   "SNAP",   0.0f, 1.0f, 0.0f, 0.6f, U::Percent },
        { "snare_tone",    "Snare Tone",     "TONE",   0.0f, 1.0f, 0.0f, 0.5f, U::Percent },
        { "snare_drive",   "Snare Drive",    "DRIVE",  0.0f, 1.0f, 0.0f, 0.2f, U::Percent },
        { "snare_crush",   "Snare Crush",    "CRUSH",  0.0f, 1.0f, 0.0f, 0.0f, U::Percent },

        { "hhc_tune",      "Closed HH Tune", "PITCH",  0.0f, 1.0f, 0.0f, 0.5f, U::Semitones },
        { "hhc_decay",     "Closed HH Decay","DECAY",  0.02f, 0.5f, 0.08f, 0.06f, U::Time },
        { "hhc_level",     "Closed HH Level","LEVEL",  0.0f, 1.0f, 0.0f, 0.6f, U::Percent },
        { "hhc_tone",      "Closed HH Tone", "TONE",   0.0f, 1.0f, 0.0f, 0.5f, U::Percent },
        { "hhc_metal",     "Closed HH Metal","METAL",  0.0f, 1.0f, 0.0f, 0.7f, U::Percent },
        { "hhc_crush",     "Closed HH Crush","CRUSH",  0.0f, 1.0f, 0.0f, 0.0f, U::Percent },

        { "hho_tune",      "Open HH Tune",   "PITCH",  0.0f, 1.0f, 0.0f, 0.5f, U::Semitones },
        { "hho_decay",     "Open HH Decay",  "DECAY",  0.1f, 2.0f, 0.5f, 0.6f, U::Time },
        { "hho_level",     "Open HH Level",  "LEVEL",  0.0f, 1.0f, 0.0f, 0.6f, U::Percent },
        { "hho_tone",      "Open HH Tone",   "TONE",   0.0f, 1.0f, 0.0f, 0.5f, U::Percent },
        { "hho_metal",     "Open HH Metal",  "METAL",  0.0f, 1.0f, 0.0f, 0.7f, U::Percent },
        { "hho_crush",     "Open HH Crush",  "CRUSH",  0.0f, 1.0f, 0.0f, 0.0f, U::Percent },

        { "crash_tune",    "Crash Tune",     "PITCH",  0.0f, 1.0f, 0.0f, 0.5f, U::Semitones },
        { "crash_decay",   "Crash Decay",    "DECAY",  0.3f, 5.0f, 1.5f, 1.6f, U::Time },
        { "crash_level",   "Crash Level",    "LEVEL",  0.0f, 1.0f, 0.0f, 0.6f, U::Percent },
        { "crash_tone",    "Crash Tone",     "TONE",   0.0f, 1.0f, 0.0f, 0.5f, U::Percent },
        { "crash_ring",    "Crash Ring",     "RING",   0.0f, 1.0f, 0.0f, 0.6f, U::Percent },
        { "crash_crush",   "Crash Crush",    "CRUSH",  0.0f, 1.0f, 0.0f, 0.0f, U::Percent },

        { "ride_tune",     "Ride Tune",      "PITCH",  0.0f, 1.0f, 0.0f, 0.62f, U::Semitones },
        { "ride_decay",    "Ride Decay",     "DECAY",  0.5f, 8.0f, 2.5f, 3.0f, U::Time },
        { "ride_level",    "Ride Level",     "LEVEL",  0.0f, 1.0f, 0.0f, 0.45f, U::Percent },
        { "ride_tone",     "Ride Tone",      "TONE",   0.0f, 1.0f, 0.0f, 0.75f, U::Percent },
        { "ride_ring",     "Ride Ring",      "RING",   0.0f, 1.0f, 0.0f, 0.9f, U::Percent },
        { "ride_crush",    "Ride Crush",     "CRUSH",  0.0f, 1.0f, 0.0f, 0.0f, U::Percent },

        { "lotom_tune",    "Lo Tom Tune",    "PITCH",  40.0f, 160.0f, 80.0f, 82.0f, U::Hz },
        { "lotom_decay",   "Lo Tom Decay",   "DECAY",  0.05f, 2.0f, 0.4f, 0.45f, U::Time },
        { "lotom_level",   "Lo Tom Level",   "LEVEL",  0.0f, 1.0f, 0.0f, 0.8f, U::Percent },
        { "lotom_bend",    "Lo Tom Bend",    "BEND",   0.0f, 1.0f, 0.0f, 0.4f, U::Percent },
        { "lotom_noise",   "Lo Tom Noise",   "NOISE",  0.0f, 1.0f, 0.0f, 0.3f, U::Percent },
        { "lotom_drive",   "Lo Tom Drive",   "DRIVE",  0.0f, 1.0f, 0.0f, 0.2f, U::Percent },
        { "lotom_crush",   "Lo Tom Crush",   "CRUSH",  0.0f, 1.0f, 0.0f, 0.0f, U::Percent },

        { "midtom_tune",   "Mid Tom Tune",   "PITCH",  60.0f, 240.0f, 120.0f, 123.0f, U::Hz },
        { "midtom_decay",  "Mid Tom Decay",  "DECAY",  0.05f, 2.0f, 0.4f, 0.4f, U::Time },
        { "midtom_level",  "Mid Tom Level",  "LEVEL",  0.0f, 1.0f, 0.0f, 0.8f, U::Percent },
        { "midtom_bend",   "Mid Tom Bend",   "BEND",   0.0f, 1.0f, 0.0f, 0.4f, U::Percent },
        { "midtom_noise",  "Mid Tom Noise",  "NOISE",  0.0f, 1.0f, 0.0f, 0.3f, U::Percent },
        { "midtom_drive",  "Mid Tom Drive",  "DRIVE",  0.0f, 1.0f, 0.0f, 0.2f, U::Percent },
        { "midtom_crush",  "Mid Tom Crush",  "CRUSH",  0.0f, 1.0f, 0.0f, 0.0f, U::Percent },

        { "hitom_tune",    "Hi Tom Tune",    "PITCH",  90.0f, 360.0f, 180.0f, 184.0f, U::Hz },
        { "hitom_decay",   "Hi Tom Decay",   "DECAY",  0.05f, 2.0f, 0.4f, 0.35f, U::Time },
        { "hitom_level",   "Hi Tom Level",   "LEVEL",  0.0f, 1.0f, 0.0f, 0.8f, U::Percent },
        { "hitom_bend",    "Hi Tom Bend",    "BEND",   0.0f, 1.0f, 0.0f, 0.4f, U::Percent },
        { "hitom_noise",   "Hi Tom Noise",   "NOISE",  0.0f, 1.0f, 0.0f, 0.3f, U::Percent },
        { "hitom_drive",   "Hi Tom Drive",   "DRIVE",  0.0f, 1.0f, 0.0f, 0.2f, U::Percent },
        { "hitom_crush",   "Hi Tom Crush",   "CRUSH",  0.0f, 1.0f, 0.0f, 0.0f, U::Percent },

        // FM percussion: Perc 1 defaults to a woody click, Perc 2 to a metallic blip
        { "perc1_pitch",   "Perc 1 Pitch",   "PITCH",  100.0f, 2000.0f, 600.0f, 720.0f, U::Hz },
        { "perc1_decay",   "Perc 1 Decay",   "DECAY",  0.01f, 1.5f, 0.15f, 0.07f, U::Time },
        { "perc1_level",   "Perc 1 Level",   "LEVEL",  0.0f, 1.0f, 0.0f, 0.7f, U::Percent },
        { "perc1_ratio",   "Perc 1 Ratio",   "RATIO",  0.5f, 8.0f, 2.0f, 2.0f, U::Ratio },
        { "perc1_index",   "Perc 1 Index",   "INDEX",  0.0f, 1.0f, 0.0f, 0.25f, U::Percent },
        { "perc1_crush",   "Perc 1 Crush",   "CRUSH",  0.0f, 1.0f, 0.0f, 0.0f, U::Percent },

        { "perc2_pitch",   "Perc 2 Pitch",   "PITCH",  100.0f, 2000.0f, 600.0f, 540.0f, U::Hz },
        { "perc2_decay",   "Perc 2 Decay",   "DECAY",  0.01f, 1.5f, 0.15f, 0.3f, U::Time },
        { "perc2_level",   "Perc 2 Level",   "LEVEL",  0.0f, 1.0f, 0.0f, 0.55f, U::Percent },
        { "perc2_ratio",   "Perc 2 Ratio",   "RATIO",  0.5f, 8.0f, 2.0f, 1.41f, U::Ratio },
        { "perc2_index",   "Perc 2 Index",   "INDEX",  0.0f, 1.0f, 0.0f, 0.55f, U::Percent },
        { "perc2_crush",   "Perc 2 Crush",   "CRUSH",  0.0f, 1.0f, 0.0f, 0.0f, U::Percent },

        // bio-neural voices (SEND feeds the gated reverb)
        { "crystal_pitch", "Crystal Pitch",  "PITCH",  200.0f, 4000.0f, 900.0f, 880.0f, U::Hz },
        { "crystal_decay", "Crystal Decay",  "DECAY",  0.1f, 6.0f, 1.0f, 1.6f, U::Time },
        { "crystal_level", "Crystal Level",  "LEVEL",  0.0f, 1.0f, 0.0f, 0.6f, U::Percent },
        { "crystal_inharm","Crystal Inharmonicity","INHARM", 0.0f, 1.0f, 0.0f, 0.7f, U::Percent },
        { "crystal_shimmer","Crystal Shimmer","SHIMMER",0.0f, 1.0f, 0.0f, 0.4f, U::Percent },
        { "crystal_send",  "Crystal Send",   "SEND",   0.0f, 1.0f, 0.0f, 0.25f, U::Percent },

        { "liquid_pitch",  "Liquid Pitch",   "PITCH",  60.0f, 2000.0f, 400.0f, 330.0f, U::Hz },
        { "liquid_decay",  "Liquid Decay",   "DECAY",  0.05f, 4.0f, 0.6f, 0.7f, U::Time },
        { "liquid_level",  "Liquid Level",   "LEVEL",  0.0f, 1.0f, 0.0f, 0.6f, U::Percent },
        { "liquid_drip",   "Liquid Drip",    "DRIP",   0.0f, 1.0f, 0.0f, 0.5f, U::Percent },
        { "liquid_modrate","Liquid Mod Rate","MODRATE",0.5f, 30.0f, 6.0f, 7.0f, U::Hz },
        { "liquid_send",   "Liquid Send",    "SEND",   0.0f, 1.0f, 0.0f, 0.3f, U::Percent },

        { "breath_pitch",  "Breath Pitch",   "PITCH",  60.0f, 400.0f, 140.0f, 140.0f, U::Hz },
        { "breath_decay",  "Breath Decay",   "DECAY",  0.03f, 2.0f, 0.35f, 0.35f, U::Time },
        { "breath_level",  "Breath Level",   "LEVEL",  0.0f, 1.0f, 0.0f, 0.7f, U::Percent },
        { "breath_vowel",  "Breath Vowel",   "VOWEL",  0.0f, 1.0f, 0.0f, 0.5f, U::Percent },
        { "breath_breath", "Breath Amount",  "BREATH", 0.0f, 1.0f, 0.0f, 0.4f, U::Percent },
        { "breath_send",   "Breath Send",    "SEND",   0.0f, 1.0f, 0.0f, 0.3f, U::Percent },

        { "digital_pitch", "Digital Pitch",  "PITCH",  50.0f, 2000.0f, 400.0f, 420.0f, U::Hz },
        { "digital_decay", "Digital Decay",  "DECAY",  0.01f, 1.5f, 0.15f, 0.12f, U::Time },
        { "digital_level", "Digital Level",  "LEVEL",  0.0f, 1.0f, 0.0f, 0.6f, U::Percent },
        { "digital_chaos", "Digital Chaos",  "CHAOS",  0.0f, 1.0f, 0.0f, 0.35f, U::Percent },
        { "digital_fold",  "Digital Fold",   "FOLD",   0.0f, 1.0f, 0.0f, 0.5f, U::Percent },
        { "digital_send",  "Digital Send",   "SEND",   0.0f, 1.0f, 0.0f, 0.15f, U::Percent },

        { "burst_pitch",   "Burst Pitch",    "PITCH",  150.0f, 6000.0f, 1200.0f, 1200.0f, U::Hz },
        { "burst_decay",   "Burst Decay",    "DECAY",  0.05f, 3.0f, 0.6f, 0.6f, U::Time },
        { "burst_level",   "Burst Level",    "LEVEL",  0.0f, 1.0f, 0.0f, 0.6f, U::Percent },
        { "burst_swarm",   "Burst Swarm",    "SWARM",  0.0f, 1.0f, 0.0f, 0.5f, U::Percent },
        { "burst_grain",   "Burst Grain",    "GRAIN",  0.0f, 1.0f, 0.0f, 0.3f, U::Percent },
        { "burst_send",    "Burst Send",     "SEND",   0.0f, 1.0f, 0.0f, 0.35f, U::Percent },
    };

    constexpr int numParams = static_cast<int> (sizeof (specs) / sizeof (specs[0]));
}
