# include
. "$PSScriptRoot\common\yandex-powershell-logging.ps1"


function Ensure-ICMP {
  "[ENFORCE]::Ensuring ICMP" | Out-InfoMessage

  Get-NetFirewallRule -Name "vm-monitoring-icmpv4" | `
    Enable-NetFirewallRule
  
  return (Test-ICMP)
}


function Test-ICMP {
  $r = (Get-NetFirewallRule -Name "vm-monitoring-icmpv4").Enabled -eq 'true'
  
  "[TEST]::ICMP rule enabled? $r" | Out-InfoMessage

  return ($r)
}
