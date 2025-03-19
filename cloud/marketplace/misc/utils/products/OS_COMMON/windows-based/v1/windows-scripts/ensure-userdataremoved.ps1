ls "$PSScriptRoot\common" -fil *.ps1 | % { . $_.FullName }

# expose
function ensure-userdataremoved {
  Get-ScheduledTask -TaskName "userdata" | Unregister-ScheduledTask -Confirm:$false
}

# runnable
if (-not (Test-DotSourced)) { ensure-userdataremoved }
