# Pro-55

**Is a Free Sequential Circuits Prophet 5-inspired synthesizer for Windows (MacOS soon!), available as VST3 and CLAP.** Pro-55 is a TrueX Audio instrument built around the Cmajor [Pro-54 synthesis engine](https://github.com/cmajor-lang/cmajor/tree/main/examples/patches/Pro54, itself a port of the Native Instruments Pro-53.

![Pro-55 interface](Pro55/docs/pro55-interface.png)

## The Story

Native Instruments built the first version Pro-Five in early 2000, then refined it into the Pro-52 later that same year. In late 2002, the Pro-53 was released and was widely regarded at the time as one of the best software recreations of a classic analog synth. NI discontinued the Pro-53 in 2020, and for a few years it simply wasn't available to buy or download anywhere.
Then in 2024, the Cmajor team brought the synth back to life as a web-playable instrument by porting an unreleased internal C++ version of the Pro-53 DSP to their Cmajor language, while also recreating its interface in HTML and JavaScript. That work preserved much of the original structure and became the foundation that made Pro-55 possible.

## What's New and What's Changed?
In addition to having full compatibility with modern DAWs via 64bit VST3 & CLAP formats, Pro-55 has had its rendering backend updated via JUCE with a revised and resizeable GUI that's compatible with large, modern monitors. The GUI was mostly rebuilt using Photopea, though some of CMajor's original assets remain.
Native Instruments actually developed alternative filter models for the Pro-53 which incorporated a more modern algorithm and the option for anti-aliasing which never saw commercial light-of-day but were present in CMajor's release. You'll noticed on the bottom-left-hand corner three options - Old, New, & New AA - More details in the Documentation.

I'm really excited to be able to give you a free and open-source (under GPL) CLAP and VST3 version of the plugin.

## Download and install

Choose the Windows x64 VST3-only, CLAP-only, or combined ZIP from [GitHub Releases](https://github.com/TruexAudio/Pro-55/releases). The beta is unsigned and has no installer. Follow the [installation and sound-storage guide](Pro55/INSTALLATION.md).

## Sound/Patch storage

The factory bank has 512 slots, numbered 111–888. To save a sound into the shared bank, press **RECORD** to capture it, choose a destination and optional name, then press **RECORD** again. Back up the user bank before replacing or resetting it. The [installation guide](Pro55/INSTALLATION.md) explains how project recall and the shared bank work.

## Known limitations

- Windows x64 only. macOS, Linux, AU, and the development standalone are not included (YET!).
- The plugins are unsigned and supplied without an installer, activation, or telemetry.
- An initial smoke test succeeded for both formats. The host and test coverage still need to be recorded; broader host and project-recall checks remain open.
- Earlier DAW tests were completed using CLAP & VST3 in Bitwig, VST3 in REAPER 6.15. REAPER 7 CLAP and FL Studio project save/reload remain unverified.

## Build from source

The complete source is available in the [GitHub repository](https://github.com/TruexAudio/Pro-55) and source archive. See the [Windows build instructions](Pro55/BUILDING.md) for the toolchain, included dependencies, and commands. The source archive contains the native project, DSP source, factory bank and source, tests, runtime assets, build utilities, and third-party notices.

For issue reports, use [GitHub Issues](https://github.com/TruexAudio/Pro-55/issues). Include your Windows version, host and version, plugin format, sample rate, and steps to reproduce the problem OR email t_r_uexaudio@gmail.com (remove the underscores).

## Credits and licences

New native code and the Pro-55 interface are by **Grant Overend (TrueX Audio)**. The interface includes adaptations from the [Cmajor Pro-54 example](https://cmajor.dev/docs/Examples/Pro54/), used with permission from Cmajor with attribution. The inherited synthesis DSP and factory sounds also come from that example, which credits the original Native Instruments Pro-53. Upstream notices are retained. See [licensing](Pro55/LICENSING.md), [third-party notices](Pro55/THIRD_PARTY_NOTICES.md), and [artwork and preset provenance](Pro55/ASSET-PROVENANCE.md).
