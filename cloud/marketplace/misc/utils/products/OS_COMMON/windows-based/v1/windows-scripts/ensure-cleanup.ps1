# importing all libraries from common directory
$libs = Get-ChildItem "$PSScriptRoot\common" -Filter *.ps1
foreach ($lib in $libs.FullName) { . $lib }

# expose methods

function ensure-baseimagecleanup {
  Write-Host "Clean downloaded updates (will restart wuauserv service)"
  Clear-DownloadedUpdates

  Write-Host "Clean panther logs"
  Clear-PantherLogs

  Write-Host "Clean temp files"
  Clear-TempFiles

  Write-Host "Clean WER"
  Clear-WER

  Write-Host "Clean recycle bin"
  Clear-RecycleBin -Force

  Write-Host "Clean system logs"
  Clear-WindowsSystemLogs

  Write-Host "Clean eventlog"
  Clear-EventLogs

  Write-Host "Clean VSS"
  Clear-VSS

  Write-Host "Disable scheduled defrag"
  Get-ScheduledTask -TaskName 'ScheduledDefrag' | Disable-ScheduledTask | Out-Null
  
  Write-Host "Remove bootstrap"
  if (Test-Path c:\bootstrap) { rm c:\bootstrap -Confirm:$false -Recurse }
}

# make it 'in-place' runnable
if (-not (Test-DotSourced)) { ensure-baseimagecleanup }
