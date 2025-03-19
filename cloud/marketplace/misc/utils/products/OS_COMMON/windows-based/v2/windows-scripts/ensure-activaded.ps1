# ToDo:
#       * add more tests for verbosity?


# include
. "$PSScriptRoot\common\yandex-powershell-globals.ps1"
. "$PSScriptRoot\common\yandex-powershell-logging.ps1"
. "$PSScriptRoot\common\yandex-powershell-common.ps1"


function Ensure-Activaded {
  "[ENFORCE]::Ensure windows is activaded" | Out-InfoMessage
  
  Set-KMSServer $global:__KMSServer
  Set-KMSClientKey
  Invoke-WindowsActivation

  return (Test-Activaded)
}

function Test-Activaded {
  $r = (& cscript /nologo C:\Windows\system32\slmgr.vbs /ato)[-2] -eq 'Product activated successfully.'
  "[TEST]::Windows activaded? $r" | Out-InfoMessage

  return $r
}
