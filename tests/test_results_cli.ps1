$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$buildRoot = [IO.Path]::GetFullPath((Join-Path $projectRoot 'build'))
$testRoot = Join-Path $buildRoot ('results-cli-' + [Guid]::NewGuid().ToString('N'))
$appBuild = Join-Path $testRoot 'build'
$resultFolder = Join-Path $testRoot 'results'
$otherCwd = Join-Path $testRoot 'other-cwd'
$previousLocation = Get-Location

function Invoke-ResultLab([string[]] $Lines, [string] $ReferenceFile = '') {
    $exe = Join-Path $appBuild 'memory_policy_lab.exe'
    if ($ReferenceFile) { $text = ($Lines | & $exe $ReferenceFile) -join "`n" }
    else { $text = ($Lines | & $exe) -join "`n" }
    if ($LASTEXITCODE -ne 0) { throw 'Result-browser app failed.' }
    return $text
}

function Require-ResultMatch([string] $Text, [string] $Pattern, [string] $Message) {
    if ($Text -notmatch $Pattern) { throw $Message }
}

try {
    New-Item -ItemType Directory -Path $appBuild -Force | Out-Null
    New-Item -ItemType Directory -Path $otherCwd -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $projectRoot 'build\memory_policy_lab.exe') -Destination $appBuild
    Set-Location -LiteralPath $otherCwd

    # Direct executable launch from another cwd still uses the project's results folder.
    $empty = Invoke-ResultLab @('1','0','0')
    if (!(Test-Path -LiteralPath $resultFolder)) { throw 'Results directory was not created.' }
    if (Test-Path -LiteralPath (Join-Path $otherCwd 'results')) { throw 'Results were created in the wrong cwd.' }
    Require-ResultMatch $empty 'No usable result files yet' 'Empty list is unclear.'

    $saved = Invoke-ResultLab @('9','','','','','','11','zulu','11','alpha.TXT','11','','11','','0')
    foreach ($name in @('zulu.txt','alpha.TXT','result_001.txt','result_002.txt')) {
        if (!(Test-Path -LiteralPath (Join-Path $resultFolder $name))) { throw ('Missing saved result: ' + $name) }
    }
    Require-ResultMatch $saved 'Saved 5 references' 'Search result was not saved.'
    if ((Get-Content -Raw -LiteralPath (Join-Path $resultFolder 'zulu.txt')).Trim() -ne '0 1 0 2 0') { throw 'Saved contents changed.' }

    Set-Content -LiteralPath (Join-Path $resultFolder 'invalid.txt') -Value 'bad input' -Encoding Ascii
    Set-Content -LiteralPath (Join-Path $resultFolder 'ignored.md') -Value '1 2 3' -Encoding Ascii
    New-Item -ItemType Directory -Path (Join-Path $resultFolder 'folder.txt') | Out-Null
    $listed = Invoke-ResultLab @('1','1','7','0')
    Require-ResultMatch $listed '(?m)^\s+1\s+alpha.TXT\s+5 references' 'List is not sorted or cannot select files.'
    Require-ResultMatch $listed 'Skipped 1 .txt files with invalid input' 'Invalid txt filtering failed.'
    Require-ResultMatch $listed 'Loaded 5 references' 'Numeric file selection failed.'
    if ($listed -match 'ignored.md|folder.txt') { throw 'List included non-input files or directories.' }

    $named = Invoke-ResultLab @('1','zulu','7','0')
    Require-ResultMatch $named 'Loaded 5 references' 'Bare filename loading failed.'
    $absolute = '"' + (Join-Path $projectRoot 'examples\classic.txt') + '"'
    $full = Invoke-ResultLab @('1',$absolute,'7','0')
    Require-ResultMatch $full 'Loaded 13 references' 'Full path loading failed.'
    $outside = Join-Path $testRoot 'outside saved.txt'
    $fullSave = Invoke-ResultLab @('11',('"' + $outside + '"'),'0') (Join-Path $resultFolder 'zulu.txt')
    if (!(Test-Path -LiteralPath $outside)) { throw 'Full path saving failed.' }
    Require-ResultMatch $fullSave 'Saved 5 references' 'Full path save was not reported.'

    $badSelection = Invoke-ResultLab @('1','999','7','0') (Join-Path $resultFolder 'zulu.txt')
    Require-ResultMatch $badSelection 'No file with this number' 'Invalid index is unclear.'
    Require-ResultMatch $badSelection 'References \(5\)' 'Invalid selection replaced previous input.'
    $cliName = Invoke-ResultLab @('7','0') 'zulu'
    Require-ResultMatch $cliName 'References \(5\)' 'Command-line bare name did not resolve from results.'

    Write-Output 'Results CLI tests passed: creation, names, listing/index, full paths, empty/invalid files, alternate cwd, unique defaults.'
} finally {
    Set-Location -LiteralPath $previousLocation.Path
    # This unique fixture tree is the only recursive cleanup target; verify its boundary.
    $resolvedTestRoot = [IO.Path]::GetFullPath($testRoot)
    if (!$resolvedTestRoot.StartsWith($buildRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Refusing to clean a fixture directory outside build.'
    }
    if (Test-Path -LiteralPath $resolvedTestRoot) { Remove-Item -LiteralPath $resolvedTestRoot -Recurse -Force }
}
