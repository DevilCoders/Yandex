ls "$PSScriptRoot\common" -fil *.ps1 | % { . $_.FullName }

# expose
function ensure-smb1 {
  $smb1 = Get-WindowsFeature FS-SMB1
  if ($smb1.Installed) {
    Remove-WindowsFeature FS-SMB1 -Restart:$false -Confirm:$false -ErrorAction 'SilentlyContinue' | Out-Null
  }  
}

# runnable
if (-not (Test-DotSourced)) { ensure-smb1 }
