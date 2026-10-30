<#
.SYNOPSIS
Generates a full-panel keyboard editing guide and machine-readable per-key colour map.

.DESCRIPTION
Reproduces the exact regular JUCE keyboard geometry currently used by PluginEditor.cpp,
projected directly from the 762x358 design space into a requested panel size. The output
is intended to be perspective-corrected over the photographic keyboard in an image editor and
returned as a PNG for use as a runtime visual/hit-test mask.
#>
param(
    [int]$Width = 1498,
    [int]$Height = 704,
    [string]$OutDir = "$PSScriptRoot\..\..\keyboard-map-out"
)

Add-Type -AssemblyName System.Drawing
$ErrorActionPreference = 'Stop'

$designW = 762.0
$designH = 358.0
$rangeStart = 36
$rangeEnd = 96
$whiteKeyCount = 36.0
$blackWidthRatio = 0.7
$blackHeightRatio = 0.7

function Round-Edge([double]$value) {
    [math]::Round($value, [MidpointRounding]::AwayFromZero)
}

function Get-LayoutRect([double]$x, [double]$y, [double]$w, [double]$h) {
    $left = Round-Edge ($x * $Width / $designW)
    $top = Round-Edge ($y * $Height / $designH)
    $right = Round-Edge (($x + $w) * $Width / $designW)
    $bottom = Round-Edge (($y + $h) * $Height / $designH)
    [System.Drawing.Rectangle]::FromLTRB($left, $top, $right, $bottom)
}

$keyboardBounds = Get-LayoutRect 82 249 668 102
$keyWidth = $keyboardBounds.Width / $whiteKeyCount
$notePositions = [double[]]@(0.0,
                             (1.0 - $blackWidthRatio * 0.6),
                             1.0,
                             (2.0 - $blackWidthRatio * 0.4),
                             2.0,
                             3.0,
                             (4.0 - $blackWidthRatio * 0.7),
                             4.0,
                             (5.0 - $blackWidthRatio * 0.5),
                             5.0,
                             (6.0 - $blackWidthRatio * 0.3),
                             6.0)
$blackPitchClasses = @(1, 3, 6, 8, 10)
$noteNames = @('C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B')

function Test-BlackKey([int]$note) {
    $blackPitchClasses -contains ($note % 12)
}

function Get-KeyStart([int]$note) {
    $octave = [math]::Floor($note / 12)
    ($octave * 7.0 + $notePositions[$note % 12]) * $keyWidth
}

$rangeStartPosition = Get-KeyStart $rangeStart

function Get-KeyRect([int]$note) {
    $isBlack = Test-BlackKey $note
    $localX = (Get-KeyStart $note) - $rangeStartPosition
    $keyW = if ($isBlack) { $keyWidth * $blackWidthRatio } else { $keyWidth }
    $keyH = if ($isBlack) { $keyboardBounds.Height * $blackHeightRatio } else { $keyboardBounds.Height }
    [System.Drawing.RectangleF]::new(
        [single]($keyboardBounds.X + $localX),
        [single]$keyboardBounds.Y,
        [single]$keyW,
        [single]$keyH)
}

function Get-NoteName([int]$note) {
    $octave = [math]::Floor($note / 12) - 2
    "$($noteNames[$note % 12])$octave"
}

# A permuted 4x4x4 RGB cube keeps neighbouring note colours well separated while
# remaining easy to quantise back to the nearest one after interpolation in the image editor.
function Get-KeyColour([int]$note) {
    $index = $note - $rangeStart
    $permuted = ($index * 37) % 64
    $levels = @(32, 96, 160, 224)
    $r = $levels[$permuted % 4]
    $g = $levels[[math]::Floor($permuted / 4) % 4]
    $b = $levels[[math]::Floor($permuted / 16) % 4]
    [System.Drawing.Color]::FromArgb(255, $r, $g, $b)
}

if (-not (Test-Path -LiteralPath $OutDir)) {
    New-Item -ItemType Directory -Path $OutDir -Force | Out-Null
}
$OutDir = (Resolve-Path -LiteralPath $OutDir).Path

$map = New-Object System.Drawing.Bitmap($Width, $Height, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$mapGraphics = [System.Drawing.Graphics]::FromImage($map)
$mapGraphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
$mapGraphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::Half
$mapGraphics.Clear([System.Drawing.Color]::Transparent)

$rows = New-Object System.Collections.Generic.List[object]

# White keys first, then black keys, matching JUCE's paint order and yielding the visible
# notched white-key silhouettes automatically where black keys overlap them.
$drawOrder = @($rangeStart..$rangeEnd | Where-Object { -not (Test-BlackKey $_) }) +
             @($rangeStart..$rangeEnd | Where-Object { Test-BlackKey $_ })

foreach ($note in $drawOrder) {
    $rect = Get-KeyRect $note
    $colour = Get-KeyColour $note
    $brush = New-Object System.Drawing.SolidBrush($colour)
    $mapGraphics.FillRectangle($brush, $rect)
    $brush.Dispose()
}

foreach ($note in $rangeStart..$rangeEnd) {
    $rect = Get-KeyRect $note
    $colour = Get-KeyColour $note
    $rows.Add([pscustomobject]@{
        MidiNote = $note
        NoteName = Get-NoteName $note
        KeyType = if (Test-BlackKey $note) { 'black' } else { 'white' }
        ColourHex = ('#{0:X2}{1:X2}{2:X2}' -f $colour.R, $colour.G, $colour.B)
        PanelX = [math]::Round($rect.X, 4)
        PanelY = [math]::Round($rect.Y, 4)
        PanelW = [math]::Round($rect.Width, 4)
        PanelH = [math]::Round($rect.Height, 4)
        KeyboardLocalX = [math]::Round($rect.X - $keyboardBounds.X, 4)
        KeyboardLocalY = [math]::Round($rect.Y - $keyboardBounds.Y, 4)
    })
}

$mapPath = Join-Path $OutDir 'keyboard-keymap.png'
$map.Save($mapPath, [System.Drawing.Imaging.ImageFormat]::Png)

$guide = New-Object System.Drawing.Bitmap($Width, $Height, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$guideGraphics = [System.Drawing.Graphics]::FromImage($guide)
$guideGraphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
$guideGraphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::Half
$guideGraphics.Clear([System.Drawing.Color]::Transparent)
$whiteFill = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(34, 0, 220, 255))
$whitePen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(255, 0, 220, 255), 1)
$blackFill = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(54, 255, 40, 210))
$blackPen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(255, 255, 40, 210), 1)
$whiteTextBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 0, 220, 255))
$blackTextBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 255, 220, 255))
$font = New-Object System.Drawing.Font('Consolas', 7, [System.Drawing.FontStyle]::Bold, [System.Drawing.GraphicsUnit]::Pixel)

