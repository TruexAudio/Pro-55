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
