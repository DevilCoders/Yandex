<#
    .FUNCTIONS
        Functions, used in script below.
#>

function Get-WinrmCertificateThumbPrint {
    $Listner = Get-WinrmHTTPSListener
    return Get-ChildItem "WSMan:\localhost\Listener\$Listner\CertificateThumbPrint" | `
        Select-Object -ExpandProperty Value
}

function Get-WinrmHTTPSListener {
    return Get-ChildItem WSMan:\localhost\Listener\ | `
        Where-Object Keys -contains 'Transport=HTTPS' | Select-Object -ExpandProperty Name
}

function New-WinrmCertificate {
    $DnsName = [System.Net.Dns]::GetHostByName($env:computerName).Hostname
    $WindowsVersion = Get-WindowsVersion
    if ($WindowsVersion -match "Windows Server 2012 R2*") {
        # yup, black sheep
        return New-SelfSignedCertificate `
            -CertStoreLocation "Cert:\LocalMachine\My" `
            -DnsName $DnsName
    }

    return New-SelfSignedCertificate `
        -CertStoreLocation "Cert:\LocalMachine\My" `
        -DnsName $DnsName `
        -Subject $ENV:COMPUTERNAME
}

function Get-WindowsVersion {
    return Get-ItemProperty `
        -Path 'HKLM:\Software\Microsoft\Windows NT\CurrentVersion' `
        -Name ProductName | Select-Object -ExpandProperty ProductName
}

<#
    .SCRIPT
        Actual script
#>

# Remove old winrm certificate, if any

Remove-Item -Path "Cert:\LocalMachine\My\$CertificateThumbPrint" | Out-Null

# Remove old winrm listeners, if any

Remove-Item -Path WSMan:\Localhost\listener\listener* -Recurse | Out-Null

# Create Listeners

$Certificate = New-WinrmCertificate

New-Item -Path WSMan:\LocalHost\Listener `
    -Transport HTTPS `
    -Address * `
    -CertificateThumbPrint $Certificate.ThumbPrint `
    -HostName $ENV:COMPUTERNAME `
    -Force | Out-Null

New-Item -Path WSMan:\LocalHost\Listener `
    -Transport HTTP `
    -Address * `
    -HostName $ENV:COMPUTERNAME `
    -Force | Out-Null

# Create allow firewall rules

if ($WINRMHTTPS = Get-NetFirewallRule -Name "WINRM-HTTPS-In-TCP" -ErrorAction SilentlyContinue) {
    $WINRMHTTPS | Enable-NetFirewallRule  
}
else {
    New-NetFirewallRule `
        -Group "Windows Remote Management" `
        -DisplayName "Windows Remote Management (HTTPS-In)" `
        -Name "WINRM-HTTPS-In-TCP" `
        -LocalPort 5986 `
        -Action "Allow" `
        -Protocol "TCP" `
        -Program "System"
}

if ($WINRMHTTPS = Get-NetFirewallRule -Name "WINRM-HTTP-In-TCP" -ErrorAction SilentlyContinue) {
    $WINRMHTTPS | Enable-NetFirewallRule  
}
else {
    New-NetFirewallRule `
        -Group "Windows Remote Management" `
        -DisplayName "Windows Remote Management (HTTP-In)" `
        -Name "WINRM-HTTP-In-TCP" `
        -LocalPort 5985 `
        -Action "Allow" `
        -Protocol "TCP" `
        -Program "System"
}
