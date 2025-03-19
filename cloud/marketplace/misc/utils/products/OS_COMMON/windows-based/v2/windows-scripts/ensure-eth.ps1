# include
. "$PSScriptRoot\common\yandex-powershell-common.ps1"
. "$PSScriptRoot\common\yandex-powershell-logging.ps1"


function Ensure-Eth {
  "[ENFORCE]::Ensure adapters names" | Out-InfoMessage

  foreach ($index in (Get-InstanceMetadataEthIndexes))
  {
    $MacAddress = Get-InstanceEthMacAddress -Index $index
    $NetAdapter = Get-NetAdapter | `
      Where-Object MacAddress -eq ($MacAddress -replace ":", "-")

    if ($NetAdapter.Name -ne "eth$index") {
      $NetAdapter | Rename-NetAdapter -NewName "eth$index"
    }
  }

  return (Test-Eth)
}


function Test-Eth {
  $CorrectNames = Get-InstanceMetadataEthIndexes | % { "eth$_" }
  $ActualNames = (Get-NetAdapter).Name
  $r = (diff $CorrectNames $ActualNames).count -eq 0

  "[TEST]::Adapters named correctly? $r" | Out-InfoMessage

  return $r
}