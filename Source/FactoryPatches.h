// Decu6000 - Copyright (C) 2026 LexLongcoding
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once
#include <utility>
#include <vector>

// Built-in patches. Each lists only what differs from the defaults (real units:
// Hz, seconds, 0..1 amounts, ratios); everything else loads at its default.
namespace FactoryPatches
{
    struct Patch
    {
        const char* name;
        std::vector<std::pair<const char*, float>> values;
    };

    inline const std::vector<Patch>& all()
    {
        static const std::vector<Patch> patches {
            { "INIT", {} },

            { "HOLOGRAM DIVA", {
                { "kick_pitch", 55.0f }, { "kick_decay", 0.45f }, { "kick_drive", 0.25f },
                { "snare_tone", 0.7f }, { "snare_snappy", 0.7f },
                { "hhc_metal", 0.9f }, { "hhc_tone", 0.7f }, { "hho_metal", 0.9f }, { "hho_tone", 0.7f },
                { "ride_level", 0.5f },
                { "crystal_level", 0.8f }, { "crystal_shimmer", 0.75f }, { "crystal_send", 0.5f },
                { "crystal_pitch", 1320.0f }, { "crystal_decay", 2.4f },
                { "liquid_send", 0.4f }, { "liquid_drip", 0.35f },
                { "master_reverb", 0.6f }, { "master_gate", 0.35f },
                { "engine_lucid", 0.25f }, { "engine_grain", 0.15f }, { "engine_crush", 0.0f }, { "engine_bits", 0.1f },
                { "engine_rate", 0.6f }, { "engine_microt", 0.006f }, { "engine_blend", 0.6f } } },

            { "RUST BELT", {
                { "kick_pitch", 42.0f }, { "kick_decay", 1.2f }, { "kick_drive", 0.8f }, { "kick_drone", 0.35f },
                { "snare_tune", 160.0f }, { "snare_drive", 0.7f }, { "snare_crush", 0.35f },
                { "hhc_crush", 0.4f }, { "hhc_metal", 1.0f }, { "hho_crush", 0.4f }, { "hho_metal", 1.0f },
                { "lotom_bend", 0.7f }, { "lotom_drive", 0.6f }, { "midtom_bend", 0.7f }, { "midtom_drive", 0.6f },
                { "hitom_bend", 0.7f }, { "hitom_drive", 0.6f },
                { "crash_crush", 0.5f }, { "crash_ring", 0.9f },
                { "perc2_ratio", 2.76f }, { "perc2_index", 0.8f }, { "perc2_crush", 0.3f },
                { "liquid_pitch", 180.0f }, { "liquid_modrate", 12.0f },
                { "master_reverb", 0.35f }, { "master_gate", 0.18f },
                { "engine_lucid", 0.35f }, { "engine_crush", 0.5f }, { "engine_bits", 0.35f }, { "engine_grain", 0.1f },
                { "engine_blend", 0.8f } } },

            { "TRIP HOP 94", {
                { "kick_pitch", 45.0f }, { "kick_decay", 0.7f }, { "kick_drive", 0.2f },
                { "snare_tune", 150.0f }, { "snare_decay", 0.35f }, { "snare_snappy", 0.45f },
                { "snare_tone", 0.3f }, { "snare_crush", 0.25f },
                { "hhc_tone", 0.3f }, { "hhc_crush", 0.3f }, { "hhc_decay", 0.05f },
                { "hho_decay", 0.45f }, { "hho_tone", 0.35f }, { "hho_crush", 0.3f },
                { "lotom_bend", 0.15f }, { "midtom_bend", 0.15f }, { "hitom_bend", 0.15f },
                { "perc1_pitch", 600.0f }, { "perc1_ratio", 1.0f }, { "perc1_index", 0.15f },
                { "crystal_level", 0.45f }, { "crystal_inharm", 0.4f },
                { "master_reverb", 0.45f }, { "master_gate", 0.6f }, { "master_predelay", 0.04f },
                { "engine_lucid", 0.2f }, { "engine_bits", 0.3f }, { "engine_crush", 0.2f }, { "engine_grain", 0.0f },
                { "engine_rate", 0.4f }, { "engine_microt", 0.008f }, { "engine_blend", 0.5f } } },

            { "GLITCH RITUAL", {
                { "snare_crush", 0.7f }, { "snare_decay", 0.12f },
                { "hhc_crush", 0.8f }, { "hhc_tune", 0.7f }, { "hho_crush", 0.6f },
                { "lotom_bend", 1.0f }, { "lotom_decay", 0.2f }, { "midtom_bend", 1.0f }, { "midtom_decay", 0.2f },
                { "hitom_bend", 1.0f }, { "hitom_decay", 0.2f },
                { "perc1_ratio", 3.5f }, { "perc1_index", 0.7f }, { "perc1_crush", 0.6f }, { "perc1_decay", 0.03f },
                { "perc2_ratio", 7.1f }, { "perc2_index", 1.0f }, { "perc2_crush", 0.7f },
                { "crystal_inharm", 1.0f }, { "crystal_shimmer", 1.0f },
                { "liquid_modrate", 22.0f }, { "liquid_drip", 0.9f },
                { "master_gate", 0.12f },
                { "engine_lucid", 0.8f }, { "engine_grain", 0.7f }, { "engine_crush", 0.45f }, { "engine_bits", 0.5f },
                { "engine_rate", 4.0f }, { "engine_microt", 0.02f }, { "engine_prob", 0.85f }, { "engine_seed", 1337.0f } } },

            { "SUBMERGED", {
                { "kick_pitch", 38.0f }, { "kick_decay", 2.0f }, { "kick_drone", 0.6f },
                { "snare_tone", 0.2f }, { "snare_snappy", 0.35f },
                { "hhc_tone", 0.15f }, { "hho_tone", 0.15f },
                { "crystal_pitch", 500.0f }, { "crystal_decay", 4.0f }, { "crystal_send", 0.45f },
                { "liquid_drip", 0.8f }, { "liquid_modrate", 3.0f }, { "liquid_send", 0.6f }, { "liquid_decay", 1.6f },
                { "master_reverb", 0.7f }, { "master_gate", 0.9f }, { "master_predelay", 0.1f },
                { "master_level", 0.55f },
                { "engine_lucid", 0.3f }, { "engine_rate", 0.3f }, { "engine_grain", 0.0f }, { "engine_blend", 0.5f } } },

            { "AWAKENING", {
                { "kick_pitch", 47.0f }, { "kick_decay", 0.8f }, { "kick_drive", 0.45f },
                { "snare_tune", 175.0f }, { "snare_crush", 0.2f },
                { "crystal_send", 0.5f }, { "crystal_shimmer", 0.6f }, { "liquid_send", 0.4f }, { "breath_vowel", 0.3f },
                { "digital_chaos", 0.55f }, { "burst_swarm", 0.7f },
                { "master_reverb", 0.55f }, { "master_gate", 0.4f },
                { "engine_lucid", 1.0f }, { "engine_grain", 0.5f }, { "engine_crush", 0.35f }, { "engine_bits", 0.35f },
                { "engine_rate", 2.5f }, { "engine_microt", 0.015f }, { "engine_prob", 0.92f }, { "engine_blend", 0.85f },
                { "engine_seed", 2600.0f } } },
        };
        return patches;
    }
}
