param(
    [Parameter(Mandatory=$true)][string]$JuceDir
)
$ErrorActionPreference = 'Stop'
# Apply only to a disposable build copy of JUCE 9.0.0.
# Exact matching deliberately stops the build if upstream code has changed.
function Replace-Once([string]$path, [string]$before, [string]$after) {
    $text = [IO.File]::ReadAllText($path).Replace("`r`n", "`n")
    if ($text.Contains($after)) { return }
    if (($text.Split([string[]]@($before), [StringSplitOptions]::None)).Count -ne 2) {
        throw "Expected one patch location in $path"
    }
    [IO.File]::WriteAllText($path, $text.Replace($before, $after), [Text.UTF8Encoding]::new($false))
}
$native = Join-Path $JuceDir 'modules/juce_gui_basics/native/juce_Windowing_windows.cpp'
Replace-Once $native 'return (localBounds.toDouble() / getPlatformScaleFactor()).toNearestInt();' @'
// Embedded plugin windows also need enough logical area to paint every physical pixel.
        return (localBounds.toDouble() / getPlatformScaleFactor()).getSmallestIntegerContainer();
'@
$vst3 = Join-Path $JuceDir 'modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp'
Replace-Once $vst3 'const auto rounded = owner->scaleManager.convertFromHostBounds (createRectangle (*newSize)).toNearestIntEdges();' @'
#if JUCE_WINDOWS
            const auto rounded = owner->scaleManager.convertFromHostBounds (createRectangle (*newSize)).getSmallestIntegerContainer();
           #else
            const auto rounded = owner->scaleManager.convertFromHostBounds (createRectangle (*newSize)).toNearestIntEdges();
           #endif
'@
Replace-Once $vst3 @'
peer->setBounds ((Rectangle { newSize->getWidth(), newSize->getHeight() }.toFloat()
                                         / peer->getPlatformScaleFactor()).toNearestInt(),
                                     false);
'@ @'
// Round outwards on Windows: any excess lies under the host's clip, while
                    // rounding down exposes a column/row of the host's background.
                    const auto logicalBounds = Rectangle { newSize->getWidth(), newSize->getHeight() }.toFloat()
                                               / peer->getPlatformScaleFactor();
                   #if JUCE_WINDOWS
                    peer->setBounds (logicalBounds.getSmallestIntegerContainer(), false);
                   #else
                    peer->setBounds (logicalBounds.toNearestInt(), false);
                   #endif
'@
Replace-Once $vst3 'pluginEditor->setBoundsConstrained (editorArea.withZeroOrigin());' @'
#if JUCE_WINDOWS
                    // The host already negotiates constraints before onSize. Fill the accepted
                    // allocation; constraining a second time can shrink one edge again.
                    pluginEditor->setBounds (editorArea.withZeroOrigin());
                   #else
                    pluginEditor->setBoundsConstrained (editorArea.withZeroOrigin());
                   #endif
'@
Write-Output 'Applied Windows GUI boundary candidate patches.'
