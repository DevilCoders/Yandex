# ToDo:
#      * write logging

# include
. "$PSScriptRoot\common\yandex-powershell-logging.ps1"
. "$PSScriptRoot\common\yandex-powershell-common.ps1"


function Ensure-NGen {
  $Ngen = Get-NgenImage
  foreach ($AssemblyPath in (Get-PowershellAssemblies)) { & $Ngen install $AssemblyPath | Out-Null }
  Write-Output "executed ngen for powershell assemblies"

  foreach($Ngen in (Get-NgenImagesPath)) { & $Ngen update /force | Out-Null }
  Write-Output "executed ngen forcefully"

  return (Test-NGen)
}

function Test-NGen {
  # dont needed
  return $true
}