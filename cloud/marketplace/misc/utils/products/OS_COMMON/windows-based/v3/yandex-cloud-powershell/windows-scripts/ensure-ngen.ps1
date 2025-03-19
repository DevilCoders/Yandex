. "$PSScriptRoot\common\yandex-powershell-logging.ps1"
. "$PSScriptRoot\common\yandex-powershell-common.ps1"

function Ensure-NGen {
  $Ngen = Get-NgenImage
  foreach ($AssemblyPath in (Get-PowershellAssemblies)) { & $Ngen install $AssemblyPath | Out-Null }
  "Executed ngen for powershell assemblies" | Out-InfoMessage

  foreach($Ngen in (Get-NgenImagesPath)) { & $Ngen update /force | Out-Null }
  "Executed ngen forcefully" | Out-InfoMessage

  return (Test-NGen -inflight)
}

# not needed, methods not iplemented in pipeline, so...
function Test-NGen {
  param(
    [switch]$inflight
  )
  
  "Im just kind of mock" | Out-InfoMessage

  if ($inflight) { return $true }
  return $false
}