. "$PSScriptRoot\common\yandex-powershell-logging.ps1"
. "$PSScriptRoot\common\yandex-powershell-common.ps1"

function Ensure-Updates {
  "[ENFORCE]::Waiting 60s..." | Out-InfoMessage
  Start-Sleep -Seconds 60

  # ensure windows update is reachable in reasonable time
  if (-not (Test-WindowsUpdateReachable)) { Wait-WindowsUpdateReachable }
  "[ENFORCE]::Windows update is reachable: $(Test-WindowsUpdateReachable)" | Out-InfoMessage

  "[ENFORCE]::Clean old downloaded updates (will restart wuauserv service)" | Out-InfoMessage
  Clear-DownloadedUpdates

  "[ENFORCE]::Sleep 60s" | Out-InfoMessage
  Start-Sleep -Seconds 60

  # lookup
  "[ENFORCE]::Searching for updates" | Out-InfoMessage
  $windows_updates = [WindowsUpdates]::new()
  $windows_updates.Find()
  $found_updates = $windows_updates.Show()
  "[ENFORCE]::Found $($found_updates.count) updates" | Out-InfoMessage
  
  if ( $windows_updates.Show() ) {
    foreach ($update in $windows_updates.Show()) {
      "[ENFORCE]::($($update.Date), $($update.SizeMB)MB, $($update.RebootBehavior)) $($update.Title)" | Out-InfoMessage
    }

    "[ENFORCE]::Downloading updates" | Out-InfoMessage
    $windows_updates.Download()

    "[ENFORCE]::Installing updates" | Out-InfoMessage
    $windows_updates.Install()
  } else {
    # nothing to update or error
  }

  return (Test-Updates -inflight)
}

function Test-Updates {
  param(
    [switch]$inflight
  )
  
  "[TEST]::I R THEE GREAT MOCK" | Out-InfoMessage

  if ($inflight) { return $true }

  # windows updates wrapper
  $windows_updates = [WindowsUpdates]::new()

  # lookup
  "[TEST]::Searching for updates" | Out-InfoMessage
  $windows_updates.Find()
  $found_updates = $windows_updates.Show()
  "[TEST]::Found $($found_updates.count) updates" | Out-InfoMessage

  return ($found_updates.count -eq 0)
}
