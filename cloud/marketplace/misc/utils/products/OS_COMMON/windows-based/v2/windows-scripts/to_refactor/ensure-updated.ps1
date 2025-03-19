# include
. "$PSScriptRoot\common\yandex-powershell-logging.ps1"
. "$PSScriptRoot\common\yandex-powershell-common.ps1"


function Ensure-WindowsUpdated {
  # ensure windows update is reachable in reasonable time
  if (-not (Test-WindowsUpdateReachable)) { Wait-WindowsUpdateReachable }
  Write-Output "windows update is reachable: $(Test-WindowsUpdateReachable)"

  # windows updates wrapper
  $windows_updates = [WindowsUpdates]::new()

  # lookup
  Write-Host "Searching for updates"
  $windows_updates.Find()
  $found_updates = $windows_updates.Show()
  Write-Host "Found $($found_updates.count) updates"
  
  if ( $windows_updates.Show() ) {
    Write-Host "Downloading updates"
    $windows_updates.Download()

    Write-Host "Installing updates"
    $windows_updates.Install()
  } else {
    # nothing to update or error
  }
}


function Test-WindowsUpdated {

}
