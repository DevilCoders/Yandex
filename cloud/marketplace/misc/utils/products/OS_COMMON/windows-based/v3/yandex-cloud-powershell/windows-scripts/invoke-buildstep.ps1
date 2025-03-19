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
  #"[BUILDSTEP]::Found *$StepSetting* under $PSScriptRoot" | Out-InfoMessage
  ls "$PSScriptRoot\*$StepSetting*" | % { . $_.FullName }
} else {
  throw "lib *$StepSetting* not found at $PSScriptRoot"
}

# to be sure
$ErrorActionPreference = 'Stop'

# rewrite this sh*t code
"[BUILDSTEP]::About to invoke | iex Test-$StepSetting $StepArgs" | Out-InfoMessage
$ConfigurationIsNotDrifted = (iex "Test-$StepSetting")
if ("Ensure" -eq $StepAction) {
  if ( -not $ConfigurationIsNotDrifted ) {
    "[BUILDSTEP]::Eliminating configuration drift" | Out-InfoMessage
    
    # every ensure return's its test-* func
    $ConfigurationIsNotDrifted = (iex "Ensure-$StepSetting $StepArgs")
  } else {
    "[BUILDSTEP]::No configuration drift detected" | Out-InfoMessage
    
    $ConfigurationIsNotDrifted = $true
  }
}

# "Test" -eq $StepAction
#"[BUILDSTEP]::Tested $StepSetting" | Out-InfoMessage

if (-not $ConfigurationIsNotDrifted) { exit 1 }
else { exit 0 }
#else { return $true } # for win_check.ps1
#return $ConfigurationIsNotDrifted
