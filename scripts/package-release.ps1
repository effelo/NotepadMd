# Copyright (C) 2026 NotepadMd contributors
# SPDX-License-Identifier: GPL-3.0-or-later

$ErrorActionPreference = 'Stop'
$repositoryRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$versionHeader = Get-Content (Join-Path $repositoryRoot 'src\Version.h') -Raw
if ($versionHeader -notmatch '(?m)^#define NOTEPADMD_VERSION_STRING "(?<version>\d+\.\d+\.\d+)"\s*$') {
    throw 'Could not read NOTEPADMD_VERSION_STRING from src/Version.h.'
}
$version = $Matches.version
$buildScript = Join-Path $PSScriptRoot 'build.ps1'
$prepareScript = Join-Path $PSScriptRoot 'prepare-sdk.ps1'
& $prepareScript
$sdkCheckout = Join-Path $repositoryRoot '.local\plugintemplate'
$sdkSource = Join-Path $sdkCheckout 'src'
$headers = @('PluginInterface.h', 'Notepad_plus_msgs.h', 'Scintilla.h', 'Sci_Position.h')

foreach ($platform in @('Win32', 'x64')) {
    & $buildScript -Platform $platform
}

$buildRoot = Join-Path $repositoryRoot 'build'
$stageRoot = Join-Path $buildRoot ('.release-staging-' + [guid]::NewGuid().ToString('N'))
try {
    $sourceStage = Join-Path $stageRoot 'source\NotepadMd'
    New-Item -ItemType Directory -Force -Path $sourceStage | Out-Null
    foreach ($name in @('src', 'tests', 'scripts')) {
        Copy-Item -LiteralPath (Join-Path $repositoryRoot $name) -Destination $sourceStage -Recurse
    }
    foreach ($name in @('NotepadMd.vcxproj', 'README.md', 'LICENSE', 'THIRD_PARTY_NOTICES.md', '.gitattributes')) {
        Copy-Item -LiteralPath (Join-Path $repositoryRoot $name) -Destination $sourceStage
    }
    $sdkStage = Join-Path $sourceStage '.local\plugintemplate'
    $sdkStageSource = Join-Path $sdkStage 'src'
    New-Item -ItemType Directory -Force -Path $sdkStageSource | Out-Null
    foreach ($header in $headers) { Copy-Item -LiteralPath (Join-Path $sdkSource $header) -Destination $sdkStageSource }
    $revisionMarker = Join-Path $sdkCheckout 'SDK_REVISION.txt'
    if (Test-Path -LiteralPath $revisionMarker) {
        $revision = (Get-Content -LiteralPath $revisionMarker -Raw).Trim()
    }
    else {
        $revision = (& git -C $sdkCheckout rev-parse HEAD).Trim()
        if ($LASTEXITCODE -ne 0) { throw 'Could not read the pinned SDK revision for the source archive.' }
    }
    Set-Content -LiteralPath (Join-Path $sdkStage 'SDK_REVISION.txt') -Value $revision -NoNewline

    $sourceArchive = Join-Path $buildRoot "NotepadMd-$version-source.zip"
    if (Test-Path -LiteralPath $sourceArchive) { Remove-Item -LiteralPath $sourceArchive -Force }
    Compress-Archive -Path (Join-Path $stageRoot 'source\*') -DestinationPath $sourceArchive -CompressionLevel Optimal

    foreach ($platform in @('Win32', 'x64')) {
        $architecture = $platform.ToLowerInvariant()
        $releaseStage = Join-Path $stageRoot "$architecture\NotepadMd"
        New-Item -ItemType Directory -Force -Path $releaseStage | Out-Null
        Copy-Item -LiteralPath (Join-Path $buildRoot "$platform\Release\NotepadMd.dll") -Destination $releaseStage
        foreach ($name in @('LICENSE', 'THIRD_PARTY_NOTICES.md')) {
            Copy-Item -LiteralPath (Join-Path $repositoryRoot $name) -Destination $releaseStage
        }
        $archive = Join-Path $buildRoot "NotepadMd-$version-$architecture.zip"
        if (Test-Path -LiteralPath $archive) { Remove-Item -LiteralPath $archive -Force }
        Compress-Archive -Path (Join-Path $stageRoot "$architecture\*") -DestinationPath $archive -CompressionLevel Optimal
    }
}
finally {
    if (Test-Path -LiteralPath $stageRoot) { Remove-Item -LiteralPath $stageRoot -Recurse -Force }
}

Write-Output "Created release archives in $buildRoot"
