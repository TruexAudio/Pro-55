# Pro-55

**A polyphonic synthesizer for Windows, available as VST3 and CLAP.** Pro-55 is a TrueX Audio instrument built around the Cmajor Pro-54 synthesis engine, a port of the Native Instruments Pro-53. It accepts MIDI input and produces stereo audio.

![Pro-55 interface](Pro55/docs/pro55-interface.png)

## What it does

Pro-55 combines a polyphonic synthesizer with a 512-address patch programmer. Edit controls in the interface, automate supported parameters from your host, and save edited sounds with the project. The shared user bank is a separate file; changing it does not rewrite sounds already saved in a project.

## Download and install

Choose the Windows x64 VST3-only, CLAP-only, or combined ZIP from [GitHub Releases](https://github.com/TruexAudio/Pro-55/releases). The beta is unsigned and has no installer. Follow the [installation and sound-storage guide](Pro55/INSTALLATION.md).

For issue reports, use [GitHub Issues](https://github.com/TruexAudio/Pro-55/issues). Include your Windows version, host and version, plugin format, sample rate, and steps to reproduce the problem.

## Sound storage

The factory bank has 512 addresses, numbered 111–888. To save a sound into the shared bank, press **RECORD** to capture it, choose a destination and optional name, then press **RECORD** again. Back up the user bank before replacing or resetting it. The [installation guide](Pro55/INSTALLATION.md) explains how project recall and the shared bank work.

## Known limitations

- Windows x64 only. macOS, Linux, AU, and the development standalone are not included.
- The plugins are unsigned and supplied without an installer, activation, or telemetry.
- An initial post-identity smoke test succeeded for both formats. The host and test coverage still need to be recorded; broader host and project-recall checks remain open.
- Earlier host checks were before the identity change: CLAP in Bitwig and VST3 in REAPER 6.15. REAPER 7 CLAP and FL Studio project save/reload remain unverified for this identity.

## Build from source

The complete source is available in the [GitHub repository](https://github.com/TruexAudio/Pro-55) and source archive. See the [Windows build instructions](Pro55/BUILDING.md) for the toolchain, included dependencies, and commands. The source archive contains the native project, DSP source, factory bank and source, tests, runtime assets, build utilities, and third-party notices.

## Credits and licences

New native code and the Pro-55 interface are by **Grant Overend (TrueX Audio)**. The interface includes adaptations from the [Cmajor Pro-54 example](https://cmajor.dev/docs/Examples/Pro54/), used with permission from Cmajor with attribution. The inherited synthesis DSP and factory sounds also come from that example, which credits the original Native Instruments Pro-53. Upstream notices are retained. See [licensing](Pro55/LICENSING.md), [third-party notices](Pro55/THIRD_PARTY_NOTICES.md), and [artwork and preset provenance](Pro55/ASSET-PROVENANCE.md).
