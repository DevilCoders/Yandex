<#
    .TODO
        Download from S3, packer upload is very slow
        coz all binary data dehydrates into BASE64,
        uploads into remote temporary file as generated
        commands, and finally restores into binary file.
#>

$PreviousErrorActionPreference = $ErrorActionPreference
$ErrorActionPreference = [System.Management.Automation.ActionPreference]::Stop

function New-DesktopAgentCertificate {
    $p = @{
        CertStoreLocation = "Cert:\LocalMachine\My"
        Subject = "CloudDesktopAgent"
    }
    return New-SelfSignedCertificate @p
}

$Certificate = ls Cert:\LocalMachine\My\* | Where-Object Subject -eq "CN=CloudDesktopAgent"
if ($null -eq $Certificate) {
    $Certificate = New-DesktopAgentCertificate
}

# create service

$ServiceName = "cloud-desktop-agent"

$p = @{
    Name = $ServiceName
    DisplayName = "Cloud Desktop Agent"
    BinaryPathName = "C:\Program Files\Yandex.Cloud\Cloud Desktop\DesktopAgent.exe --urls `"http://+:5000;https://[::]:5050`" `"Kestrel:Certificates:Default:Subject=CloudDesktopAgent`" `"Kestrel:Certificates:Default:Store=My`" `"Kestrel:Certificates:Default:Location=LocalMachine`" `"Kestrel:Certificates:Default:AllowInvalid=true`""
    Description = "Cloud Desktop Agent, automates actions from Cloud Desktop control plane service."
    StartupType = "Automatic"
}
New-Service @p

Start-Service $ServiceName
Start-Sleep -Seconds 5

$Service = Get-Service $ServiceName
if ($Service.Status -ne "Running") {
    throw "Expected service is running, but got $($Service.Status)"
}

$ErrorActionPreference = $PreviousErrorActionPreference
