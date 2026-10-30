<#
.SYNOPSIS
Renders a transparent PNG overlay (plus a CSV) showing exactly where
src/PluginEditor.cpp positions every knob, button, wheel, slider and
digit display on the Pro-55 panel, for use as a reference layer when
painting/positioning background artwork (e.g. in Photopea).

Reproduces PluginEditor.cpp's layoutRect(): every control is authored
in a 762x358 design-space grid and projected directly into the requested
output size. Only the final left, top, right and bottom edges are rounded.

.PARAMETER Width
Output image width in pixels. Defaults to 1524 (matches the original
panel_legacy.png resolution currently in assets/).

.PARAMETER Height
Output image height in pixels. Defaults to 716.

.PARAMETER OutDir
Directory to write control-grid.png and control-grid.csv into.

.PARAMETER NoLabels
Omits text labels so the overlay contains only exact pixel-footprint outlines.
#>
param(
    [int]$Width = 1524,
    [int]$Height = 716,
    [string]$OutDir = "$PSScriptRoot\..\..\control-grid-out",
    [switch]$NoLabels
)

Add-Type -AssemblyName System.Drawing

$designW = 762.0
$designH = 358.0
$sx = $Width / $designW
$sy = $Height / $designH

function Get-LayoutRect([double]$x, [double]$y, [double]$w, [double]$h) {
    $left   = [math]::Round($x * $sx, [MidpointRounding]::AwayFromZero)
    $top    = [math]::Round($y * $sy, [MidpointRounding]::AwayFromZero)
    $right  = [math]::Round(($x + $w) * $sx, [MidpointRounding]::AwayFromZero)
    $bottom = [math]::Round(($y + $h) * $sy, [MidpointRounding]::AwayFromZero)
    [pscustomobject]@{
        X = $left
        Y = $top
        W = $right - $left
        H = $bottom - $top
    }
}

# id, design-x, design-y, type ("knob" or "button") -- from controlSpecs[] in PluginEditor.cpp
$controlSpecs = @(
    @("PolyModFilterEnv", 23, 23, "knob"),   @("PolyModOscB", 61, 23, "knob"),
    @("PolyModFreqA", 95, 26, "button"),     @("PolyModPWA", 115, 26, "button"),
    @("PolyModFilt", 135, 26, "button"),     @("OscAFreq", 168, 23, "knob"),
    @("OscASaw", 202, 26, "button"),         @("OscAPulse", 222, 26, "button"),
    @("OscAPW", 244, 23, "knob"),            @("OscASync", 281, 26, "button"),
    @("MixerOscALevel", 313, 23, "knob"),    @("MixerOscBLevel", 349, 23, "knob"),
    @("MixerNoiseLevel", 385, 23, "knob"),   @("ExternalInputLevel", 385, 69, "knob"),
    @("FilterCutoff", 432, 23, "knob"),      @("FilterResonance", 468, 23, "knob"),
    @("FilterEnvAmt", 504, 23, "knob"),      @("FilterKeyboardTracking", 540, 23, "knob"),
    @("FilterAttack", 432, 69, "knob"),      @("FilterDecay", 468, 69, "knob"),
    @("FilterSustain", 504, 69, "knob"),     @("FilterRelease", 540, 69, "knob"),
    @("DelayTime", 607, 23, "knob"),         @("DelaySpread", 643, 23, "knob"),
    @("DelayDepth", 679, 23, "knob"),        @("DelayRate", 715, 23, "knob"),
    @("DelayFeedback", 607, 69, "knob"),     @("DelayHiCut", 643, 69, "knob"),
    @("DelayLoCut", 679, 69, "knob"),        @("DelayINV", 721, 72, "button"),
    @("DelayON", 613, 118, "button"),        @("DelayWet", 643, 115, "knob"),
    @("DelaySync", 685, 118, "button"),      @("DelayMidi", 721, 118, "button"),
    @("LfoMidiSync", 41, 72, "button"),      @("LfoFrequency", 61, 69, "knob"),
    @("LfoShapeSaw", 95, 72, "button"),      @("LfoShapeTri", 115, 72, "button"),
    @("LfoShapePulse", 135, 72, "button"),   @("OscBFreq", 168, 69, "knob"),
    @("OscBFreqFine", 206, 69, "knob"),      @("OscBShapeSaw", 240, 72, "button"),
    @("OscBShapeTri", 260, 72, "button"),    @("OscBShapePulse", 280, 72, "button"),
    @("OscBPWAmount", 302, 69, "knob"),      @("OscBSubOsc", 336, 72, "button"),
    @("OscBKKeyboardTracking", 356, 72, "button"), @("WheelModulationLfoNoise", 23, 115, "knob"),
    @("WheelModulationFreqOscA", 55, 118, "button"), @("WheelModulationFreqOscB", 75, 118, "button"),
    @("WheelModulationPWA", 95, 118, "button"),      @("WheelModulationPWB", 115, 118, "button"),
    @("WheelModulationFilter", 135, 118, "button"),  @("Glide", 168, 115, "knob"),
    @("Unison", 212, 118, "button"),         @("AmplifierAttack", 432, 115, "knob"),
    @("AmplifierDecay", 468, 115, "knob"),   @("AmplifierSustain", 504, 115, "knob"),
    @("AmplifierRelease", 540, 115, "knob"), @("Release", 386, 118, "button"),
    @("Velocity", 346, 118, "button"),       @("Repeat", 21, 72, "button"),
    @("Drone", 574, 118, "button"),          @("FilterHPF", 574, 26, "button"),
    @("FilterInvertEnv", 574, 72, "button"), @("Analog", 302, 115, "knob"),
    @("MasterTune", 607, 161, "knob"),       @("Volume", 679, 161, "knob"),
    @("TestTone", 649, 164, "button")
)

