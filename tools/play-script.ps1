# Plays scripted runs of the game (UJamKitScriptSubsystem) and reports what passed: each script launches the game in
# its own window with -jamscript=<file>, plays its steps through the real input and quits. Results and shots land in
# Saved/Script/<name>/. Works while the editor is open (it runs the built editor binaries with -game, not Live Coding
# patches).
#
#   powershell -File tools/play-script.ps1 Tests/Scripts/some_script.json
#   powershell -File tools/play-script.ps1 Tests/Scripts            # every script in the folder
#   powershell -File tools/play-script.ps1 Tests/Scripts/bot.json -Seeds 1..10 -FixedStep 60 -Quiet
#        the same script once per seed (-jamseed), as fast as the machine runs it, and a table of each run's
#        "report" line (the script's summary event, e.g. the game over) with the averages
#
# Options: -Seeds, -FixedStep <fps> (-jamfixedstep: every frame is 1/fps of game time, uncapped), -Quiet (no step
# tables), -ResX/-ResY window size (default 1280x720), -Extra more game arguments, -EngineDir, -TimeoutMinutes.
# Every run's results are also kept as Saved/Script/<name>_runs/<seed>.jsonl when -Seeds is given.
# Exit code: the number of runs that failed.
param(
  [Parameter(Mandatory, Position = 0)] [string[]]$Script,
  [string[]]$Seeds = @(),
  [double]$FixedStep = 0,
  [switch]$Quiet,
  [int]$ResX = 1280,
  [int]$ResY = 720,
  [string[]]$Extra = @(),
  [string]$EngineDir = $env:UE_ENGINE_DIR,
  [int]$TimeoutMinutes = 10
)
$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$uproject = Get-ChildItem -Path $root -Filter *.uproject | Select-Object -First 1
if (-not $uproject) { throw "No .uproject in $root" }
if (-not $EngineDir) {
  $installed = Join-Path $env:ProgramData "Epic/UnrealEngineLauncher/LauncherInstalled.dat"
  if (Test-Path $installed) {
    $EngineDir = ((Get-Content $installed -Raw | ConvertFrom-Json).InstallationList | Where-Object AppName -eq "UE_5.8").InstallLocation
  }
}
$exe = if ($EngineDir) { Join-Path $EngineDir "Engine/Binaries/Win64/UnrealEditor.exe" } else { $null }
if (-not ($exe -and (Test-Path $exe))) { throw "Can't find UE 5.8. Pass -EngineDir or set UE_ENGINE_DIR." }

# The game saves its graphics settings when it quits (a -SteamDeck run would leave Low quality behind for every
# later run and the editor's play sessions), so a test run puts the user settings back as they were.
$userSettings = Join-Path $root "Saved/Config/WindowsEditor/GameUserSettings.ini"
$savedSettings = if (Test-Path $userSettings) { Get-Content $userSettings -Raw } else { $null }
function Restore-UserSettings {
  if ($null -ne $savedSettings) { [IO.File]::WriteAllText($userSettings, $savedSettings) }
  elseif (Test-Path $userSettings) { Remove-Item $userSettings }
}

$files = foreach ($each in (($Script -join ",") -split "," | Where-Object { $_ })) {
  $path = if ([IO.Path]::IsPathRooted($each)) { $each } else { Join-Path $root $each }
  if (Test-Path $path -PathType Container) { Get-ChildItem $path -Filter *.json | ForEach-Object FullName } else { (Resolve-Path $path).Path }
}
# "1,2,3" or "1..20" (through powershell -File a list arrives as one string).
$seedList = @(foreach ($part in (($Seeds -join ",") -split "," | ForEach-Object { $_.Trim() } | Where-Object { $_ })) {
  if ($part -match '^(-?\d+)\.\.(-?\d+)$') { [int]$Matches[1]..[int]$Matches[2] } else { [int]$part }
})
# One run without a seed (the script's own or a fresh one) unless seeds were given. Assigned, not returned from an
# if: a pipeline would flatten @($null) to $null and the loop would never run.
$runSeeds = @($null)
if ($seedList.Count) { $runSeeds = $seedList }

