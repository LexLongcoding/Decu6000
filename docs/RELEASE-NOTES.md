**Decu6000** - an anime-inspired, mid-90s cyber-industrial / glitch / trip-hop drum synthesizer.
16 fully synthesized voices, a gated reverb, and the Bio-Neural Engine. Free and open source (AGPL-3.0-or-later).

## New in 0.9.1
- **Fixed:** the patch jumped back to INIT when playback stopped or a loop restarted (seen in Cubase).
  Patches are now chosen only in Decu6000's own patch browser; they no longer appear in the DAW's
  preset/program menu. DAW presets still work. If Decu6000 is already in a project, you may need to
  remove it from the track and insert it again so the DAW notices the change.

## Downloads
| File | What |
|---|---|
| `Decu6000-*-Windows-x64.zip` | Windows 64-bit: **VST3** plug-in + **Standalone** app |
| `Decu6000-*-Linux-x86_64.tar.gz` | Linux x86_64: **VST3** plug-in + **Standalone** app |
| `Decu6000-*-User-Manual.pdf` | The full user manual |

## Install
- **Windows VST3:** copy the `Decu6000.vst3` folder to `C:\Program Files\Common Files\VST3\` and rescan in your DAW.
- **Windows Standalone:** run `Decu6000.exe`. If Windows shows "Windows protected your PC", click **More info -> Run anyway** (the app isn't code-signed).
- **Linux VST3:** copy `Decu6000.vst3` to `~/.vst3/`. **Standalone:** run `./Decu6000` (ALSA or JACK).
- In the Standalone, click **Options -> Audio/MIDI Settings** to pick your audio output and MIDI controller.

Each download includes an INSTALL file, the license and the manual.

## What's inside
- **11 machine voices:** kick, snare, closed/open hats, crash, ride, three toms, two FM percussion voices
- **5 bio-neural voices:** Crystal Impact, Liquid Metal, Synthetic Breath, Digital Perc, Bio Burst
- **Bio-Neural Engine:** global instability that "awakens" as you play - stutters, crush, bits, tape wow, micro-timing, probability; LUCID (how unstable) + BLEND (how much, full left = off)
- **Gated reverb** on the master
- **Rack view** and **pad view**; built for 4x4 pad controllers like the Akai MPD18, works with keyboards too
- **MIDI learn** for pads and knobs; pad layouts for pads, keyboards and General MIDI
- **Patches:** save/recall, 7 factory patches, shared between plug-in and standalone
