# importing all libraries from common directory
$libs = Get-ChildItem "$PSScriptRoot\common" -Filter *.ps1
foreach ($lib in $libs.FullName) { . $lib }

# expose methods

function ensure-shutdownwithoutlogon {
  if (-not (Get-ShutdownWithoutLogonPolicy)) { Enable-ShutdownWithoutLogonPolicy }
  Write-Output "shutdown without logon: $(Get-ShutdownWithoutLogonPolicy)"
}

# make it 'in-place' runnable
if (-not (Test-DotSourced)) { ensure-shutdownwithoutlogon }
