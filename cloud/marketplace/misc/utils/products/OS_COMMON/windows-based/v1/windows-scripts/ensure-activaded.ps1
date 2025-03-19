# importing all libraries from common directory
$libs = Get-ChildItem "$PSScriptRoot\common" -Filter *.ps1
foreach ($lib in $libs.FullName) { . $lib }

function ensure-activaded {
  # ensure kms server
  Set-KMSServer "kms.cloud.yandex.net:1688"

  # ensure kms client key
  Set-KMSClientKey

  # ensure activation status
  Write-Output "windows activataded? $(Invoke-WindowsActivation)"
}

if (-not (Test-DotSourced)) { ensure-activaded }
