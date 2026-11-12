# Decu6000 - project notes (for contributors and for resuming work)

Last updated: 2026-11-11, version 0.9.1. Repo: https://github.com/LexLongcoding/Decu6000
License: AGPL-3.0-or-later, copyright LexLongcoding (JUCE is used under its AGPLv3 option).

## What it is
A synthesized (sample-free) drum instrument, VST3 + Standalone, Windows + Linux, built with
JUCE 8.0.4 (fetched by CMake). Mid-90s cyber-industrial / glitch / trip-hop; "anime inspired".
Designed around a 4x4 pad controller (Akai MPD18) but works with keyboards and DAW piano rolls.

## Design decisions (and why)
- **Layout** comes from the owner's "SHARON-909 GUI Mockup" design (1440x820 canvas: 11 machine
  voice strips on top; 5 bio-neural strips + engine + master below). The owner asked for **no
  "909" references anywhere**; the mockup's "SHARON APPLE VOICES" heading was relabelled
  "BIO-NEURAL VOICES // SYNTHETIC SOURCES" (third-party character name).
- **Two views**: RACK VIEW (the mockup) and PAD VIEW (4x4 grid; click a pad to edit its voice -
  chosen because 17 knob groups no longer fit beside the grid).
- **Every voice has its own controls** (hats and toms were once shared; split to match the mockup).
- **Pad mapping = NoteMap** (any note -> any pad, several notes per pad), with presets: pads 36-51
  (default), keyboard from C2 / middle C, General MIDI. Replaced an early "base note" setting the
  owner found confusing.
- **MIDI learn** for pads (notes) and knobs (CC 0-119). Mappings are saved with the DAW project,
  **not** in patches (they describe the controller).
- **Patches**: factory (code) + user `.decupatch` XML in the user app-data folder; values stored by
  parameter ID in real units, so old patches load in new versions. Patches are NOT exposed as DAW programs (getNumPrograms() = 1):
  Cubase re-sent program 0 on stop / loop restart and reloaded INIT (fixed in 0.9.1).
- **Bio-Neural Engine**: LUCID / UNSTABLE = how unstable ("drive"); BLEND = how much you hear
  ("mix"; 0 = off). Either at 0 = exact bypass (tested bit-identical). Owner explicitly wanted an
  amount knob where full left = off. Awakening rises with playing. SEED makes glitches repeatable;
  display jitter uses a separate RNG so the UI never changes the sound.
- **Fonts**: Orbitron is OFL with a Reserved Font Name; static 700/900 instances were generated
  and renamed "Decu Display". Share Tech Mono unmodified. All embedded via BinaryData.
- **Windows** links the MSVC runtime statically (no VC++ redistributable needed).
- **Linux** enables ALSA + JACK (JACK loaded at runtime).
- **Master** soft limiter, transparent below 0.8 (~-2 dBFS).

## Source map
| File | Role |
|---|---|
| `Source/Parameters.h` | Single table of all 112 parameters (id, name, knob label, range, skew, default, unit) + group titles |
| `Source/PluginProcessor.*` | Voices, MIDI/event queue, CC & note learn, reverb, engine, state |
| `Source/*Voice.h` | DSP voices (JUCE-free, unit-testable): Kick, Snare, Hat, Tom, Cymbal (crash/ride), Perc (FM), Crystal (modal), Liquid (FM drip), Breath (formant), Digital (fold/chaos), Burst (granular) |
| `Source/BioEngine.h` | Global instability layer (stutter, wow, crush/bits, micro-timing, prob, seed, history curve) |
| `Source/GatedReverb.h` | Send-bus predelay -> juce::Reverb -> gate |
| `Source/NoteMap.h`, `PadMap.h` | Note->pad map with presets; pad indices/names |
| `Source/PatchManager.*`, `FactoryPatches.h` | Patch files, factory patches, session restore |
| `Source/DspUtils.h` | Noise, SVF, crusher, drive, soft limit |
| `Source/PluginEditor.*` | Header (tabs, patch LCD, layout menu, MIDI LEARN), scaling canvas, dialogs |
| `Source/UI/Theme.*` | Colours, embedded fonts, knob drawing (from the mockup) |
| `Source/UI/Widgets.*` | Knob, pads, voice strip, engine panel, master panel, meters |
| `Source/UI/Views.*` | RackView, PadView, LearnOverlay |
| `docs/manual/make_manual.py` | Builds `docs/Decu6000-User-Manual.pdf` (reads Parameters.h) |
| `.github/workflows/build.yml` | Windows + Linux builds; tags `v*` publish a release |

## Testing
- `Tests/voice_render_test.cpp` - DSP only, no JUCE: renders every voice + extremes to WAV.
- Plugin test suite (126 checks: MIDI learn, layouts, CC learn, patches, reverb gate, engine bypass/
  prob/micro-timing/bits/stutter/seed determinism, editor screenshots, demo renders) and a VST3 host
  test (loads the built .vst3 like a DAW) live in the resume pack's `dev-tests/` (they use absolute
  paths from the original session; adjust paths to reuse).

## Status at 2026-11-11
- Linux VST3 + Standalone: built, all tests pass, VST3 verified in a host, Standalone launches.
  Not tested with real audio hardware.
- Windows: **builds on GitHub Actions** (run 43106445486, 2026-11-11: VST3 + Standalone, Visual Studio
  2026 / MSVC 19.51, zero warnings from our code). Not yet run on a real Windows machine.
  (Runs 43097004219 / 43098653239 got stuck "queued" during a GitHub outage; they can be ignored or
  cancelled.) The workflow no longer hard-codes a Visual Studio version.
- All 16 voices, engine, reverb, patches, MIDI learn, both views, manual: done.

## Possible next steps (discussed or natural)
- Try the Windows build on a real PC (VST3 in a DAW + Standalone with audio/MIDI).
- Optional: macOS build in CI, per-voice panning / multi-out, more factory patches.