# Elements whose position/size is set explicitly (resized()/paint() in PluginEditor.cpp)
# rather than through controlSpecs[]: id, x, y, w, h, type
$fixedControls = @(
    @("SaveButton", 174, 164, 13, 19, "button"),
    @("SaveAsButton", 202, 164, 13, 19, "button"),
    @("BankButton", 222, 164, 13, 19, "button"),
    @("ProgramDisplay", 246, 166, 30, 14, "display"),
    @("PresetName", 454, 166, 126, 13, "display"),
    @("PresetSelector", 454, 164, 128, 17, "display"),
    @("PitchWheel", 27, 258, 10, 71, "wheel"),
    @("ModWheel", 55, 258, 10, 71, "wheel"),
    @("FilterVersion", 24, 162, 50, 28, "display"),
    @("ActiveVoices/VoicesDigits", 250, 119, 20, 14, "display"),
    @("ProgramDigit1", 246, 166, 10, 14, "display"),
    @("ProgramDigit2", 256, 166, 10, 14, "display"),
    @("ProgramDigit3", 266, 166, 10, 14, "display"),
    @("MidiLight", 723, 168, 9, 9, "indicator"),
    @("BpfBadge", 573, 47, 17, 9, "indicator"),
    @("Keyboard", 82, 249, 668, 102, "keyboard"),
    @("WoodenPanel", 15.24, 208, 731.52, 33, "panel")
)
$fixedControls += @(
    @("ProgramButton1", 286, 164, 13, 19, "button"),
    @("ProgramButton2", 306, 164, 13, 19, "button"),
    @("ProgramButton3", 326, 164, 13, 19, "button"),
    @("ProgramButton4", 346, 164, 13, 19, "button"),
    @("ProgramButton5", 366, 164, 13, 19, "button"),
    @("ProgramButton6", 386, 164, 13, 19, "button"),
    @("ProgramButton7", 406, 164, 13, 19, "button"),
    @("ProgramButton8", 426, 164, 13, 19, "button")
)

