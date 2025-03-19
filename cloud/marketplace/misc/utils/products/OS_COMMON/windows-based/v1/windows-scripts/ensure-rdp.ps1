ls "$PSScriptRoot\common" -fil *.ps1 | % { . $_.FullName }

# expose
function ensure-rdp { 
  Set-ItemProperty `
    -Path "HKLM:\System\CurrentControlSet\Control\Terminal Server" `
    -Name "fDenyTSConnections" `
    -Value 0
  
  Enable-NetFirewallRule -DisplayGroup "Remote Desktop" 
}

# runnable
if (-not (Test-DotSourced)) { ensure-rdp }
