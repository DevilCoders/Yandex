# remove existing winrm listener's
Remove-Item -Path WSMan:\Localhost\listener\listener* -Recurse | Out-Null

# winrm-http-listener
New-Item -Path WSMan:\LocalHost\Listener `
  -Transport HTTP `
  -Address * `
  -Force | Out-Null

# winrm-https listener
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

# winrm-https firewall (winrm-http rule exist out-of-the-box)
New-NetFirewallRule `
  -Group "Windows Remote Management" `
  -DisplayName "Windows Remote Management (HTTPS-In)" `
  -Name "WINRM-HTTPS-In-TCP" `
  -LocalPort 5986 `
  -Action "Allow" `
  -Protocol "TCP" `
  -Program "System"

rm "C:\Windows\Setup\Scripts\Setupcomplete.ps1"
Stop-Computer -Force
