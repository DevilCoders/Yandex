# importing all libraries from common directory
$libs = Get-ChildItem "$PSScriptRoot\common" -Filter *.ps1
foreach ($lib in $libs.FullName) { . $lib }

function ensure-winrm {
  Clear-WinrmListeners
  Clear-MyCertificates
  New-WinrmHTTPListener
  New-WinrmHTTPSListener -CertificateThumbPrint (New-WinrmCertificate).ThumbPrint
  Enable-WinrmFirewallRules
  Write-Output "configured winrm service"
}

if (-not (Test-DotSourced)) { ensure-winrm }