$knobBox   = 25.0
$buttonW   = 13.0
$buttonH   = 19.0
$colours = @{
    knob      = [System.Drawing.Color]::FromArgb(255, 0, 220, 220)   # cyan
    button    = [System.Drawing.Color]::FromArgb(255, 255, 210, 0)   # yellow
    wheel     = [System.Drawing.Color]::FromArgb(255, 255, 0, 220)   # magenta
    display   = [System.Drawing.Color]::FromArgb(255, 60, 255, 60)   # green
    indicator = [System.Drawing.Color]::FromArgb(255, 60, 255, 60)   # green
    keyboard  = [System.Drawing.Color]::FromArgb(255, 180, 180, 180) # grey
    panel     = [System.Drawing.Color]::FromArgb(255, 180, 180, 180) # grey
}

$bitmap = New-Object System.Drawing.Bitmap($Width, $Height)
$graphics = [System.Drawing.Graphics]::FromImage($bitmap)
$graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
$graphics.Clear([System.Drawing.Color]::Transparent)

$font = New-Object System.Drawing.Font("Consolas", 7)
$rows = New-Object System.Collections.Generic.List[object]

function Add-Row($id, $type, $designX, $designY, $designW, $designH, $rect) {
    $rows.Add([pscustomobject]@{
        Id = $id; Type = $type
        DesignX = $designX; DesignY = $designY; DesignW = $designW; DesignH = $designH
        PixelX = $rect.X; PixelY = $rect.Y; PixelW = $rect.W; PixelH = $rect.H
    })
}

function Draw-Control($id, $type, $rect) {
    $colour = $colours[$type]
    $brush = New-Object System.Drawing.SolidBrush($colour)
    $x = [int]$rect.X; $y = [int]$rect.Y
    $w = [int]$rect.W; $h = [int]$rect.H
    if ($w -le 0 -or $h -le 0) { throw "Invalid rectangle for ${id}: ${w}x${h}" }

    # Fill four one-pixel strips inside the control footprint. Unlike DrawRectangle(x,y,w,h),
    # this occupies exactly x..x+w-1 and y..y+h-1: a 26x38 control measures 26x38 in Photopea.
    $graphics.FillRectangle($brush, $x,          $y,          $w, 1)
    $graphics.FillRectangle($brush, $x,          $y + $h - 1, $w, 1)
    $graphics.FillRectangle($brush, $x,          $y,           1, $h)
    $graphics.FillRectangle($brush, $x + $w - 1, $y,           1, $h)

    if (-not $NoLabels) {
        $labelY = if ($type -eq "keyboard" -or $type -eq "panel") { $rect.Y + 4 } else { $rect.Y - 9 }
        $graphics.DrawString($id, $font, $brush, [float]$rect.X, [float]$labelY)
    }

    $brush.Dispose()
}

foreach ($spec in $controlSpecs) {
    $id, $x, $y, $type = $spec
    $w = if ($type -eq "knob") { $knobBox } else { $buttonW }
    $h = if ($type -eq "knob") { $knobBox } else { $buttonH }
    $rect = Get-LayoutRect $x $y $w $h
    Draw-Control $id $type $rect
    Add-Row $id $type $x $y $w $h $rect
}

foreach ($spec in $fixedControls) {
    $id, $x, $y, $w, $h, $type = $spec
    $rect = Get-LayoutRect $x $y $w $h
    Draw-Control $id $type $rect
    Add-Row $id $type $x $y $w $h $rect
}

if (-not (Test-Path $OutDir)) {
    New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
}
$OutDir = (Resolve-Path $OutDir).Path

$pngPath = Join-Path $OutDir "control-grid.png"
$csvPath = Join-Path $OutDir "control-grid.csv"

$bitmap.Save($pngPath, [System.Drawing.Imaging.ImageFormat]::Png)
$rows | Export-Csv -Path $csvPath -NoTypeInformation -Encoding UTF8

$graphics.Dispose()
$bitmap.Dispose()
$font.Dispose()

Write-Host "Wrote $pngPath ($Width x $Height) and $csvPath"