$failedRuns = 0
foreach ($file in $files) {
  $name = (Get-Content $file -Raw | ConvertFrom-Json).name
  if (-not $name) { $name = [IO.Path]::GetFileNameWithoutExtension($file) }
  $reports = @()
  foreach ($seed in $runSeeds) {
    $gameArgs = @("`"$($uproject.FullName)`"", "-game", "-windowed", "-ResX=$ResX", "-ResY=$ResY", "-nosplash", "-log=Script_$name.log", "-jamscript=`"$file`"")
    if ($null -ne $seed) { $gameArgs += "-jamseed=$seed" }
    if ($FixedStep -gt 0) { $gameArgs += "-jamfixedstep=$FixedStep" }
    $gameArgs += $Extra
    Write-Host ("Script {0}{1}" -f $name, $(if ($null -ne $seed) { ", seed $seed" } else { "" }))
    $process = Start-Process -FilePath $exe -ArgumentList $gameArgs -PassThru
    $finished = $process.WaitForExit($TimeoutMinutes * 60 * 1000)
    if (-not $finished) { $process.Kill(); $process.WaitForExit() }
    Restore-UserSettings
    if (-not $finished) {
      Write-Warning "$name didn't finish in $TimeoutMinutes minutes (killed). See Saved/Logs/Script_$name.log."
      $failedRuns++
      continue
    }
    $result = Join-Path $root "Saved/Script/$name/result.jsonl"
    if (-not (Test-Path $result)) { Write-Warning "No result for $name. See Saved/Logs/Script_$name.log."; $failedRuns++; continue }
    if ($null -ne $seed) {
      $runs = Join-Path $root "Saved/Script/${name}_runs"
      New-Item -ItemType Directory -Force $runs | Out-Null
      Copy-Item $result (Join-Path $runs "$seed.jsonl") -Force
    }
    $lines = Get-Content $result | ForEach-Object { $_ | ConvertFrom-Json }
    if (-not $Quiet) {
      $lines | Where-Object k -eq "step" | Select-Object step, op, what,
        @{n = "ok"; e = { if ($_.ok) { "ok" } else { "FAIL" } } }, @{n = "t"; e = { [math]::Round($_.t, 1) } }, note |
        Format-Table -AutoSize | Out-String -Width 220
    }
    $done = $lines | Where-Object k -eq "done"
    if (-not $done) { Write-Warning "$name stopped early (no 'done' line). See Saved/Logs/Script_$name.log."; $failedRuns++; continue }
    $verdict = if ($done.failed -gt 0) { "FAIL" } else { "PASS" }
    Write-Host ("{0}: {1}, {2} check(s) passed, {3} failed, {4:N0} s of game in {5:N0} s{6}" -f $name, $verdict, $done.passed, $done.failed,
      $done.gameSeconds, $done.realSeconds, $(if ($done.why) { " ($($done.why))" } else { "" }))
    if ($done.failed -gt 0) { $failedRuns++ }

    # The run's summary: the last trace line of the script's "report" kind, flattened (its "what" and number fields).
    $report = ($lines | Where-Object k -eq "report" | Select-Object -Last 1).line
    $row = [ordered]@{ seed = $done.seed; result = $verdict }
    if ($report) {
      $row["what"] = $report.w
      foreach ($property in $report.PSObject.Properties) {
        if ($property.Name -notin @("t", "lvl", "k", "w") -and $property.Value -is [ValueType]) { $row[$property.Name] = $property.Value }
      }
    }
    $reports += [pscustomobject]$row
  }
  if ($seedList.Count -gt 1 -and $reports.Count) {
    Write-Host "`n$name over $($reports.Count) seed(s):"
    $reports | Format-Table -AutoSize | Out-String -Width 220
    $numeric = $reports[0].PSObject.Properties | Where-Object { $_.Name -ne "seed" -and $_.Value -is [ValueType] -and $_.Value -isnot [bool] } | ForEach-Object Name
    foreach ($field in $numeric) {
      $values = $reports | ForEach-Object { $_.$field } | Where-Object { $null -ne $_ }
      $stats = $values | Measure-Object -Average -Minimum -Maximum
      Write-Host ("  {0}: mean {1:N1}, min {2:N1}, max {3:N1}" -f $field, $stats.Average, $stats.Minimum, $stats.Maximum)
    }
  }
}
exit $failedRuns
