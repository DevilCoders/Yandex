# include
. "$PSScriptRoot\common\yandex-powershell-logging.ps1"


function Ensure-Packertasksremoved {
  "[ENFORCE]::Ensure no packer task in scheduler" | Out-InfoMessage
  
  Get-ScheduledTask -TaskName 'packer-*' -ErrorAction 'SilentlyContinue' | ? State -eq 'Ready' | `
    Unregister-ScheduledTask -Confirm:$false

  return (Test-Packertasksremoved)
}


function Test-Packertasksremoved {
  $r = (Get-ScheduledTask -TaskName "packer-*" -ErrorAction 'SilentlyContinue').count -eq 0
  "[TEST]::No packer task in scheduler? $r" | Out-InfoMessage

  return $r
}
