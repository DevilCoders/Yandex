#ps1

# we first set administrators password, then check and create scheduled task, 
# that will invoke script from metadata's 'deploy' key this task will run on 
# every system start, so its removal/reporting etc is its own responsibility

$MyAdministratorPlainTextPassword = '${ admin_pass }'
if (-not [string]::IsNullOrEmpty($MyAdministratorPlainTextPassword))
{
    "set local administrator password" | Write-Host
    $MyAdministratorPassword = $MyAdministratorPlainTextPassword | ConvertTo-SecureString -AsPlainText -Force

    # S-1-5-21domain-500 is a well-known SID for Administrator
    # https://docs.microsoft.com/en-us/troubleshoot/windows-server/identity/security-identifiers-in-windows
    $MyAdministrator = Get-LocalUser | Where-Object -Property "SID" -like "S-1-5-21-*-500"
    $MyAdministrator | Set-LocalUser -Password $MyAdministratorPassword
}
else
{
    throw "no password provided for Administrator account"
    exit 1
}

"check if deployment script is set at metadata 'deploy' key" | Write-Host

try {
    $param = @{
        Headers = @{"Metadata-Flavor"="Google"}
        Uri = "http://169.254.169.254/computeMetadata/v1/instance/attributes/deploy"
    }
    $deployment = Invoke-RestMethod @param 
}
catch [System.Net.WebException] {
    "no deployment availible or got error: $($PSItem.ToString())" | Write-Host
}

if ( -not [string]::IsNullOrEmpty($deployment) ) {
    "deployment found" | Write-Host
    schtasks /Create /TN "deploy" /RU SYSTEM /SC ONSTART /RL HIGHEST /TR "Powershell -NoProfile -ExecutionPolicy Bypass -Command \`"& {iex (irm -H @{\\\`"Metadata-Flavor\\\`"=\\\`"Google\\\`"} \\\`"http://169.254.169.254/computeMetadata/v1/instance/attributes/deploy\\\`")}\`""
}

"init complete" | Write-Host
