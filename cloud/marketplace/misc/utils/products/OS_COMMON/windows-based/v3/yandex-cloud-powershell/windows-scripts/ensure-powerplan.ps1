. "$PSScriptRoot\common\yandex-powershell-logging.ps1"
$__HighPerformanceID = "8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c"


function Ensure-Powerplan {
  "[ENFORCE]::Ensuring powerplan" | Out-InfoMessage  
  
  & powercfg -setactive $__HighPerformanceID

  return (Test-Powerplan)
}


function Test-Powerplan {  
  $r = (& powercfg -getactivescheme)
  "[TEST]::Active power scheme is $($r.Split('(')[-1].Replace(')', ''))" | Out-InfoMessage

  return ($r -match $__HighPerformanceID)
}
