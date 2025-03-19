# importing all libraries from common directory
$libs = Get-ChildItem "$PSScriptRoot\common" -Filter *.ps1
foreach ($lib in $libs.FullName) { . $lib }

function ensure-icmp {
  if (-not (Get-IcmpFirewallRule)) {
    Enable-IcmpFirewallRule
  }  
  Write-Output "allow icmp rule: $(Get-IcmpFirewallRule)"
}

if (-not (Test-DotSourced)) { ensure-icmp }
