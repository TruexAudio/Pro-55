# Build Pro-55 on Windows

The source ZIP includes the plugin source and the pinned JUCE and CLAP integration dependencies. You do not need to install Cmajor or regenerate the DSP engine for a normal build.

## Requirements

- Windows x64
- CMake 3.24 or later
- Visual Studio 2022 or later with the C++ desktop workload and Windows SDK
- Node.js only if you want to regenerate the inherited factory bank

The release candidate was built with CMake 4.3.2 and Visual Studio 18 2026 / MSVC 19.50.35723.0. Other toolchains have not been checked for this build.

## Build plugins and run the checks

Open PowerShell in the repository's `Pro55` folder and run:

```powershell
$juceSource = (Resolve-Path ../vendor/JUCE).Path
$clapSource = (Resolve-Path ../vendor/clap-juce-extensions).Path
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DJUCE_DIR="$juceSource" -DCLAP_JUCE_EXTENSIONS_DIR="$clapSource" -DPRO55_ALT_GUI=ON -DPRO55_BUILD_TESTS=ON
cmake --build build --config Release --target Pro55_VST3 Pro55_CLAP Pro55_Standalone Pro55FormatTests Pro55ProgrammerTests Pro55AudioSmokeTest --parallel 4
ctest --test-dir build -C Release --output-on-failure
```

If Visual Studio runs short of memory, use one compiler worker and limit its internal parallel compile pool:

```powershell
cmake --build build --config Release --target Pro55_VST3 Pro55_CLAP Pro55_Standalone Pro55FormatTests Pro55ProgrammerTests Pro55AudioSmokeTest --parallel 1 -- /p:CL_MPCount=1
```

The VST3, CLAP, and standalone are built for Windows x64. The standalone is useful for development and is not part of the beta download. The test executables use isolated data and do not write to your real user bank.

The Windows x64 build was checked from this source layout on 29 October 2026 using CMake 4.3.2, Visual Studio 18 2026 / MSVC 19.50.35723.0, and Windows SDK 10.0.26100. The VST3, CLAP, and standalone targets built; all three CTest suites passed. The GUI boundary probe also passed its VST3 allocation checks and 55 CLAP sizing cases. These checks do not replace testing the new plugin IDs in audio hosts.

## Dependency notes

The supplied JUCE 9.0.0 and CLAP integration sources include the small Windows editor-sizing fixes used by this project. The patch scripts in `scripts/` can reapply those changes to pristine dependency checkouts. The source tree also records the upstream revisions and retained notices.

To regenerate the inherited factory bank from its included source, run from `source/Pro55`:

```powershell
node tools/convert_factory_bank.mjs provenance/presetBank.js presets/Factory.pro55bank
```

This regeneration step is optional and is not required to build the plugins.

## macOS Apple Silicon

The macOS build uses the included JUCE 9.0.0 and CLAP integration sources. Build on an Apple Silicon Mac with CMake 3.24 or later and Xcode Command Line Tools installed. The release build was made on macOS 26.6.2 with Apple Clang 21.0.0. Its deployment target is macOS 11.0, but macOS 11 compatibility has not been tested.

Open Terminal in the repository's `Pro55` folder and run:

```sh
cmake -S . -B build-macos -G "Unix Makefiles" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
  -DJUCE_DIR="$PWD/../vendor/JUCE" \
  -DCLAP_JUCE_EXTENSIONS_DIR="$PWD/../vendor/clap-juce-extensions" \
  -DPRO55_ALT_GUI=ON \
  -DPRO55_BUILD_TESTS=OFF

cmake --build build-macos --config Release --parallel 4 \
  --target Pro55_AU Pro55_VST3 Pro55_CLAP
```

The AUv2, VST3, and CLAP bundles are written under `build-macos/Pro55_artefacts/Release/`. The published beta was signed ad-hoc and passed strict signature verification after all generated bundle files were present. It has no Developer ID signature or notarization. The Mac beta received limited checks in GarageBand and Bitwig Studio; other hosts and older macOS releases remain unverified.
