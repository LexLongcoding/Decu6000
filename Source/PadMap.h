// Decu6000 - Copyright (C) 2026 LexLongcoding
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once
#include <array>

// 4x4 pad map for MPD18-style controllers.
// Pads are numbered from the BOTTOM-LEFT, left to right, then upward.
// Which MIDI notes play which pad is set by NoteMap (layout presets + MIDI
// learn). The note numbers below are the default "Pads: 36-51" layout.
//
//   top      | 48 Crystal | 49 Liquid | 50 Breath  | 51 DigitalPerc |
//            | 44 Ride    | 45 Perc 1 | 46 Perc 2  | 47 Bio Burst   |
//            | 40 Lo Tom  | 41 Mid Tom| 42 Hi Tom  | 43 Crash       |
//   bottom   | 36 Kick    | 37 Snare  | 38 Cl. HH  | 39 Open HH     |
namespace PadMap
{
    constexpr int numPads = 16;

    enum PadIndex : int
    {
        Kick = 0, Snare, ClosedHat, OpenHat,
        LoTom, MidTom, HiTom, Crash,
        Ride, Perc1, Perc2, BioBurst,
        Crystal, Liquid, Breath, DigitalPerc
    };

    constexpr std::array<const char*, numPads> names {{
        "Kick", "Snare", "Closed HH", "Open HH",
        "Lo Tom", "Mid Tom", "Hi Tom", "Crash",
        "Ride", "Perc 1", "Perc 2", "Bio Burst",
        "Crystal Impact", "Liquid Metal", "Synthetic Breath", "Digital Perc" }};
}
