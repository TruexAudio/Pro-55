# Install Pro-55

## Windows x64

Pro-55 0.9.0 Public Beta targets Windows x64. Separate unsigned VST3-only and CLAP-only ZIPs are available, along with an optional combined ZIP.

1. Close your audio host, then extract the ZIP.
2. If your ZIP contains `Pro-55.vst3`, copy the complete folder, including its `Contents` directory, into your VST3 folder. The per-user standard location is `%LOCALAPPDATA%\Programs\Common\VST3`; the system-wide location is `C:\Program Files\Common Files\VST3`.
3. If your ZIP contains `Pro-55.clap`, copy it into the CLAP folder used by your host. The per-user standard location is `%LOCALAPPDATA%\Programs\Common\CLAP`; the system-wide location is `C:\Program Files\Common Files\CLAP`.
4. Restart the host and rescan plugins if necessary. Windows may require administrator permission to copy into system folders. Your host may support a configured user plugin folder.

A single-format ZIP contains only the plugin named in its filename. The combined ZIP contains both. Install only the formats your host supports. The development standalone is not included.

## Apple Silicon macOS beta

The macOS beta is for Apple Silicon (arm64) only. Download the separate AUv2, VST3, or CLAP ZIP, close your audio host, extract it, and copy the complete plugin bundle into the matching folder:

- AUv2 (`Pro-55.component`): `~/Library/Audio/Plug-Ins/Components`
- VST3 (`Pro-55.vst3`): `~/Library/Audio/Plug-Ins/VST3`
- CLAP (`Pro-55.clap`): `~/Library/Audio/Plug-Ins/CLAP`

Keep each bundle intact. Restart your host or rescan its plugins. GarageBand and Logic use the AUv2 build; VST3 and CLAP require a host that supports those formats.

These beta builds have an ad-hoc signature, with no Developer ID signature or notarization. macOS may display a security warning. The build targets macOS 11.0 but has only been tested on macOS 26.6.2; Intel Macs and earlier macOS versions are unverified.

## Sound storage and bank backup

With no user bank present, Pro-55 uses its embedded factory bank. The first completed RECORD save creates `%APPDATA%\Truex Audio\Pro55\UserBank.json` automatically.

Press RECORD once to capture the sound, select its destination, optionally rename it, then press RECORD again to save. Selecting a destination while armed does not load that destination's sound. Later knob edits do not change the captured sound.

FILE and BANK select which of the first two address digits the numbered buttons change; with neither selected, the buttons select the final digit. Each digit can also be dragged. Click the sound name to browse patches; use F2 or right-click → Rename when unarmed. While armed, click the name to edit it.

Back up `UserBank.json` before bank operations or updates. Saving a host project stores that instance's sound, selected address and name; it does not save the shared bank. The project sound remains independent of later bank edits.

Each instance holds its own bank copy. If another instance changes the shared file, Pro-55 asks before replacing it with the current instance's complete bank. Cancelling keeps the captured sound. Other open instances do not automatically reload the file.

If the bank is damaged, Pro-55 warns and uses factory sounds while protecting the damaged file from overwrite. Close all instances and move or rename that file as a backup before a deliberate reset or new bank save. Keep another copy of anything you want to preserve.

## Beta feedback

Use [GitHub Issues](https://github.com/TruexAudio/Pro-55/issues). Include your operating system and version, host and version, plugin format, sample rate, expected result, and steps to reproduce the issue. Share project, preset, or audio material only when you choose to.
