# Copyright (C) 2026 NotepadMd contributors
# SPDX-License-Identifier: GPL-3.0-or-later

# Fetch or verify the exact upstream plugin headers used by this project.
$ErrorActionPreference = 'Stop'
$repository = 'https://github.com/npp-plugins/plugintemplate.git'
$revision = '27b7077ba89766b3a5a136a3d71af3a0cccc7d2a'
$checkout = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\.local\plugintemplate'))
$headers = @{
    'PluginInterface.h' = '27b2ce23b78d365fc61efb21ac1e3a64bc551f0e0a736db420ec5db7a230d6bf'
    'Notepad_plus_msgs.h' = '8283285bdac6a232762572fad7b3221ea472fbc6d48b50275dbd31c2dd25495f'
    'Scintilla.h' = '2ee1a1a8190f9e9bb723b011c0486fd3b00960a49d5bd0a72dbaa95723673e59'
    'Sci_Position.h' = '8b72e5c0b677abafc97d90588b05ae9adfc568e6ee119f2e10967703ee230653'
}

if (-not (Test-Path -LiteralPath $checkout)) {
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $checkout) | Out-Null
    & git clone --quiet $repository $checkout
    if ($LASTEXITCODE -ne 0) { throw 'Could not clone the Notepad++ plugin template.' }
    & git -C $checkout checkout --quiet --detach $revision
    if ($LASTEXITCODE -ne 0) { throw 'Could not check out the pinned SDK revision.' }
}

$gitDirectory = Join-Path $checkout '.git'
if (Test-Path -LiteralPath $gitDirectory) {
    $actual = (& git -C $checkout rev-parse HEAD)
    if ($LASTEXITCODE -ne 0 -or $actual -ne $revision) {
        throw ('SDK checkout is not at the pinned revision {0}: {1}' -f $revision, $checkout)
    }
    $changes = (& git -C $checkout status --porcelain --untracked-files=no)
    if ($LASTEXITCODE -ne 0 -or $changes) {
        throw "SDK checkout has local modifications: $checkout"
    }
}
else {
    $revisionFile = Join-Path $checkout 'SDK_REVISION.txt'
    if (-not (Test-Path -LiteralPath $revisionFile) -or (Get-Content -LiteralPath $revisionFile -Raw).Trim() -ne $revision) {
        throw "SDK directory is not a Git checkout and has no valid pinned revision marker: $checkout"
    }
}

$sourceDirectory = Join-Path $checkout 'src'
foreach ($name in $headers.Keys) {
    $path = Join-Path $sourceDirectory $name
    if (-not (Test-Path -LiteralPath $path)) { throw "Missing SDK header: $name" }
    $bytes = [IO.File]::ReadAllBytes($path)
    $normalized = [Collections.Generic.List[byte]]::new()
    for ($index = 0; $index -lt $bytes.Length; $index++) {
        if ($bytes[$index] -eq 13 -and $index + 1 -lt $bytes.Length -and $bytes[$index + 1] -eq 10) {
            $normalized.Add(10)
            $index++
        }
        elseif ($bytes[$index] -eq 13) {
            $normalized.Add(10)
        }
        else {
            $normalized.Add($bytes[$index])
        }
    }
    $sha = [Security.Cryptography.SHA256]::Create()
    $actualHash = [BitConverter]::ToString($sha.ComputeHash($normalized.ToArray())).Replace('-', '').ToLowerInvariant()
    $sha.Dispose()
    if ($actualHash -ne $headers[$name]) {
        throw "SDK header does not match pinned revision ${revision}: $name"
    }
}
Write-Output "Notepad++ plugin SDK ready at $checkout ($revision; header SHA-256 verified)"
