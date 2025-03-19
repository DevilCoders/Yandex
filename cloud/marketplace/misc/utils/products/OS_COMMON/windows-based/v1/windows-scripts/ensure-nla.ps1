ls "$PSScriptRoot\common" -fil *.ps1 | % { . $_.FullName }

# expose
function ensure-nla { 
  Get-CimInstance `
    -ClassName Win32_TSGeneralSetting `
    -Namespace root\cimv2\terminalservices | `
      Invoke-CimMethod `
        -MethodName SetUserAuthenticationRequired `
        -Arguments @{ UserAuthenticationRequired = 1 }
}

# runnable
if (-not (Test-DotSourced)) { ensure-nla }
