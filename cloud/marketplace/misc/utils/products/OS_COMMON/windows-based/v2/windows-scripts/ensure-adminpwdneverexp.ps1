# include
. "$PSScriptRoot\common\yandex-powershell-logging.ps1"


function Ensure-Adminpwdneverexp {
  "[ENFORCE]::Ensure Adminpwdneverexp" | Out-InfoMessage
  # note for tests ---> .AccountExpires && .PasswordExpires must be $null
  Set-LocalUser -Name "Administrator" -PasswordNeverExpires 1

  return (Test-Adminpwdneverexp)
}

function Test-Adminpwdneverexp {
  # no matter how it is named, SID is well-known and ends with -500
  $u = Get-LocalUser | ? SID -like *-500
  $a = $u.AccountExpires -eq $null
  $p = $u.PasswordExpires -eq $null

  "[TEST]::Account not expires? $a" | Out-InfoMessage
  "[TEST]::Password not expires? $p" | Out-InfoMessage

  return ($a -and $p)
}
