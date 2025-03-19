# include
. "$PSScriptRoot\common\yandex-powershell-logging.ps1"


function Ensure-NLA {
  "[ENFORCE]::Ensure NLA" | Out-InfoMessage

  Get-CimInstance `
    -ClassName Win32_TSGeneralSetting `
    -Namespace root\cimv2\terminalservices | `
      Invoke-CimMethod `
        -MethodName SetUserAuthenticationRequired `
        -Arguments @{ UserAuthenticationRequired = 1 } | `
          Out-Null
  
  return (Test-NLA)
}


function Test-NLA {
  $nla = (Get-CimInstance `
    -ClassName Win32_TSGeneralSetting `
    -Namespace root\cimv2\terminalservices).UserAuthenticationRequired

  "[TEST]::NLA setting is $nla" | Out-InfoMessage
  
  return $nla -eq 1
}
