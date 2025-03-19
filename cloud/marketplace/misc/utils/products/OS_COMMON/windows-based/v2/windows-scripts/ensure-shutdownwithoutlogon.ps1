#include
. "$PSScriptRoot\common\yandex-powershell-logging.ps1"


function Ensure-Shutdownwithoutlogon {
  "[ENFORCE]::Ensuring shutdownwithoutlogon" | Out-InfoMessage 

  Set-ItemProperty `
    -Path "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\System" `
    -Name "ShutdownWithoutLogon" `
    -Value 1

  return (Test-Shutdownwithoutlogon)
}


function Test-Shutdownwithoutlogon {
  $r = (Get-ItemProperty `
    -Path "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\System" `
    -Name "ShutdownWithoutLogon" `
    -ErrorAction 'SilentlyContinue').Shutdownwithoutlogon  
  "[TEST]::Shutdownwithoutlogon is $r" | Out-InfoMessage

  return ($r -eq 1)
}
