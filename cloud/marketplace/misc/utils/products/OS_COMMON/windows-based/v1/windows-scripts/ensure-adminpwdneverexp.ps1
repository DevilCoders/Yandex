ls "$PSScriptRoot\common" -fil *.ps1 | % { . $_.FullName }

# expose
function ensure-adminpwdneverexp {
  # note for tests ---> .AccountExpires && .PasswordExpires must be $null
  Set-LocalUser -Name "Administrator" -PasswordNeverExpires 1
}

# runnable
if (-not (Test-DotSourced)) { ensure-adminpwdneverexp }
