ls "$PSScriptRoot\common" -fil *.ps1 | % { . $_.FullName }

# expose
function ensure-packertasksremoved {
  # packer got bug, resolving
  Get-ScheduledTask -TaskName "packer-*" -ErrorAction 'SilentlyContinue' | Unregister-ScheduledTask -Confirm:$false
}

# runnable
if (-not (Test-DotSourced)) { ensure-packertasksremoved }
