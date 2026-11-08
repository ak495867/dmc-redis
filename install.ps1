param (
    [string]$InstallDir = "$HOME\.dmc-redis\bin",
    [string]$Version = "latest"
)

$ErrorActionPreference = "Stop"

$Repo = "ak495867/dmc-redis"

if ($Version -eq "latest") {
    $Tag = "v1.0.0"
} else {
    $Tag = $Version
}

$AssetUrl = "https://github.com/$Repo/releases/download/$Tag/dmc-redis-windows-x86_64.zip"

if (-not (Test-Path $InstallDir)) {
    New-Item -ItemType Directory -Path $InstallDir -Force | Out-Null
}

$ZipPath = Join-Path $env:TEMP "dmc-redis.zip"
$ExtractDir = Join-Path $env:TEMP "dmc-redis-extracted"

Invoke-WebRequest -Uri $AssetUrl -OutFile $ZipPath -UseBasicParsing

if (Test-Path $ExtractDir) {
    Remove-Item -Recurse -Force $ExtractDir
}

Expand-Archive -Path $ZipPath -DestinationPath $ExtractDir -Force

$ExeSource = Get-ChildItem -Path $ExtractDir -Filter "dmc-redis.exe" -Recurse | Select-Object -First 1

if (-not $ExeSource) {
    throw "Executable dmc-redis.exe not found in archive"
}

Copy-Item -Path $ExeSource.FullName -Destination (Join-Path $InstallDir "dmc-redis.exe") -Force

Remove-Item -Force $ZipPath -ErrorAction SilentlyContinue
Remove-Item -Recurse -Force $ExtractDir -ErrorAction SilentlyContinue

$UserPath = [Environment]::GetEnvironmentVariable("PATH", [EnvironmentVariableTarget]::User)
if ($UserPath -notlike "*$InstallDir*") {
    [Environment]::SetEnvironmentVariable("PATH", "$InstallDir;$UserPath", [EnvironmentVariableTarget]::User)
}

$env:PATH = "$InstallDir;$env:PATH"

Write-Output "dmc-redis $Tag successfully installed to $InstallDir"
