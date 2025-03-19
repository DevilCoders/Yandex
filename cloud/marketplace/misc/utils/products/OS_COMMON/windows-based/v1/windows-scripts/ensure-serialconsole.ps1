ls "$PSScriptRoot\common" -fil *.ps1 | % { . $_.FullName }

# expose
function ensure-serialconsole {
  # note for tests ---> (& bcdedit /enum "{current}" | select-string ems) -match "Yes"
  & bcdedit /ems "{current}" on
  & bcdedit /emssettings EMSPORT:2 EMSBAUDRATE:115200
}

# runnable
if (-not (Test-DotSourced)) { ensure-serialconsole }
