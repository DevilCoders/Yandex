# ToDo:
#       * add globals 
#         can away hard-coded TaskName
#         think before, it'll force rewriting quemu builder bootstrap


# include
. "$PSScriptRoot\common\yandex-powershell-logging.ps1"


function Ensure-UserdataRemoved {
  "[ENFORCE]::Removing userdata scheduled task" | Out-InfoMessage
  Get-ScheduledTask -TaskName "userdata" | Unregister-ScheduledTask -Confirm:$false

  return (Test-UserdataRemoved)
}


function Test-UserdataRemoved {
  $r = (Get-ScheduledTask -TaskName "userdata" -ea:SilentlyContinue) -eq $null
  "[TEST]::Scheduled task exist? $r" | Out-InfoMessage

  return $r
}
