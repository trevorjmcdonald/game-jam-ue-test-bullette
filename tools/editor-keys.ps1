# Sends key presses to an Unreal Editor window with PostMessage, without focusing it or touching the user's other windows.
# Drives menus and gameplay input in PIE (or a standalone -game run with -ProcessId).
# Usage: tools/editor-keys.ps1 -Keys P,Down,Enter [-DelayMs 600] [-ProcessId <pid>]
# Escape stops PIE in the editor, so test pause with P there.
param([string[]]$Keys, [int]$DelayMs = 600, [int]$ProcessId = 0)
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class EditorKeys {
  [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
  [DllImport("user32.dll")] public static extern uint MapVirtualKey(uint code, uint mapType);
}
"@
$named = @{ Enter = 0x0D; Escape = 0x1B; Space = 0x20; Left = 0x25; Up = 0x26; Right = 0x27; Down = 0x28; Tab = 0x09 }
$p = if ($ProcessId) { Get-Process -Id $ProcessId } else { Get-Process UnrealEditor | Select-Object -First 1 }
foreach ($key in $Keys) {
  $vk = if ($named.ContainsKey($key)) { $named[$key] } else { [int][char]$key.ToUpper() }
  $lParam = 1 -bor ([int][EditorKeys]::MapVirtualKey($vk, 0) -shl 16)
  [void][EditorKeys]::PostMessage($p.MainWindowHandle, 0x100, [IntPtr]$vk, [IntPtr]$lParam)             # WM_KEYDOWN
  Start-Sleep -Milliseconds 80
  [void][EditorKeys]::PostMessage($p.MainWindowHandle, 0x101, [IntPtr]$vk, [IntPtr]($lParam -bor 0xC0000000))  # WM_KEYUP
  Start-Sleep -Milliseconds $DelayMs
}
"Sent $($Keys -join ',') to $($p.Id)"
