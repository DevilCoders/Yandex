# ToDo: 
#       * after implementation of passing env's via fabric we can
#         download latest windows-scripts and work with'em like in bootstrap
#       * detect tests, simply by looking up for *.tests.json file
#         add expected result and compare with it
#       * isolate test via jobs

# imports, mandatory and test items from *.tests.json
. "$PSScriptRoot\windows-scripts\common\yandex-powershell-logging.ps1"
ls $PSScriptRoot\*.tests.json | select -exp fullname | % { [array]$tests += (Get-Content $_ | ConvertFrom-Json).items }

# discover modules
$isCompliant = $true
foreach ($test in $tests) {
  "[CHECK]::Processing $($test.name)" | Out-InfoMessage

  $r = & $PSScriptRoot\windows-scripts\invoke-buildstep.ps1 -StepAction Test -StepSetting $($test.name)

  $isCompliant = $r -and $isCompliant
  "[CHECK]::Succeeded? $r" | Out-InfoMessage
}

"[TEST]::Fully compliant? $isCompliant" | Out-InfoMessage
if ($isCompliant) { exit 0 }
else { exit 1 }
