# ToDo:
#       * add globals?
#       * separate tests, coz on different stages you'll expect different state

# include
. "$PSScriptRoot\common\yandex-powershell-logging.ps1"
. "$PSScriptRoot\common\yandex-powershell-common.ps1"


function Ensure-Cleanup {
  "[ENFORCE]::Ensure cleanup" | Out-InfoMessage

  "[ENFORCE]::Clean downloaded updates (will restart wuauserv service)" | Out-InfoMessage
  Clear-DownloadedUpdates

  "[ENFORCE]::Clean panther logs" | Out-InfoMessage
  Clear-PantherLogs

  "[ENFORCE]::Clean temp files" | Out-InfoMessage
  Clear-TempFiles

  # bug fixed now, but there some leftovers and cleanup at sealing is not accessable in rdls
  $packer_fs_leftover_env    = "C:\Windows\Temp\packer-ps-env-vars-5f46360f-499f-0175-b9a1-40fae03b032c.ps1"
  $packer_fs_leftover_script = "C:\Windows\Temp\script-5f46360f-be78-8efa-85e3-160da3350f7e.ps1"
  if (Test-Path $packer_fs_leftover_env) { rm $packer_fs_leftover_env }
  if (Test-Path $packer_fs_leftover_script) { rm $packer_fs_leftover_script }

  "[ENFORCE]::Clean WER" | Out-InfoMessage
  Clear-WER

  "[ENFORCE]::Clean recycle bin" | Out-InfoMessage
  Clear-RecycleBin -Force

  "[ENFORCE]::Clean system logs" | Out-InfoMessage
  # we'll clean stderr l8r
  Clear-WindowsSystemLogs

  "[ENFORCE]::Clean eventlog" | Out-InfoMessage
  Clear-EventLogs

  "[ENFORCE]::Clean VSS" | Out-InfoMessage
  Clear-VSS

  "[ENFORCE]::Disable scheduled defrag" | Out-InfoMessage
  Get-ScheduledTask -TaskName 'ScheduledDefrag' | Disable-ScheduledTask | Out-Null
  
  "[ENFORCE]::Remove bootstrap" | Out-InfoMessage
  if (Test-Path c:\bootstrap) { rm c:\bootstrap -Confirm:$false -Recurse }

  # yup its not cool, but some parts must be dirty
  return $true
}


function Test-Cleanup {
  # a value judgment, won't be 0 at any times
  # messy, yhink about to rewrite it
  #$e = ((Get-WinEvent -ListLog * -ea:SilentlyContinue).LogName | `
  #  % { Get-WinEvent -Logname $_ -ea:SilentlyContinue } | `
  #    measure | `
  #      select -exp count) -lt 500
  #"[TEST]::WinEvents less than 500? $e" | Out-InfoMessage

  $b = -not (Test-Path 'C:\bootstrap')
  "[TEST]::Folder C:\bootstrap removed? $b" | Out-InfoMessage

  $s = (ls C:\Windows\Setup\Scripts\ -ea:SilentlyContinue).Count -eq 0  
  "[TEST]::Folder C:\Windows\Setup\Scripts empty? $s" | Out-InfoMessage

  #$u = (ls C:\Windows\SoftwareDistribution\ -rec -ea:SilentlyContinue | measure -sum length).sum -lt 100MB
  #"[TEST]::Folder C:\Windows\SoftwareDistribution less than 100MB? $u" | Out-InfoMessage
  
  #$v = -not [string]::IsNullOrEmpty( ((vssadmin.exe list shadows) -match 'No items found') )
  #"[TEST]::No VSS snapshots? $v" | Out-InfoMessage

  $d = (Get-ScheduledTask -TaskName 'ScheduledDefrag').State -eq 'Disabled'
  "[TEST]::Defragmentation task disabled? $s" | Out-InfoMessage

  return ($s -and
          $d -and
          $b)
}
