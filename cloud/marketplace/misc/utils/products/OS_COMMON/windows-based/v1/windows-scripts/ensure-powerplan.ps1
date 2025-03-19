# importing all libraries from common directory
$libs = Get-ChildItem "$PSScriptRoot\common" -Filter *.ps1
foreach ($lib in $libs.FullName) { . $lib }

function ensure-powerplan {
  # ensure powerplan is 'high performance'
  $HighPerformance = "8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c"
  if ( (Get-ActivePowerScheme) -notmatch $HighPerformance ) { Set-ActivePowerScheme $HighPerformance }
  Write-Output "power plan scheme: $(Get-ActivePowerScheme)"
}

if (-not (Test-DotSourced)) { ensure-powerplan }
