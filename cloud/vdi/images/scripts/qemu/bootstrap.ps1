# serial console

& bcdedit /ems "{current}" on
& bcdedit /emssettings EMSPORT:2 EMSBAUDRATE:115200

# powerplan

& powercfg -setactive "8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c"
& powercfg -change -monitor-timeout-ac 0
& powercfg -change -standby-timeout-ac 0
& powercfg -change -hibernate-timeout-ac 0

# shutdown
Set-ItemProperty -Path "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\System" -Name "ShutdownWithoutLogon" -Value 1
Set-ItemProperty -Path "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Windows" -Name "ShutdownWarningDialogTimeout" -Value 1

# clock
Set-ItemProperty -Path 'HKLM:\SYSTEM\CurrentControlSet\Control\TimeZoneInformation' -Name "RealTimeIsUniversal" -Value 1 -Type DWord -Force

# icmp
Get-NetFirewallRule -Name "vm-monitoring-icmpv4" | Enable-NetFirewallRule

# invoke userdata at startup
& schtasks /Create /TN "userdata" /RU SYSTEM /SC ONSTART /RL HIGHEST /TR "Powershell -NoProfile -ExecutionPolicy Bypass -Command \`"& {iex (irm -H @{\\\`"Metadata-Flavor\\\`"=\\\`"Google\\\`"} \\\`"http://169.254.169.254/computeMetadata/v1/instance/attributes/user-data\\\`")}\`"" | Out-Null

# enable Administrator
Get-LocalUser Administrator | Enable-LocalUser

# setupcomplete
mkdir "C:\Windows\Setup\Scripts" | Out-Null
cp "$PSScriptRoot\SetupComplete.cmd" "C:\Windows\Setup\Scripts\"
cp "$PSScriptRoot\SetupComplete.ps1" "C:\Windows\Setup\Scripts\"
cp "$PSScriptRoot\SetupWinRM.ps1" "C:\Windows\Setup\Scripts\"

# set location?
