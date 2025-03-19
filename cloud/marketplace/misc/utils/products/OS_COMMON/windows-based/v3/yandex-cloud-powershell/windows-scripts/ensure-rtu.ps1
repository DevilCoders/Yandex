# include
. "$PSScriptRoot\common\yandex-powershell-logging.ps1"


function Ensure-RTU { 
  "[ENFORCE]::Ensure RDP" | Out-InfoMessage

  Set-ItemProperty `
    -Path 'HKLM:\SYSTEM\CurrentControlSet\Control\TimeZoneInformation' `
    -Name "RealTimeIsUniversal" `
    -Value 1 `
    -Type DWord `
    -Force
  
  return (Test-RTU)
}


function Test-RTU {
  $r = Get-ItemProperty `
    -Path 'HKLM:\SYSTEM\CurrentControlSet\Control\TimeZoneInformation' `
    -Name "RealTimeIsUniversal" -ea:SilentlyContinue | `
      Select-Object -ExpandProperty RealTimeIsUniversal
  "[TEST]::RealTimeIsUniversal is $r" | Out-InfoMessage

  return ($r -eq 1)
}
