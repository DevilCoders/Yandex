# include
. "$PSScriptRoot\common\yandex-powershell-logging.ps1"


function Ensure-RDP {
  "[ENFORCE]::Ensuring RDP" | Out-InfoMessage

  Set-ItemProperty `
    -Path "HKLM:\System\CurrentControlSet\Control\Terminal Server" `
    -Name "fDenyTSConnections" `
    -Value 0
  
  Enable-NetFirewallRule -DisplayGroup "Remote Desktop"
  
  return (Test-RDP)
}


function Test-RDP { 
  $r = (Get-ItemProperty `
    -Path "HKLM:\System\CurrentControlSet\Control\Terminal Server" `
    -Name "fDenyTSConnections").fDenyTSConnections
  "[TEST]::fDenyTSConnections is $r" | Out-InfoMessage

  # fw, .enabled is sting -_-
  $f = (Get-NetFirewallRule -DisplayGroup "Remote Desktop").Enabled -notcontains "False"
  "[TEST]::RemoteDesktop rules enabled $f" | Out-InfoMessage

  return ($r -eq 0) -and $f
}
