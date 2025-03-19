##### Remove Bootstrat-Aftersysprep (rework image building needed)
$ErrorActionPreference='Stop'
Remove-Item "C:\Program Files\Cloudbase Solutions\Cloudbase-Init\LocalScripts\*"
# Install KMS Service
Install-WindowsFeature -Name VolumeActivation -IncludeAllSubFeature -IncludeManagementTools

##### FIREWALL #####
$cloud_ipv4_subnets = @(
'130.193.32.0/255.255.224.0',
'84.201.128.0/255.255.192.0',
'87.250.237.120/255.255.255.248',
'93.158.162.112/255.255.255.248',
'93.158.179.208/255.255.255.248',
'141.8.182.32/255.255.255.248',
'198.18.235.0/255.255.255.0',
'172.18.0.0/255.240.0.0'
)

# Disable all firewall rules except WinRM
Show-NetFirewallRule | where {$_.enabled -eq 'true' -AND $_.direction -eq 'inbound' -AND $_.Name -ne 'WINRM-HTTPS-In-TCP'} | Disable-NetFirewallRule

# Allow 1688 TCP
Set-NetFirewallRule -DisplayName 'Key Management Service (TCP-In)' `
-Description 'Inbound rule for the Key Management Service to allow for machine counting and license compliance. [TCP 1688]' `
-Action Allow `
-Enabled True `
-Profile Any `
-Direction Inbound `
-LocalPort 1688 `
-Protocol TCP `
-RemotePort Any `
-RemoteAddress $cloud_ipv4_subnets

# Allow RDP from local subnet
$local_subnets = @(
'172.16.1.0/255.255.255.0',
'172.16.2.0/255.255.255.0',
'172.16.3.0/255.255.255.0'
)

Set-NetFirewallRule -DisplayName 'Remote Desktop - User Mode (TCP-In)' `
-RemoteAddress $local_subnets `
-Enabled True

Set-NetFirewallRule -DisplayName 'Remote Desktop - User Mode (UDP-In)' `
-RemoteAddress $local_subnets `
-Enabled True

# Allow DHCP
Set-NetFirewallRule -DisplayName 'Core Networking - Dynamic Host Configuration Protocol (DHCP-In)' `
-Enabled True
Write-Host 'Done'

# Insert KMS key
# DON'T Do this. A maximum of 10 activations per key exists.
#& "C:\Windows\System32\cscript.exe" "C:\Windows\System32\slmgr.vbs" /ipk ""

Start-Sleep -s 30

# Activate KMS Server
& "C:\Windows\System32\cscript.exe" "C:\Windows\System32\slmgr.vbs" /ato
