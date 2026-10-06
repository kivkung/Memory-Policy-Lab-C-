$ErrorActionPreference = 'Stop'
Set-Location (Split-Path $PSScriptRoot -Parent)

& .\build.bat
if ($LASTEXITCODE -ne 0) { throw 'Simulator build failed.' }

function Invoke-Lab([string[]] $InputLines, [string] $ReferenceFile = '') {
    if ($ReferenceFile) {
        $text = ($InputLines | & .\build\memory_policy_lab.exe $ReferenceFile) -join "`n"
    } else {
        $text = ($InputLines | & .\build\memory_policy_lab.exe) -join "`n"
    }
    if ($LASTEXITCODE -ne 0) { throw 'Simulator exited with an error.' }
    return $text
}

function Require-Match([string] $Text, [string] $Pattern, [string] $Message) {
    if ($Text -notmatch $Pattern) { throw $Message }
}

$trace = Invoke-Lab @('2','2','5','10','2','0') 'examples\lru_wins.txt'
Require-Match $trace 'FIFO after access' 'Missing frame column.'
Require-Match $trace '(?m)^4\s+3\s+\[3, 2\]\s+Fault' 'Incorrect single or paired FIFO row.'
Require-Match $trace '(?m)^5\s+1\s+\[3, 1\]\s+Fault\s+\[1, 3\]\s+Hit' 'Incorrect paired result.'
Require-Match $trace 'First different replacement decision: step 4' 'Missing first decision.'
Require-Match $trace 'First different Hit/Fault result: step 5' 'Missing first outcome.'
if ($trace.Contains('|') -or $trace -match '(?m)^\s*[-=]{3,}') { throw 'Table contains separator lines.' }
Require-Match $trace '(?m)^1\s+1\s+\[1, -\]\s+Fault\s+None\r?\n\r?\n' 'Missing blank spacing between rows.'

$search = Invoke-Lab @('9','2','3','5','2','100000','11','build\cli-found.tmp','10','2','0')
Require-Match $search 'Tested: 34 / 243' 'Incorrect search count.'
Require-Match $search 'FIFO: 4 faults\r?\nLRU:  3 faults' 'Incorrect search result.'
Require-Match $search 'Saved 5 references' 'Save failed.'
Require-Match $search 'Frames: 2\s+Algorithm: FIFO\s+References: 5' 'Found input was not activated.'
$reload = Invoke-Lab @('2','2','6','0') 'build\cli-found.tmp'
Require-Match $reload '(?m)^FIFO\s+1\s+4\s+2\s+80.00%' 'Saved FIFO result changed.'
Require-Match $reload '(?m)^LRU\s+2\s+3\s+1\s+60.00%' 'Saved LRU result changed.'
Remove-Item -LiteralPath 'build\cli-found.tmp'

$fifoWins = Invoke-Lab @('9','1','4','7','3','100000','0')
Require-Match $fifoWins 'Match found' 'FIFO-wins search did not find a match.'

$exhausted = Invoke-Lab @('9','1','3','5','1','243','7','0') 'examples\classic.txt'
Require-Match $exhausted 'No match in the complete configured search space' 'Exhausted status missing.'
Require-Match $exhausted 'References \(13\)' 'Failed search replaced the original input.'
Require-Match $exhausted 'Frames: 3\s+Algorithm: FIFO\s+References: 13' 'Failed search changed frames.'

$limited = Invoke-Lab @('9','2','3','5','2','1','0')
Require-Match $limited 'Search limit reached' 'Limit status missing.'
Require-Match $limited 'Tested: 1 / 243' 'Limit was exceeded.'

$defaults = Invoke-Lab @('9','','','','','','0')
Require-Match $defaults 'Tested: 34 / 243' 'Search defaults failed.'

$stopped = Invoke-Lab @('2','2','10','1','q','0') 'examples\lru_wins.txt'
Require-Match $stopped 'Stopped: 1/5 steps shown' 'Paired stop failed.'
if ($stopped -match 'First different replacement decision: step 4') { throw 'Partial analysis included unseen steps.' }

$step = Invoke-Lab @('4','q','4','a','0') 'examples\classic.txt'
Require-Match $step 'References:\s+1 / 13' 'Single stop counts failed.'
Require-Match $step 'References:\s+13 / 13' 'Restart/run remaining failed.'

$sweep = Invoke-Lab @('8','0') 'examples\belady.txt'
Require-Match $sweep "FIFO Belady's anomaly: 9 -> 10" 'Belady sweep failed.'

# Full-width IDs must not shift the following column into the frame text.
Set-Content -LiteralPath 'build\wide.tmp' -Value '2147483647 0 2147483647' -Encoding Ascii
$wide = Invoke-Lab @('2','10','5','10','2','0') 'build\wide.tmp'
Require-Match $wide '\[2147483647, 0, -, -, -, -, -, -, -, -\]\s+Hit' 'Wide frame formatting failed.'
Remove-Item -LiteralPath 'build\wide.tmp'

# Quoted path and invalid settings still work after menu changes.
$quotedPath = '"' + (Join-Path (Get-Location).Path 'examples\classic.txt') + '"'
$invalid = Invoke-Lab @('oops','2','0','1',$quotedPath,'6','0')
Require-Match $invalid 'Please enter a menu number' 'Menu validation failed.'
Require-Match $invalid 'Invalid value' 'Frame validation failed.'
Require-Match $invalid '53.85%' 'Quoted loading/Optimal comparison failed.'

Write-Output 'CLI tests passed: clean tables, paired analysis, search, save/reload, defaults, limits, stop/restart, wide IDs.'
