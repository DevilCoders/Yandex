# importing all libraries from common directory
$libs = Get-ChildItem "$PSScriptRoot\common" -Filter *.ps1
foreach ($lib in $libs.FullName) { . $lib }

function ensure-eth {
  $ethIndexes = Get-InstanceEthIndexes
  foreach ($index in $ethIndexes)
  {
    $MacAddress = Get-InstanceEthMacAddress -Index $index
    $NetAdapter = Get-NetAdapter | Where-Object MacAddress -eq ($MacAddress -replace ":", "-")
    if ($NetAdapter.Name -ne "eth$index") {
      $NetAdapter | Rename-NetAdapter -NewName "eth$index"
      Write-Output "renamed netAdapter $MacAddress to: eth$index"
    }
  }
}

if (-not (Test-DotSourced)) { ensure-eth }
