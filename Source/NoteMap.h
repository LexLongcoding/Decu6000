// Decu6000 - Copyright (C) 2026 LexLongcoding
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once
#include "PadMap.h"
#include <array>
#include <atomic>
#include <cstdint>
#include <initializer_list>
#include <string>

// Which MIDI note triggers which pad. Every one of the 128 notes points at a
// pad (0..15) or at nothing (-1), so a pad can answer to several notes (e.g.
// GM kick on 35 and 36) and any controller or keyboard layout can be learned.
//
// Entries are atomics: the audio thread reads them (and writes them during
// MIDI learn) while the editor reads them for display and writes them when a
// preset is chosen. No locks, no allocation on the audio thread.
class NoteMap
{
public:
    static constexpr int numNotes = 128;

    enum class Layout : int
    {
        PadsFrom36 = 0,   // MPD18 / most 4x4 pad controllers, default bank
        KeysFromC2,       // notes 48..63
        KeysFromC3,       // notes 60..75, starting at middle C
        GeneralMidi,      // GM drum map (DAW drum racks, e-kits, MIDI files)
        numLayouts
    };

    static const char* layoutName (Layout l) noexcept
    {
        switch (l)
        {
            case Layout::PadsFrom36:  return "Pads: notes 36-51 (MPD18 default)";
            case Layout::KeysFromC2:  return "Keyboard: from C2 (notes 48-63)";
            case Layout::KeysFromC3:  return "Keyboard: from middle C (notes 60-75)";
            case Layout::GeneralMidi: return "General MIDI drum map";
            case Layout::numLayouts:  break;
        }
        return "";
    }

    NoteMap() { applyLayout (Layout::PadsFrom36); }

    int padForNote (int note) const noexcept
    {
        return (note >= 0 && note < numNotes) ? map[static_cast<size_t> (note)].load (std::memory_order_relaxed) : -1;
    }

    void clearAll() noexcept
    {
        for (auto& m : map) m.store (-1, std::memory_order_relaxed);
    }

    void clearPad (int pad) noexcept
    {
        for (auto& m : map)
            if (m.load (std::memory_order_relaxed) == pad)
                m.store (-1, std::memory_order_relaxed);
    }

    // MIDI learn: this note now plays `pad`, replacing the pad's previous notes.
    // If another pad used this note, it gives it up.
    void learn (int note, int pad) noexcept
    {
        if (note < 0 || note >= numNotes || pad < 0 || pad >= PadMap::numPads)
            return;
        clearPad (pad);
        map[static_cast<size_t> (note)].store (static_cast<int8_t> (pad), std::memory_order_relaxed);
    }

    // Lowest note assigned to the pad and how many notes it has in total.
    struct PadNotes { int firstNote = -1; int count = 0; };

    PadNotes notesForPad (int pad) const noexcept
    {
        PadNotes r;
        for (int n = 0; n < numNotes; ++n)
            if (padForNote (n) == pad)
            {
                if (r.count++ == 0) r.firstNote = n;
            }
        return r;
    }

    void applyLayout (Layout l) noexcept
    {
        clearAll();
        switch (l)
        {
            case Layout::PadsFrom36:  chromatic (36); break;
            case Layout::KeysFromC2:  chromatic (48); break;
            case Layout::KeysFromC3:  chromatic (60); break;
            case Layout::GeneralMidi: generalMidi();  break;
            case Layout::numLayouts:  break;
        }
    }

    // Compact text form for saving with the plugin state: 128 comma-separated values.
    std::string toString() const
    {
        std::string s;
        for (int n = 0; n < numNotes; ++n)
        {
            if (n > 0) s += ',';
            s += std::to_string (padForNote (n));
        }
        return s;
    }

    // Returns false (and leaves the map untouched) if the text is malformed.
    bool fromString (const std::string& s)
    {
        std::array<int8_t, numNotes> parsed {};
        size_t pos = 0;
        for (int n = 0; n < numNotes; ++n)
        {
            const size_t end = s.find (',', pos);
            const std::string token = s.substr (pos, end == std::string::npos ? std::string::npos : end - pos);
            if (token.empty()) return false;

            int v = 0;
            try { v = std::stoi (token); } catch (...) { return false; }
            if (v < -1 || v >= PadMap::numPads) return false;
            parsed[static_cast<size_t> (n)] = static_cast<int8_t> (v);

            if (end == std::string::npos) { if (n != numNotes - 1) return false; }
            else pos = end + 1;
        }
        for (size_t n = 0; n < parsed.size(); ++n)
            map[n].store (parsed[n], std::memory_order_relaxed);
        return true;
    }

private:
    void set (int note, int pad) noexcept
    {
        map[static_cast<size_t> (note)].store (static_cast<int8_t> (pad), std::memory_order_relaxed);
    }

    void chromatic (int base) noexcept
    {
        for (int p = 0; p < PadMap::numPads; ++p)
            set (base + p, p);
    }

    void generalMidi() noexcept
    {
        using namespace PadMap;
        auto assign = [this] (int pad, std::initializer_list<int> notes)
        {
            for (int n : notes) set (n, pad);
        };
        assign (Kick,        { 35, 36 });   // acoustic / bass drum 1
        assign (Perc1,       { 37 });       // side stick
        assign (Snare,       { 38, 40 });   // acoustic / electric snare
        assign (Perc2,       { 39 });       // hand clap
        assign (LoTom,       { 41, 43 });   // low floor / high floor tom
        assign (ClosedHat,   { 42, 44 });   // closed / pedal hi-hat
        assign (MidTom,      { 45, 47 });   // low / low-mid tom
        assign (OpenHat,     { 46 });
        assign (HiTom,       { 48, 50 });   // hi-mid / high tom
        assign (Crash,       { 49, 57 });   // crash 1 / crash 2
        assign (Ride,        { 51, 59 });   // ride 1 / ride 2
        assign (Crystal,     { 52 });       // chinese cymbal
        assign (Liquid,      { 53 });       // ride bell
        assign (BioBurst,    { 54 });       // tambourine
        assign (Breath,      { 55 });       // splash
        assign (DigitalPerc, { 56 });       // cowbell
    }

    std::array<std::atomic<int8_t>, numNotes> map {};
};
