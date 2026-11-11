# Third-party notices

Decu6000 is licensed under the GNU Affero General Public License v3.0 or later
(see `LICENSE`). It is built on, and its binaries include, the following:

| Component | Used for | License |
|---|---|---|
| [JUCE](https://juce.com) 8 (fetched at build time) | Audio plug-in / app framework | AGPLv3 (Decu6000 uses JUCE under the AGPLv3) |
| [VST 3 SDK](https://github.com/steinbergmedia/vst3sdk) (bundled with JUCE) | VST3 plug-in format | GPLv3 option of the SDK as bundled with JUCE 8.0.4; MIT from SDK 3.8 |
| [Orbitron](https://github.com/theleagueof/orbitron) - instanced as "Decu Display" Bold/Black | Display font | SIL Open Font License 1.1 (`Resources/Fonts/OFL-Orbitron.txt`) |
| [Share Tech Mono](https://fonts.google.com/specimen/Share+Tech+Mono) | Mono font | SIL Open Font License 1.1 (`Resources/Fonts/OFL-ShareTechMono.txt`) |

The two display fonts are static instances generated from the Orbitron variable font
and renamed, because "Orbitron" is a Reserved Font Name under the OFL.

VST is a trademark of Steinberg Media Technologies GmbH. Decu6000 is not affiliated
with Steinberg.