foreach ($note in $rangeStart..$rangeEnd | Where-Object { -not (Test-BlackKey $_) }) {
    $rect = Get-KeyRect $note
    $guideGraphics.FillRectangle($whiteFill, $rect)
    $guideGraphics.DrawRectangle($whitePen, $rect.X, $rect.Y, $rect.Width, $rect.Height)
    $label = "$(Get-NoteName $note) $note"
    $guideGraphics.DrawString($label, $font, $whiteTextBrush, $rect.X + 1, $rect.Bottom - 10)
}

foreach ($note in $rangeStart..$rangeEnd | Where-Object { Test-BlackKey $_ }) {
    $rect = Get-KeyRect $note
    # Remove white-key guide pixels that would otherwise show through the black-key shape.
    $guideGraphics.CompositingMode = [System.Drawing.Drawing2D.CompositingMode]::SourceCopy
    $clearBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::Transparent)
    $guideGraphics.FillRectangle($clearBrush, $rect)
    $clearBrush.Dispose()
    $guideGraphics.CompositingMode = [System.Drawing.Drawing2D.CompositingMode]::SourceOver
    $guideGraphics.FillRectangle($blackFill, $rect)
    $guideGraphics.DrawRectangle($blackPen, $rect.X, $rect.Y, $rect.Width, $rect.Height)
    $guideGraphics.DrawString("$(Get-NoteName $note) $note", $font, $blackTextBrush, $rect.X + 1, $rect.Y + 2)
}

# A full-canvas registration frame makes the otherwise transparent overlay easy to snap
# precisely to the 1498x704 panel bounds in the image editor. Fill explicit one-pixel strips so
# every frame pixel remains inside the canvas (DrawRectangle can straddle its outer edge).
$frameBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 255, 210, 0))
$guideGraphics.FillRectangle($frameBrush, 0, 0, $Width, 1)
$guideGraphics.FillRectangle($frameBrush, 0, $Height - 1, $Width, 1)
$guideGraphics.FillRectangle($frameBrush, 0, 0, 1, $Height)
$guideGraphics.FillRectangle($frameBrush, $Width - 1, 0, 1, $Height)
$frameBrush.Dispose()

$guidePath = Join-Path $OutDir 'keyboard-grid-labelled.png'
$guide.Save($guidePath, [System.Drawing.Imaging.ImageFormat]::Png)
$rows | Export-Csv -LiteralPath (Join-Path $OutDir 'keyboard-keymap.csv') -NoTypeInformation -Encoding UTF8

$readme = @"
PRO-55 PERSPECTIVE KEYBOARD MAP

Canvas: ${Width}x${Height}
Keyboard bounds: X=$($keyboardBounds.X), Y=$($keyboardBounds.Y), W=$($keyboardBounds.Width), H=$($keyboardBounds.Height)
Notes: MIDI $rangeStart-$rangeEnd (61 keys)

FILES
- keyboard-grid-labelled.png: visible alignment guide for placing over the Pro-55 panel.
  Its yellow outer frame is exactly ${Width}x${Height} for edge-to-edge registration.
- keyboard-keymap.png: machine-readable key shapes; every key has a unique solid colour.
- keyboard-keymap.csv: mapping from colour to MIDI note and the current JUCE rectangles.

MAP EDITING WORKFLOW
1. Keep the document and exported PNG exactly ${Width}x${Height}; do not crop or resize it.
2. Place keyboard-grid-labelled.png above the panel to see the current calculated geometry.
3. Edit/warp keyboard-keymap.png so every coloured region matches the visible physical key beneath it.
4. Preserve one distinct colour per key and transparency everywhere outside the key surfaces.
5. Soft/anti-aliased edges from transforms are acceptable; they will be quantised to the nearest palette colour.
6. Export the edited machine map as a 32-bit PNG with transparency and return it with the CSV unchanged.

The map will remain invisible in Pro-55. It will define the per-note grey pressed-key mask and can
also be used for perspective-correct mouse hit-testing.
"@
[System.IO.File]::WriteAllText((Join-Path $OutDir 'README.txt'), $readme)

$font.Dispose()
$whiteFill.Dispose()
$whitePen.Dispose()
$blackFill.Dispose()
$blackPen.Dispose()
$whiteTextBrush.Dispose()
$blackTextBrush.Dispose()
$guideGraphics.Dispose()
$guide.Dispose()
$mapGraphics.Dispose()
$map.Dispose()

Write-Host "Wrote keyboard map package to $OutDir"
Write-Host "Panel ${Width}x${Height}; keyboard $($keyboardBounds.Width)x$($keyboardBounds.Height) at ($($keyboardBounds.X),$($keyboardBounds.Y))"
