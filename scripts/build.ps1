# Copyright (C) 2026 NotepadMd contributors
# SPDX-License-Identifier: GPL-3.0-or-later

param(
    [ValidateSet('Win32', 'x64')]
    [string]$Platform = 'x64',
    [switch]$Test
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$programRoots = @(${env:ProgramFiles(x86)}, $env:ProgramFiles) |
    Where-Object { $_ } |
    Select-Object -Unique
$vswhere = $null

foreach ($root in $programRoots) {
    $candidate = Join-Path $root 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path -LiteralPath $candidate) {
        $vswhere = $candidate
        break
    }
}

$installation = $null
$hasWindowsSdk = $false
if ($vswhere) {
    $vswhereArguments = @(
        '-products', '*',
        '-version', '[17.0,18.0)',
        '-requires',
        'Microsoft.Component.MSBuild',
        'Microsoft.VisualStudio.Component.VC.Tools.x86.x64',
        '-format', 'json'
    )
    $json = & $vswhere @vswhereArguments

    if ($LASTEXITCODE -eq 0 -and $json) {
        $candidates = $json |
            ConvertFrom-Json |
            Sort-Object installationVersion -Descending

        foreach ($candidate in $candidates) {
            $root = $candidate.installationPath
            $msbuildCandidate = Join-Path $root 'MSBuild\Current\Bin\MSBuild.exe'
            $toolsetRoot = Join-Path $root 'VC\Tools\MSVC'
            $toolset = Get-ChildItem -LiteralPath $toolsetRoot -Directory -ErrorAction SilentlyContinue |
                Where-Object {
                    try {
                        $toolsetVersion = [version]$_.Name
                        $hasV143 = $toolsetVersion.Major -eq 14 -and $toolsetVersion.Minor -ge 30
                        $hasX64Compiler = Test-Path (Join-Path $_.FullName 'bin\Hostx64\x64\cl.exe')
                        $hasX86Compiler = Test-Path (Join-Path $_.FullName 'bin\Hostx64\x86\cl.exe')
                        $hasV143 -and $hasX64Compiler -and $hasX86Compiler
                    }
                    catch {
                        $false
                    }
                } |
                Sort-Object Name -Descending |
                Select-Object -First 1

            $sdkRoots = @()
            if ($env:WindowsSdkDir) {
                $sdkRoots += $env:WindowsSdkDir
            }
            try {
                $sdkRoots += (Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\Windows Kits\Installed Roots').KitsRoot10
            }
            catch { }

            foreach ($sdkRoot in ($sdkRoots | Where-Object { $_ } | Select-Object -Unique)) {
                $includeRoot = Join-Path $sdkRoot 'Include'
                $sdk = Get-ChildItem -LiteralPath $includeRoot -Directory -ErrorAction SilentlyContinue |
                    Where-Object { Test-Path (Join-Path $_.FullName 'um\Windows.h') } |
                    Sort-Object Name -Descending |
                    Select-Object -First 1
                $hasRequiredLibraries = (
                    $sdk -and
                    (Test-Path (Join-Path $sdkRoot "Lib\$($sdk.Name)\um\x64\kernel32.lib")) -and
                    (Test-Path (Join-Path $sdkRoot "Lib\$($sdk.Name)\um\x86\kernel32.lib"))
                )

                if ($hasRequiredLibraries) {
                    $hasWindowsSdk = $true
                    break
                }
            }

            $hasRequiredTools = (
                (Test-Path -LiteralPath $msbuildCandidate) -and
                $toolset -and
                $hasWindowsSdk
            )
            if ($hasRequiredTools) {
                $installation = $root
                $msbuild = $msbuildCandidate
                break
            }
        }
    }
}

if (-not $installation) {
    $message = (
        'A compatible Visual Studio 2022 installation with MSBuild, the v143 C++ toolset, ' +
        'and the Windows SDK for x86 and x64 is required. Install the C++ desktop ' +
        'development tools and Windows SDK.'
    )
    throw $message
}

if ($Test) {
    $project = Join-Path $repositoryRoot 'tests\MarkdownHeadingScannerTests.vcxproj'
}
else {
    $project = Join-Path $repositoryRoot 'NotepadMd.vcxproj'
}

$buildArguments = @(
    $project,
    '/p:Configuration=Release',
    "/p:Platform=$Platform",
    '/v:minimal'
)
& $msbuild @buildArguments
if ($LASTEXITCODE -ne 0) {
    throw "Build failed: $project ($Platform)"
}

if ($Test) {
    $executable = Join-Path $repositoryRoot "build\$Platform\Release\MarkdownHeadingScannerTests.exe"
    & $executable
    if ($LASTEXITCODE -ne 0) {
        throw 'Scanner tests failed.'
    }
}
