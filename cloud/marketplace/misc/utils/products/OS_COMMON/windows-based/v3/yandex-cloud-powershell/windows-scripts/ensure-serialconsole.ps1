# ToDo:
#       * find a way to get EMSPORT and EMSBAUDRATE


# include
. "$PSScriptRoot\common\yandex-powershell-logging.ps1"


# expose
function Ensure-Serialconsole {
  "[ENFORCE]::Ensure EMS" | Out-InfoMessage

  & bcdedit /ems "{current}" on
  & bcdedit /emssettings EMSPORT:2 EMSBAUDRATE:115200

  return Test-Serialconsole
}


function Test-Serialconsole {
  $r = (& bcdedit /enum "{current}" | select-string ems) -match "Yes"

  "[TEST]::EMS enabled? $r" | Out-InfoMessage

  return $r
}
