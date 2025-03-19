#ps1
# ^^^ 'ps1' is only for cloudbase-init, some sort of sh-bang in linux

#New-ItemProperty -Name `
#    "LocalAccountTokenFilterPolicy" `
#    -Path "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\System" `
#    -PropertyType DWord `
#    -Value 1

# logging
Start-Transcript -Path "$ENV:SystemDrive\provision.txt" -IncludeInvocationHeader -Force
"Bootstrap script started" | Write-Host

# inserting value's from terraform
$MyAdministratorPlainTextPassword = '${ admin_pass }'
if (-not [string]::IsNullOrEmpty($MyAdministratorPlainTextPassword))
{
    "Set local administrator password" | Write-Host
    $MyAdministratorPassword = $MyAdministratorPlainTextPassword | ConvertTo-SecureString -AsPlainText -Force
    # S-1-5-21domain-500 is a well-known SID for Administrator
    # https://docs.microsoft.com/en-us/troubleshoot/windows-server/identity/security-identifiers-in-windows
    $MyAdministrator = Get-LocalUser | Where-Object -Property "SID" -like "S-1-5-21-*-500"
    $MyAdministrator | Set-LocalUser -Password $MyAdministratorPassword
}
else
{
    throw "no password provided for Administrator account"
}

"Bootstrap script ended" | Write-Host
