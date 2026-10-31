# Copies the packaged Win64 build to the Steam Deck, where Steam runs it under Proton.
#   powershell -File tools/deploy-deck.ps1 [-Package] [-AddToSteam]
# SSH goes through WSL's `steamdeck` alias (same one srpg uses): its ProxyCommand finds the Deck's current
# DHCP address and only connects if the host key matches the pinned one. If it says "Deck not found", the
# Deck is asleep, off or on another network.
#   -Package     package first (Win64 Development into Packaged/); otherwise deploy the existing package
#   -AddToSteam  first deploy only: create the library entry (Steam must be running on the Deck).
#                Then, on the Deck once: game Properties > Compatibility > force a Proton version.
# Quit the game on the Deck first. The Deck-side Saved/ folder (logs, settings) is kept between deploys.
param(
  [string]$DeckHost = "steamdeck",
  [string]$RemoteDir = "Games/bullette",
  [switch]$Package,
  [switch]$AddToSteam
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$uproject = Join-Path $root "Bullette.uproject"
$build = Join-Path $root "Packaged/Windows"
$archive = Join-Path $root "Packaged/bullette-win64.tar.gz"
$exe = "Bullette.exe"
$pak = "Bullette/Content/Paks/Bullette-Windows.pak"
$uat = "C:\Games\Epic Games\Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat"

function Deck([string]$command) {
  wsl.exe -e ssh -o BatchMode=yes -o ConnectTimeout=10 $DeckHost $command
  if ($LASTEXITCODE -ne 0) { throw "On the Deck, failed: $command" }
}

if ($Package) {
  # Close the editor first: BuildCookRun rebuilds the editor binaries the cook uses. Cooking with editor
  # binaries older than a Live Coding session silently drops properties that only exist in the patch.
  # -applocaldirectory: ship the VC++ runtime DLLs next to the exes. Proton's per-game prefix has no VC++ redist,
  # so without them the launcher stub stops with "missing Microsoft Visual C++ 2015-2022 Redistributable".
  # Commandlets never auto-start the MCP server, so the cook needs no MCP override (one wiped the editor's auto-start setting).
  if (Get-Process UnrealEditor -ErrorAction SilentlyContinue) { throw "Close the Unreal Editor before -Package." }
  & $uat BuildCookRun -project="$uproject" -noP4 -platform=Win64 -clientconfig=Development `
    -build -cook -stage -pak -archive -archivedirectory="$(Join-Path $root 'Packaged')" `
    -applocaldirectory='$(EngineDir)/Binaries/ThirdParty/AppLocalDependencies' `
    -utf8output
  if ($LASTEXITCODE -ne 0) { throw "Packaging failed (RunUAT exit $LASTEXITCODE)." }
}

$pakPath = Join-Path $build $pak
if (-not (Test-Path (Join-Path $build $exe)) -or -not (Test-Path $pakPath)) { throw "No packaged build in $build. Run with -Package." }

# Refuse to ship a package older than the code or content.
$newestSource = Get-ChildItem -Path (Join-Path $root "Source"), (Join-Path $root "Content"), (Join-Path $root "Config") -Recurse -File -ErrorAction SilentlyContinue |
  Sort-Object LastWriteTime -Descending | Select-Object -First 1
$pakWritten = (Get-Item $pakPath).LastWriteTime
if ($newestSource -and $newestSource.LastWriteTime -gt $pakWritten) {
  throw "Stale package: the pak was written $pakWritten but $($newestSource.FullName) changed $($newestSource.LastWriteTime). Re-run with -Package."
}

# One archive: copying thousands of files one by one over Wi-Fi is slow and unreliable.
if (Test-Path $archive) { Remove-Item $archive }
tar.exe -czf $archive -C $build .
if ($LASTEXITCODE -ne 0) { throw "Packing $archive failed." }

# [B] keeps pgrep from matching the shell running this command.
$running = wsl.exe -e ssh -o BatchMode=yes -o ConnectTimeout=10 $DeckHost "pgrep -f '[B]ullette.exe' >/dev/null && echo running || true"
if ($LASTEXITCODE -ne 0) { throw "Can't reach the Deck through the WSL alias '$DeckHost'." }
if ($running -eq "running") { throw "The game is running on the Deck; quit it first." }

$wslArchive = (wsl.exe -e wslpath -a $archive.Replace('\', '/')).Trim()
wsl.exe -e bash -c "ssh -o BatchMode=yes $DeckHost 'cat > /tmp/bullette-win64.tar.gz' < '$wslArchive'"
if ($LASTEXITCODE -ne 0) { throw "Copy to the Deck failed." }

# Replace everything except Bullette/Saved, which holds the Deck's logs and settings.
Deck "set -e; mkdir -p ~/$RemoteDir; cd ~/$RemoteDir; find . -mindepth 1 -maxdepth 1 ! -name Bullette -exec rm -rf {} +; mkdir -p Bullette; find Bullette -mindepth 1 -maxdepth 1 ! -name Saved -exec rm -rf {} +; tar -xzf /tmp/bullette-win64.tar.gz; rm /tmp/bullette-win64.tar.gz; md5sum $pak"
Write-Host "Local:"
"$((Get-FileHash -Algorithm MD5 $pakPath).Hash.ToLower())  $pak"

if ($AddToSteam) {
  # SteamOS's own helper hands the shortcut to the running Steam client (never write shortcuts.vdf).
  Deck "steamos-add-to-steam ~/$RemoteDir/$exe"
}

Write-Host "Deployed to ${DeckHost}:~/$RemoteDir. Log on the Deck: ~/$RemoteDir/Bullette/Saved/Logs/Bullette.log"
