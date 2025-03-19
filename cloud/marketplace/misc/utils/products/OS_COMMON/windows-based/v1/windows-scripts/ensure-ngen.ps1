# importing all libraries from common directory
$libs = Get-ChildItem "$PSScriptRoot\common" -Filter *.ps1
foreach ($lib in $libs.FullName) { . $lib }

# expose methods

function ensure-ngen {
  $Ngen = Get-NgenImage
  foreach ($AssemblyPath in (Get-PowershellAssemblies)) { & $Ngen install $AssemblyPath | Out-Null }
  Write-Output "executed ngen for powershell assemblies"

  foreach($Ngen in (Get-NgenImagesPath)) { & $Ngen update /force | Out-Null }
  Write-Output "executed ngen forcefully"
}

# make it 'in-place' runnable

if (-not (Test-DotSourced)) { ensure-ngen }
