# Saves a half-size PNG of the Unreal Editor window only (PrintWindow renders it even when other windows cover it).
# Never screen-copy instead: that captures whatever the user has on top. Usage: tools/editor-shot.ps1 -Out <file.png>
# -ProcessId picks a specific UnrealEditor process (e.g. a standalone -game run next to the editor).
param([string]$Out, [int]$ProcessId = 0)
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class Win {
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint flags);
}
"@
$p = if ($ProcessId) { Get-Process -Id $ProcessId } else { Get-Process UnrealEditor | Select-Object -First 1 }
$r = New-Object Win+RECT
[void][Win]::GetWindowRect($p.MainWindowHandle, [ref]$r)
$w = $r.R - $r.L; $h = $r.B - $r.T
$bmp = New-Object System.Drawing.Bitmap $w, $h
$g = [System.Drawing.Graphics]::FromImage($bmp)
$hdc = $g.GetHdc()
$ok = [Win]::PrintWindow($p.MainWindowHandle, $hdc, 2)   # PW_RENDERFULLCONTENT
$g.ReleaseHdc($hdc)
$small = New-Object System.Drawing.Bitmap $bmp, ([int]($w / 2)), ([int]($h / 2))
$small.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
"PrintWindow=$ok $w x $h -> $Out"
