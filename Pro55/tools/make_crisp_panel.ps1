param(
    [Parameter(Mandatory = $true)] [string] $MockupPath,
    [Parameter(Mandatory = $true)] [string] $LogoPath,
    [Parameter(Mandatory = $true)] [string] $OutputPath
)

Add-Type -AssemblyName System.Drawing

$mockup = [System.Drawing.Bitmap]::FromFile($MockupPath)
$logoSource = [System.Drawing.Bitmap]::FromFile($LogoPath)

try {
    $composite = New-Object System.Drawing.Bitmap($mockup.Width, $mockup.Height,
        [System.Drawing.Imaging.PixelFormat]::Format32bppPArgb)
    $graphics = [System.Drawing.Graphics]::FromImage($composite)
    try {
        $graphics.DrawImageUnscaled($mockup, 0, 0)

        # Replace the old mark with timber sampled from the same horizontal strip.
        $graphics.Flush()
        $patchX = 45
        $patchY = 475
        $patchWidth = 400
        $patchHeight = 94
        $feather = 24.0
        for ($y = 0; $y -lt $patchHeight; ++$y) {
            for ($x = 0; $x -lt $patchWidth; ++$x) {
                $edge = [Math]::Min([Math]::Min($x, $patchWidth - 1 - $x),
                                    [Math]::Min($y, $patchHeight - 1 - $y))
                $mix = [Math]::Min(1.0, $edge / $feather)
                $old = $composite.GetPixel($patchX + $x, $patchY + $y)
                $wood = $mockup.GetPixel(455 + $x, $patchY + $y)
                $red = [int]($old.R * (1.0 - $mix) + $wood.R * $mix)
                $green = [int]($old.G * (1.0 - $mix) + $wood.G * $mix)
                $blue = [int]($old.B * (1.0 - $mix) + $wood.B * $mix)
                $composite.SetPixel($patchX + $x, $patchY + $y,
                    [System.Drawing.Color]::FromArgb(255, $red, $green, $blue))
            }
        }
        $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic

        # The supplied TrueX artwork is cream on a dark background. Convert its luminance to
        # transparency so the same lettering can be drawn in black over the timber.
        $logo = New-Object System.Drawing.Bitmap($logoSource.Width, $logoSource.Height,
            [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
        for ($y = 0; $y -lt $logoSource.Height; ++$y) {
            for ($x = 0; $x -lt $logoSource.Width; ++$x) {
                $pixel = $logoSource.GetPixel($x, $y)
                $luminance = [int](0.2126 * $pixel.R + 0.7152 * $pixel.G + 0.0722 * $pixel.B)
                $alpha = [Math]::Min(255, [Math]::Max(0, [int](($luminance - 80) * 1.8)))
                $logo.SetPixel($x, $y, [System.Drawing.Color]::FromArgb($alpha, 0, 0, 0))
            }
        }
        try {
            $graphics.DrawImage($logo, (New-Object System.Drawing.Rectangle(70, 494, 335, 66)))
        }
        finally {
            $logo.Dispose()
        }
    }
    finally {
        $graphics.Dispose()
    }

    # The mock-up is ten pixels wider than the editor's fixed aspect ratio. A centred crop
    # avoids stretching the circular controls, then the result is stored at true 2x size.
    $crop = New-Object System.Drawing.Rectangle(5, 0, 1819, 855)
    $output = New-Object System.Drawing.Bitmap(2286, 1074,
        [System.Drawing.Imaging.PixelFormat]::Format32bppPArgb)
    $outputGraphics = [System.Drawing.Graphics]::FromImage($output)
    try {
        $outputGraphics.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
        $outputGraphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
        $outputGraphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
        $outputGraphics.DrawImage($composite,
            (New-Object System.Drawing.Rectangle(0, 0, 2286, 1074)),
            $crop,
            [System.Drawing.GraphicsUnit]::Pixel)
        $output.Save($OutputPath, [System.Drawing.Imaging.ImageFormat]::Png)
    }
    finally {
        $outputGraphics.Dispose()
        $output.Dispose()
        $composite.Dispose()
    }
}
finally {
    $mockup.Dispose()
    $logoSource.Dispose()
}
