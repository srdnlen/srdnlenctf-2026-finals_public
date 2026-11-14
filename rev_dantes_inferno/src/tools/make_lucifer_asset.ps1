Add-Type -AssemblyName System.Drawing

$ProjectRoot = Split-Path -Parent $PSScriptRoot
$SourcePath = Join-Path $ProjectRoot "assets_online\Nightmare_Imp.png"
$DestPath = Join-Path $ProjectRoot "graphics\lucifer.bmp"

$img = [System.Drawing.Image]::FromFile($SourcePath)
$canvas = New-Object System.Drawing.Bitmap 64, 64, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$g = [System.Drawing.Graphics]::FromImage($canvas)
$g.Clear([System.Drawing.Color]::Transparent)
$g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
$g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::Half

$scale = [Math]::Min(58.0 / $img.Width, 58.0 / $img.Height)
$w = [int]($img.Width * $scale)
$h = [int]($img.Height * $scale)
$x = [int]((64 - $w) / 2)
$y = [int]((64 - $h) / 2)
$g.DrawImage($img, $x, $y, $w, $h)
$g.Dispose()

$bmp8 = New-Object System.Drawing.Bitmap 64, 64, ([System.Drawing.Imaging.PixelFormat]::Format8bppIndexed)
$palette = $bmp8.Palette
$colors = @(
    [System.Drawing.Color]::FromArgb(255, 255, 0, 255),
    [System.Drawing.Color]::FromArgb(255, 0, 0, 0),
    [System.Drawing.Color]::FromArgb(255, 18, 25, 31),
    [System.Drawing.Color]::FromArgb(255, 36, 57, 69),
    [System.Drawing.Color]::FromArgb(255, 58, 91, 106),
    [System.Drawing.Color]::FromArgb(255, 80, 136, 151),
    [System.Drawing.Color]::FromArgb(255, 130, 195, 204),
    [System.Drawing.Color]::FromArgb(255, 232, 250, 246),
    [System.Drawing.Color]::FromArgb(255, 55, 36, 30),
    [System.Drawing.Color]::FromArgb(255, 94, 58, 42),
    [System.Drawing.Color]::FromArgb(255, 146, 95, 65),
    [System.Drawing.Color]::FromArgb(255, 206, 151, 99),
    [System.Drawing.Color]::FromArgb(255, 238, 205, 157),
    [System.Drawing.Color]::FromArgb(255, 74, 28, 44),
    [System.Drawing.Color]::FromArgb(255, 132, 42, 70),
    [System.Drawing.Color]::FromArgb(255, 206, 72, 102)
)

for($i = 0; $i -lt $palette.Entries.Length; ++$i)
{
    $palette.Entries[$i] = [System.Drawing.Color]::FromArgb(255, 0, 0, 0)
}

for($i = 0; $i -lt $colors.Length; ++$i)
{
    $palette.Entries[$i] = $colors[$i]
}

$bmp8.Palette = $palette
$rect = New-Object System.Drawing.Rectangle 0, 0, 64, 64
$data = $bmp8.LockBits($rect, [System.Drawing.Imaging.ImageLockMode]::WriteOnly, [System.Drawing.Imaging.PixelFormat]::Format8bppIndexed)

try
{
    $bytes = New-Object byte[] ($data.Stride * 64)

    for($py = 0; $py -lt 64; ++$py)
    {
        for($px = 0; $px -lt 64; ++$px)
        {
            $c = $canvas.GetPixel($px, $py)
            $idx = 0

            if($c.A -ge 32)
            {
                $best = 1
                $bestDistance = [int]::MaxValue

                for($ci = 1; $ci -lt $colors.Length; ++$ci)
                {
                    $pc = $colors[$ci]
                    $dr = [int]$c.R - [int]$pc.R
                    $dg = [int]$c.G - [int]$pc.G
                    $db = [int]$c.B - [int]$pc.B
                    $distance = $dr * $dr + $dg * $dg + $db * $db

                    if($distance -lt $bestDistance)
                    {
                        $bestDistance = $distance
                        $best = $ci
                    }
                }

                $idx = $best
            }

            $bytes[$py * $data.Stride + $px] = [byte]$idx
        }
    }

    [System.Runtime.InteropServices.Marshal]::Copy($bytes, 0, $data.Scan0, $bytes.Length)
}
finally
{
    $bmp8.UnlockBits($data)
}

if(Test-Path $DestPath)
{
    Remove-Item -LiteralPath $DestPath
}

$bmp8.Save($DestPath, [System.Drawing.Imaging.ImageFormat]::Bmp)

$bmp8.Dispose()
$canvas.Dispose()
$img.Dispose()
