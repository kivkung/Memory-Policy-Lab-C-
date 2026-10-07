$ErrorActionPreference = 'Stop'
$project = Split-Path $PSScriptRoot -Parent
$qa = Join-Path $project 'build\gui-qa'
New-Item -ItemType Directory -Path $qa -Force | Out-Null
$program = Join-Path $project 'memory_policy_lab.exe'
$startInfo = New-Object System.Diagnostics.ProcessStartInfo
$startInfo.FileName = $program
$startInfo.Arguments = '--smoke-test "' + $qa + '"'
$startInfo.WorkingDirectory = $project
$startInfo.UseShellExecute = $false
$startInfo.CreateNoWindow = $true
$startInfo.WindowStyle = [System.Diagnostics.ProcessWindowStyle]::Hidden
$process = [System.Diagnostics.Process]::Start($startInfo)
if (-not $process.WaitForExit(30000)) {
    Stop-Process -Id $process.Id
    throw 'GUI smoke test timed out.'
}
if ($process.ExitCode -ne 0) { throw "GUI smoke test exited with $($process.ExitCode)" }
$report = Get-Content -LiteralPath (Join-Path $qa 'gui-test.log') -Raw
if ($report -notmatch 'PASS:') { throw $report }
Write-Output $report
