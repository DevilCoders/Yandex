# ToDo:
#      * make Setupcomplete declarative like our tests on fabric
#      * add logging

$SetupCompleteSteps = ls "C:\Windows\Setup\Scripts" -Filter "ensure-*.ps1" | `
  Select-Object -ExpandProperty name | `
    % { $_ -replace ".ps1", "" -replace "ensure-", ""} | `
      % { & $PSScriptRoot\invoke-buildstep.ps1 -StepAction Ensure -StepSetting $_ }
