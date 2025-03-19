# importing all libraries from common directory
$libs = Get-ChildItem "$PSScriptRoot\common" -Filter *.ps1
# import
foreach ($lib in $libs.FullName) {
  Write-Host "found `"$($libs.FullName)`", dot-sourcing"
  . $lib
}

# expose methods

function ensure-windowsupdates {
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

  # later we could us it for reboot and rerun updates
  #exit (Get-RebootRequiredCode)
}

# make it 'in-place' runnable

if (-not (Test-DotSourced) ) { ensure-windowsupdates }
