ls "$PSScriptRoot\common" -fil *.ps1 | % { . $_.FullName }

# expose
function ensure-rtu { 
  Set-ItemProperty `
    -Path 'HKLM:\SYSTEM\CurrentControlSet\Control\TimeZoneInformation' `
    -Name "RealTimeIsUniversal" `
    -Value 1 `
    -Type DWord `
    -Force
}

# runnable
if (-not (Test-DotSourced)) { ensure-rtu }
