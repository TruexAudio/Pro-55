# Pro-55 0.9.0 — Windows beta

Pro-55 is a native Windows x64 VST3 and CLAP synthesizer with a resizable interface, host automation, and a 512-address patch programmer.

## Included

- VST3 and CLAP plugin formats for Windows x64.
- User-bank saving and host project recall for edited sounds.
- The inherited factory sound bank, with source attribution and notices.
- Native interface and programmer implementation by Grant Overend (TrueX Audio).

## Compatibility notes

An initial post-identity smoke test succeeded for both VST3 and CLAP. The host and test coverage still need to be recorded, so broader host compatibility and project recall remain open. Earlier host checks were before the identity change: CLAP in Bitwig and VST3 in REAPER 6.15. REAPER 7 CLAP and FL Studio project save/reload remain unverified for this identity.

This beta is unsigned and has no installer. macOS, Linux, AU, and the development standalone are not included.

The Windows x64 VST3, CLAP, and standalone targets build from the supplied source. The three registered regression suites pass, as do the GUI boundary probes. Host loading and project recall still need checks with the new Pro-55 plugin IDs.

Artwork and component attribution are documented in [ASSET-PROVENANCE.md](ASSET-PROVENANCE.md), [LICENSING.md](LICENSING.md), and [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
