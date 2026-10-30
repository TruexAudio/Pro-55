# Third-party notices — Pro-55 0.9.0

Upstream notices remain authoritative. This table identifies the components used by Pro-55; accompanying notice files retain upstream text. New project contributions and inherited-content attribution are described in [LICENSING.md](LICENSING.md).

| Component | Source and release treatment | Retained notice |
| --- | --- | --- |
| Cmajor Pro-54 DSP and inherited preset-bank source | Cmajor Software Ltd, (C)2024 in the supplied headers; GPLv3 (or later) option stated by the accompanying upstream root licence for Cmajor-marked files. The example describes a port of Native Instruments Pro-53. | Original source headers; `licenses/GPL-3.0.txt`; `licenses/third-party/Cmajor-LICENSE.md`, `Cmajor-Pro54-README.md` |
| JUCE 9.0.0 | Raw Material Software Limited; AGPLv3 option. Project-specific Windows boundary patches are supplied separately. | `licenses/third-party/JUCE-LICENSE.md`; `licenses/AGPL-3.0.txt` |
| clap-juce-extensions | Paul Walker, copyright 2019–2020 in the pinned root notice; MIT. The project-specific Windows coverage patch does not replace that notice. | `licenses/third-party/clap-juce-extensions-MIT.txt` |
| CLAP and clap-helpers | Alexandre BIQUE, copyright 2021 in the pinned notices; MIT. | `licenses/third-party/CLAP-MIT.txt`, `clap-helpers-MIT.txt` |
| VST3 SDK | Steinberg; MIT in the supplied JUCE 9 SDK. This does not imply endorsement or ownership of Steinberg names. | `licenses/third-party/VST3-MIT.txt` |
| FLAC, Ogg/Vorbis | Embedded audio-format code; retain the supplied BSD-family notices. | `FLAC.txt`, `Ogg-Vorbis.txt` |
| JPEG | Independent JPEG Group and contributors; retain the supplied README and its copyright/licence conditions. | `JPEG-README.txt` |
| PNG and zlib | Retain the supplied PNG/zlib notices. | `PNG.txt`, `zlib.txt` |
| HarfBuzz | Embedded font shaping; supplied COPYING file lists its licence and contributors. | `HarfBuzz.txt` |
| SheenBidi | Embedded Unicode bidirectional processing; supplied Apache notice. | `SheenBidi.txt` |
| LunaSVG and PlutoVG | Embedded SVG/drawing code; supplied MIT notices. | `LunaSVG.txt`, `PlutoVG.txt` |
| LV2, Lilv, Serd, Sord, Sratom | Processor-format source retained within the supplied JUCE tree. Pro-55 does not ship an LV2 plugin; this collection covers bundled source as well as enabled binary dependencies. | `LV2.txt`, `Lilv.txt`, `Serd.txt`, `Sord.txt`, `Sratom.txt` |

All abbreviated notice paths in the table are under `licenses/third-party`. `manifest.json` records the original upstream file path and SHA-256 for each copied notice. Copyright details inside those full notices are retained, not replaced by this summary.

The dependency source trees retain additional notices for platform and format components that are not part of the Windows VST3/CLAP build. Their original notice files remain with the supplied dependency source.

The inherited panel/control graphics and factory sounds include material from the Cmajor Pro-54 example. Original source, README and root licence notices are retained. Cmajor's root licence does not by itself identify ownership or terms for every unmarked image. See [asset and preset provenance](ASSET-PROVENANCE.md).
