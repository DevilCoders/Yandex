# shutdown without active session
Set-ItemProperty `
  -Path "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\System" `
  -Name "ShutdownWithoutLogon" `
  -Value 1

# invoke userdata at startup
& schtasks /Create /TN "userdata" /RU SYSTEM /SC ONSTART /RL HIGHEST /TR "Powershell -NoProfile -ExecutionPolicy Bypass -Command \`"& {iex (irm -H @{\\\`"Metadata-Flavor\\\`"=\\\`"Google\\\`"} \\\`"http://169.254.169.254/computeMetadata/v1/instance/attributes/user-data\\\`")}\`""

# winrm
Remove-Item -Path WSMan:\Localhost\listener\listener* -Recurse | Out-Null
New-Item -Path WSMan:\LocalHost\Listener `
    -Transport HTTP `
    -Address * `
    -Force | Out-Null

$DnsName = [System.Net.Dns]::GetHostByName($env:computerName).Hostname
$WindowsVersion = Get-ItemProperty `
  -Path 'HKLM:\Software\Microsoft\Windows NT\CurrentVersion' `
  -Name ProductName | `
    Select-Object -ExpandProperty ProductName

if ($WindowsVersion -match "Windows Server 2012 R2*") {
  # yup, black sheep
  $SelfSignedCertificateParams = @{
    CertStoreLocation = "Cert:\LocalMachine\My"
    DnsName = $DnsName
  }
} else {
  $SelfSignedCertificateParams = @{
    CertStoreLocation = "Cert:\LocalMachine\My"
    DnsName = $DnsName
    Subject = $ENV:COMPUTERNAME
  }
}

$Certificate = New-SelfSignedCertificate @SelfSignedCertificateParams
New-Item -Path WSMan:\LocalHost\Listener `
  -Transport HTTPS `
  -Address * `
  -CertificateThumbPrint $Certificate.Thumbprint `
  -HostName $ENV:COMPUTERNAME `
  -Force | Out-Null

# http firewall
if ( $WINRMHTTP = Get-NetFirewallRule -Name "WINRM-HTTP-In-TCP" -ErrorAction SilentlyContinue ) {
  $WINRMHTTP | Enable-NetFirewallRule
} else {
  New-NetFirewallRule `
    -Group "Windows Remote Management" `
    -DisplayName "Windows Remote Management (HTTP-In)" `
    -Name "WINRM-HTTP-In-TCP" `
    -LocalPort 5985 `
    -Action "Allow" `
    -Protocol "TCP" `
    -Program "System"
}

# HTTPS firewall
if ($WINRMHTTPS = Get-NetFirewallRule -Name "WINRM-HTTPS-In-TCP" -ErrorAction SilentlyContinue) {
  $WINRMHTTPS | Enable-NetFirewallRule  
} else {
  New-NetFirewallRule `
    -Group "Windows Remote Management" `
    -DisplayName "Windows Remote Management (HTTPS-In)" `
    -Name "WINRM-HTTPS-In-TCP" `
    -LocalPort 5986 `
    -Action "Allow" `
    -Protocol "TCP" `
    -Program "System"
}  

Start-Sleep -Seconds (60*10)
Stop-Computer -Force
