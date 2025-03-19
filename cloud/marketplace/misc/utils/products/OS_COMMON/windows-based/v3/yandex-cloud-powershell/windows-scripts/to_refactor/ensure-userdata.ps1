# include
. "$PSScriptRoot\common\yandex-powershell-logging.ps1"


function Ensure-Userdata {
  "[ENFORCE]::Ensure RDP" | Out-InfoMessage
  
  & schtasks /Create /TN "userdata" /RU SYSTEM /SC ONSTART /RL HIGHEST /TR "Powershell -NoProfile -ExecutionPolicy Bypass -Command \`"& {iex (irm -H @{\\\`"Metadata-Flavor\\\`"=\\\`"Google\\\`"} \\\`"http://169.254.169.254/computeMetadata/v1/instance/attributes/user-data\\\`")}\`""

  return (Test-Userdata)
}


function Test-Userdata {
  $r = (Get-ScheduledTask -TaskName 'userdata') -ne $null
  "[REG]::Userdata invoker script exist? is $r" | Out-InfoMessage
}
