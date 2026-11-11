# Decu6000

Mid-90s cyber-industrial / glitch / trip-hop drum synth (JUCE, VST3 + Standalone).
Free and open source - see [License](#license).

## Getting builds

### Easiest: let GitHub build it (Windows + Linux, no compiler needed)
This project includes `.github/workflows/build.yml`. Put the project on GitHub and
GitHub's own Windows and Linux machines build it for free:

1. Create a free account at github.com, then **New repository** (e.g. `decu6000`;
   private is fine).
2. Upload the *contents* of this `Decu6000` folder - including the hidden `.github`
   folder. The simplest reliable way is **GitHub Desktop**: *File > Add local
   repository* (choose this folder; let it create the repository), then
   *Publish repository*.
3. On GitHub, open the **Actions** tab. A "Build Decu6000" run starts by itself
   (or click *Run workflow*). It takes about 10 minutes.
4. Open the finished run and download **Decu6000-Windows** and/or **Decu6000-Linux**
   from the *Artifacts* section at the bottom. Each zip has an INSTALL file.

Pushing a tag such as `v0.8.0` also publishes both as a GitHub Release.

### Build it yourself
Requires CMake 3.22+, a C++17 compiler, and internet access (JUCE is fetched on first configure).

**Windows** - install *Visual Studio 2022 Community* (free; tick "Desktop development
with C++") and CMake, then in this folder:

    cmake -B build -G "Visual Studio 17 2022" -A x64
    cmake --build build --config Release

**Linux** (Debian/Ubuntu package names):

    sudo apt install build-essential cmake git libasound2-dev libjack-jackd2-dev \
        libfreetype6-dev libfontconfig1-dev libx11-dev libxcomposite-dev libxcursor-dev \
        libxext-dev libxinerama-dev libxrandr-dev libxrender-dev libgl1-mesa-dev
    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel

Output: `build/Decu6000_artefacts/Release/VST3/Decu6000.vst3` and
`build/Decu6000_artefacts/Release/Standalone/`. See `packaging/INSTALL-*.txt` for
where to put them.

## The editor
Two views, switched with the tabs in the header (your choice is saved with the project):

- **RACK VIEW** - every voice as a column of knobs with its trigger pad underneath,
  bio-neural voices, engine and master section along the bottom.
- **PAD VIEW** - the 4x4 grid laid out like the hardware (pad 1 bottom-left). Click a
  pad to hear it and edit it: its knobs appear large on the right, master below.

The window can be resized by dragging the bottom-right corner (it keeps its shape);
the size is saved with the project. The LCD in the header shows the value of the knob
you're turning and MIDI-learn messages. Double-click a knob to reset it.


## Patches
The LCD in the header shows the current patch (`PATCH 03 // RUST BELT`); a `*` means
you've changed it since loading. Use the arrows either side to step through patches,
or click the LCD for the full list plus **Save**, **Save As...**, **Delete** and
**Open patch folder**. Your DAW's preset menu lists the same patches.

- 7 factory patches: INIT, HOLOGRAM DIVA, RUST BELT, TRIP HOP 94, GLITCH RITUAL, SUBMERGED, AWAKENING.
- User patches are small `.decupatch` files - copy them between computers or share them:
  - Windows: `%APPDATA%\Decu6000\Patches`
  - macOS: `~/Library/Decu6000/Patches`
- A patch stores every sound setting. Pad/key layouts and MIDI CC knob assignments are
  *not* part of a patch (they describe your controller), so they stay when you switch sounds.
- Your project also remembers which patch was loaded and any unsaved tweaks.

## Pads, keyboards and MIDI learn
**Layout presets** (the "PAD LAYOUT..." menu):
- *Pads: notes 36-51* - MPD18 and most 4x4 pad controllers (default)
- *Keyboard: from C2 (48-63)* or *from middle C (60-75)* - 16 keys in a row
- *General MIDI drum map* - matches DAW drum racks, e-kits and GM drum MIDI files
  (several notes per sound, e.g. kick on 35 and 36)

**MIDI LEARN** (header button): every pad and knob gets an outline showing its current
note (N 36) or controller (CC 21).
- Click a **pad**, then hit any key or pad: that note now plays the sound (it replaces the
  pad's previous notes and is taken from any other pad that had it).
- Click a **knob**, then move a knob/fader on your controller: that CC now controls it
  (one CC per knob; a CC moved to a new knob leaves the old one).
- Right-click a pad or knob in learn mode to clear its mapping. Esc cancels / exits.

Outside learn mode you can also right-click any knob -> *Learn MIDI CC...* / *Clear MIDI CC*,
and right-click any pad -> *Clear MIDI notes*. Note and CC mappings are saved with the
project. Note names use middle C = C3 (Akai / Ableton convention).

## Voices (v0.9.0)
| Pad | Voice | Controls |
|---|---|---|
| 36 | Kick | Pitch, Decay, Level, Drive, Drone |
| 37 | Snare | Pitch, Decay, Level, Snap, Tone, Drive, Crush |
| 38 / 39 | Closed / Open HH | each: Pitch, Decay, Level, Tone, Metal, Crush (closed chokes open) |
| 40-42 | Lo / Mid / Hi Tom | each: Pitch, Decay, Level, Bend, Noise, Drive, Crush |
| 43 / 44 | Crash / Ride | each: Pitch, Decay, Level, Tone, Ring, Crush |
| 45 / 46 | Perc 1 / Perc 2 | each: Pitch, Decay, Level, Ratio, Index, Crush (2-operator FM) |
| 48 | Crystal Impact | Pitch, Decay, Level, Inharm, Shimmer, Send (modal glass synthesis) |
| 49 | Liquid Metal | Pitch, Decay, Level, Drip, Modrate, Send (dripping, wobbling FM metal) |
| 50 | Synthetic Breath | Pitch, Decay, Level, Vowel, Breath, Send (formant "robot throat", U-O-A-E-I) |
| 51 | Digital Perc | Pitch, Decay, Level, Chaos, Fold, Send (wavefolded blip + chaotic digital noise) |
| 47 | Bio Burst | Pitch, Decay, Level, Swarm, Grain, Send (granular swarm of tiny chirps) |

**Master // Gated Verb:** Level, Reverb (return), Gate T (how long the reverb stays open
after a hit before it's cut), Predly (predelay). The bio-neural voices' SEND knobs feed it.

The master output has a soft limiter (transparent below 0.8) so stacked hits saturate
rather than hard-clip. All 16 pads now have voices.

**FM percussion tips:** whole-number Ratios (1:1, 2:1, 3:1) give woody / tonal hits
(blocks, claves, toms); in-between Ratios (1.41:1, 2.76:1...) give bells, cowbells and metal.
Index sets how buzzy the attack is; harder hits are brighter.

## Bio-Neural Engine
A global instability layer over the whole kit (bottom row of the rack view; bottom-right
of the pad view).

- **LUCID / UNSTABLE** (big knob) - *how* unstable things get. The engine "awakens":
  instability builds the longer you play and relaxes when you stop - the live curve on
  its screen shows it, with pink marks for each glitch stutter.
- **BLEND** - *how much* of the engine you hear. All the way left = off (completely
  bypassed), all the way right = full effect. Crush / bits / wow / stutter mix in
  parallel with your dry kit; PROB and MICRO-T scale down with it.
- **GRAIN** glitch stutters (slices of the output repeat) - **CRUSH** sample-rate
  reduction - **BITS** bit depth (16 down to 3 bits) - **RATE** speed of a drifting
  tape-style pitch wow - **MICRO-T** random micro-timing push per hit (up to 40 ms) -
  **PROB** chance each hit plays.
- **SEED** - click for new random behaviour; the same seed repeats the same glitches.

LUCID at 0 or BLEND at 0 = the engine is off and the output is untouched. INIT has
LUCID at 0, so the engine only does something once you turn it up.

## Offline voice test (no JUCE needed)
    cd Tests && c++ -std=c++17 -O2 voice_render_test.cpp -o voice_test && ./voice_test 48000
This tests the DSP only. Renders single hits for every voice, Bend/extreme-setting tests,
a hat choke test and a 2-bar 90 BPM beat with a tom fill to WAV files in the current folder.

## Fonts
`Resources/Fonts` holds the embedded fonts (SIL Open Font License 1.1, licenses included).
See `Resources/Fonts/README-fonts.txt`.

## License
Decu6000 is free software: you can redistribute it and/or modify it under the terms of
the **GNU Affero General Public License v3.0 or later** (see [`LICENSE`](LICENSE)).
It is distributed in the hope that it will be useful, but without any warranty.

In short: you can use, study, change and share it; if you distribute a modified version,
you must make its source available under the same license.

Decu6000 is built with [JUCE](https://juce.com), used under the AGPLv3. Fonts are
under the SIL Open Font License. See [`THIRD-PARTY-NOTICES.md`](THIRD-PARTY-NOTICES.md).

Source code: https://github.com/LexLongcoding/Decu6000
