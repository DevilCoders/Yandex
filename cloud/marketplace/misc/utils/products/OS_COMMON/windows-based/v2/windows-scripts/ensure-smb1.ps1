# include
. "$PSScriptRoot\common\yandex-powershell-logging.ps1"


function Ensure-Smb1 {
  "[ENFORCE]::Ensure SMB1" | Out-InfoMessage
  
  Remove-WindowsFeature `
    FS-SMB1 `
    -Restart:$false `
    -Confirm:$false `
    -ErrorAction 'SilentlyContinue' | `
      Out-Null
  
  return (Test-SMB1)
}


function Test-SMB1 {
  $r = -not (Get-WindowsFeature FS-SMB1).Installed

  "[TEST]::SMB1 not installed? $r" | Out-InfoMessage

  return $r
}
