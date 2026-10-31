# Pro-55

A free polyphonic synthesizer inspired by the Sequential Circuits Prophet-5, for Windows x64 and Apple Silicon macOS. Windows builds are available as VST3 and CLAP; the macOS beta also includes AUv2. Pro-55 is a TrueX Audio instrument built around the [Cmajor Pro-54 synthesis engine](https://github.com/cmajor-lang/cmajor/tree/main/examples/patches/Pro54), itself a port of Native Instruments' Pro-53.

![Pro-55 interface](Pro55/docs/pro55-interface.png)

## What it does

Pro-55 combines a polyphonic synthesizer with a 512-address patch programmer. Change controls in the interface, automate supported parameters from your host, and save edited sounds with your project. The shared user bank is separate; changing it does not alter sounds already saved in a project.

## Download and install

Windows x64 VST3-only, CLAP-only, and combined beta ZIPs are available from the [Pro-55 0.9.0-beta release](https://github.com/TruexAudio/Pro-55/releases/tag/v0.9.0-beta). Separate Apple Silicon macOS beta ZIPs for AUv2, VST3, and CLAP are available from the same release. Choose the format supported by your DAW. See the [installation and sound-storage guide](Pro55/INSTALLATION.md). A matching [Apple Silicon macOS source snapshot](https://github.com/TruexAudio/Pro-55/releases/download/v0.9.0-beta/Pro-55-0.9.0-beta-macOS-arm64-source.zip) is also included in the release assets.

The macOS beta has had limited host checks: AUv2 in GarageBand 10.4.14, and VST3 and CLAP in Bitwig Studio 6.0. The builds target macOS 11.0 but have only been checked on macOS 26.6.2 and Apple Silicon. They are ad-hoc signed, not Developer ID signed or notarized; macOS may show a security warning. Intel Macs and earlier macOS versions are unverified.

## Sound storage

The factory bank has 512 addresses, numbered 111–888. To save a sound into the shared bank, press **RECORD** to capture it, choose a destination and optional name, then press **RECORD** again. Back up the user bank before replacing or resetting it. The [installation guide](Pro55/INSTALLATION.md) explains how project recall and the shared bank work.

## Known limitations

- The macOS builds are experimental Apple Silicon beta packages. Their host coverage is limited; broad DAW compatibility, Intel Macs, and macOS 11 compatibility have not been established.
- The Windows x64 plugins are unsigned and have no installer, activation, or telemetry.
- After the Pro-55 identity change, initial smoke tests passed for both Windows VST3 and CLAP. Earlier DAW checks used the previous Pro-54 identity: Bitwig (VST3 and CLAP), REAPER 6.15 (VST3), plus checks in Ableton Live and FL Studio. REAPER 7 CLAP and FL Studio project save/reload remain unverified with Pro-55.

## Build from source

The full source code is in this repository. See the [build instructions](Pro55/BUILDING.md) for Windows and Apple Silicon macOS toolchains, included dependencies, and commands. The source includes the native project, DSP source, factory bank and source, tests, runtime assets, build utilities, and third-party notices.

## Credits and licensing

New Pro-55 code and the interface are by **Grant Overend (TrueX Audio)** and licensed under AGPL-3.0. Some control artwork is adapted from the [Cmajor Pro-54 example](https://cmajor.dev/docs/Examples/Pro54/), used with permission and attribution. The inherited DSP and factory sounds also come from that example. Its DSP source retains GPL-3.0-or-later terms; other dependencies retain their own licences. Cmajor credits the original Native Instruments Pro-53. See [licensing](Pro55/LICENSING.md), [third-party notices](Pro55/THIRD_PARTY_NOTICES.md), and [artwork and preset provenance](Pro55/ASSET-PROVENANCE.md).
