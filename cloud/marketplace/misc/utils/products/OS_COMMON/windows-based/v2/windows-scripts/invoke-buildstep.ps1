# ToDo:
#      * kwargs?
#      * force flag?
#      * exit { 0 | 1 } here?

param(
  [Parameter(Mandatory=$true)]
  [ValidateSet("Ensure", "Test")]
  [String]$StepAction,
  
  [Parameter(Mandatory=$true)]
  [ValidateNotNullOrEmpty()]
  [String]$StepSetting,

  [String]$StepArgs
)

# include
. "$PSScriptRoot\common\yandex-powershell-logging.ps1"
if (Test-Path "$PSScriptRoot\*$StepSetting*") {
  "[BUILDSTEP]::Found *$StepSetting* under $PSScriptRoot" | Out-InfoMessage
  ls "$PSScriptRoot\*$StepSetting*" | % { . $_.FullName }
} else {
  throw "lib *$StepSetting* not found at $PSScriptRoot"
}

# to be sure
$ErrorActionPreference = 'Stop'

# run
"[BUILDSTEP]::About to invoke | iex Test-$StepSetting $StepArgs" | Out-InfoMessage
$isConfigurationDrifted = (iex "Test-$StepSetting")
if ("Ensure" -eq $StepAction) {
  if ( -not $isConfigurationDrifted ) {
    "[BUILDSTEP]::Eliminating configuration drift" | Out-InfoMessage
    
    # every ensure return's its test-* func
    return (iex "Ensure-$StepSetting $StepArgs")
  } else {
    "[BUILDSTEP]::No configuration drift detected" | Out-InfoMessage
    
    return $true
  }
}

# "Test" -eq $StepAction
"[BUILDSTEP]::Tested $StepSetting" | Out-InfoMessage

return $isConfigurationDrifted
