param(
    [Parameter(Mandatory=$true)][string]$JuceDir,
    [Parameter(Mandatory=$true)][string]$ClapDir,
    [string]$CmajorDir,
    [string]$Destination = (Join-Path $PSScriptRoot '../licenses/third-party')
)
$ErrorActionPreference = 'Stop'
New-Item -ItemType Directory -Force -Path $Destination | Out-Null
$items = @(
    @($JuceDir, 'LICENSE.md', 'JUCE-LICENSE.md'),
    @($ClapDir, 'LICENSE.md', 'clap-juce-extensions-MIT.txt'),
    @($ClapDir, 'clap-libs/clap/LICENSE', 'CLAP-MIT.txt'),
    @($ClapDir, 'clap-libs/clap-helpers/LICENSE', 'clap-helpers-MIT.txt'),
    @($JuceDir, 'modules/juce_audio_processors_headless/format_types/VST3_SDK/LICENSE.txt', 'VST3-MIT.txt'),
    @($JuceDir, 'modules/juce_audio_formats/codecs/flac/Flac Licence.txt', 'FLAC.txt'),
    @($JuceDir, 'modules/juce_audio_formats/codecs/oggvorbis/Ogg Vorbis Licence.txt', 'Ogg-Vorbis.txt'),
    @($JuceDir, 'modules/juce_graphics/image_formats/jpglib/README', 'JPEG-README.txt'),
    @($JuceDir, 'modules/juce_graphics/image_formats/pnglib/LICENSE', 'PNG.txt'),
    @($JuceDir, 'modules/juce_core/zip/zlib/LICENSE', 'zlib.txt'),
    @($JuceDir, 'modules/juce_graphics/fonts/harfbuzz/COPYING', 'HarfBuzz.txt'),
    @($JuceDir, 'modules/juce_graphics/unicode/sheenbidi/LICENSE', 'SheenBidi.txt'),
    @($JuceDir, 'modules/juce_graphics/drawables/lunasvg/LICENSE', 'LunaSVG.txt'),
    @($JuceDir, 'modules/juce_graphics/drawables/lunasvg/plutovg/LICENSE', 'PlutoVG.txt'),
    @($JuceDir, 'modules/juce_audio_processors_headless/format_types/LV2_SDK/lv2/COPYING', 'LV2.txt'),
    @($JuceDir, 'modules/juce_audio_processors_headless/format_types/LV2_SDK/lilv/COPYING', 'Lilv.txt'),
    @($JuceDir, 'modules/juce_audio_processors_headless/format_types/LV2_SDK/serd/COPYING', 'Serd.txt'),
    @($JuceDir, 'modules/juce_audio_processors_headless/format_types/LV2_SDK/sord/COPYING', 'Sord.txt'),
    @($JuceDir, 'modules/juce_audio_processors_headless/format_types/LV2_SDK/sratom/COPYING', 'Sratom.txt')
)
if ($CmajorDir) { $items += ,@($CmajorDir, 'LICENSE.md', 'Cmajor-LICENSE.md') }
if ($CmajorDir) { $items += ,@($CmajorDir, 'README.md', 'Cmajor-Pro54-README.md') }
$manifest = foreach ($item in $items) {
    $source = Join-Path $item[0] $item[1]
    $target = Join-Path $Destination $item[2]
    Copy-Item -LiteralPath $source -Destination $target
    [pscustomobject]@{notice=$item[2]; upstreamFile=$item[1]; sha256=(Get-FileHash -LiteralPath $target).Hash}
}
$manifest | ConvertTo-Json | Set-Content (Join-Path $Destination 'manifest.json')
Write-Output "Collected $($items.Count) upstream notice files without rewriting their contents."
