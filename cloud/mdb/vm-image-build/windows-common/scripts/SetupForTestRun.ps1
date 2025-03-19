param (
    [int]$where
)
function ConfigureNetwork {
    if ($where -eq 1) {
        Get-Netadapter | Select-Object -First 1 | Rename-NetAdapter -NewName "eth1" -Confirm:$false
        New-NetIPAddress -InterfaceAlias "eth1" -AddressFamily IPv6 -IPAddress fd01:ffff:ffff:ffff::42 -PrefixLength 96 -DefaultGateway fd01:ffff:ffff:ffff::1
        Set-DnsClientServerAddress -InterfaceAlias "eth1" -ServerAddresses @("2a02:6b8:0:3400::5005", "2a02:6b8:0:3400::1")
        Get-Netadapter | Where-Object { $_.Name -ne "eth1" } | Rename-NetAdapter -NewName "eth0"
    }
    else {
        Get-NetAdapter | Select-Object -First 1 | Rename-NetAdapter -NewName eth1 -Confirm:$false
    }

}

function SetHostName {
    Rename-Computer vm-img-win-t -Force
    Add-Computer -WorkgroupName "db.yandex.net"
    
    Set-ItemProperty -Path HKLM:\system\CurrentControlSet\Services\tcpip\parameters -Name "Hostname" -Value vm-img-win-t
    Set-ItemProperty -Path HKLM:\system\CurrentControlSet\Services\tcpip\parameters -Name "NV Hostname" -Value vm-img-win-t
    Set-ItemProperty -Path HKLM:\system\CurrentControlSet\Services\tcpip\parameters -Name "Domain" -Value db.yandex.net
    Set-ItemProperty -Path HKLM:\system\CurrentControlSet\Services\tcpip\parameters -Name "NV Domain" -Value db.yandex.net
}

function ConfigureMinion {
    Set-Content -Path "C:\salt\conf\hostname" -Value "vm-image-template-windows-test.db.yandex.net"
    Set-Content -Path "C:\salt\conf\minion_id" -Value "vm-image-template-windows-test.db.yandex.net"
    Set-Content -Path "C:\salt\conf\mdb_deploy_api_host" -Value "deploy-api-test.db.yandex-team.ru"
}

function EjectCDImage {
    $cdrom = Get-WmiObject win32_volume | Where-Object { $_.DriveType -eq 5 }
    if ($null -ne $cdrom) {
        $cdrom.DriveLetter = "Z:"
        $cdrom.Put()
    }
}

function SetDefaultFirewallRules {
    if ($where -eq 1) {
        New-NetFirewallRule -DisplayName mdb_intranet_tcp80_out -RemotePort 80 -Protocol tcp -RemoteAddress any -Direction Outbound -Action Allow
        New-NetFirewallRule -DisplayName mdb_intranet_tcp443_out -RemotePort 443 -Protocol tcp -RemoteAddress any -Direction Outbound -Action Allow
        #remove all the built-in standard rules
        Get-NetFirewallRule | Where-Object { $_.DisplayName -notlike 'mdb_*' -and $_.DisplayName -notlike "OpenSSH Server (sshd)" -and $_.Enabled -eq 'true' } | disable-netfirewallrule
        Set-NetFirewallProfile -Name Public -DefaultOutboundAction Block
        Set-NetFirewallProfile -Name Private -DefaultOutboundAction Block
    }
    else {
        Get-NetFirewallRule | Where-Object { $_.DisplayName -notlike 'mdb_*' -and $_.DisplayName -notlike "OpenSSH Server (sshd)" -and $_.Enabled -eq 'true' -and $_.Direction -like 'Inbound' } | disable-netfirewallrule
    }
}

function GetAdminPassword {
    Write-Host $(Get-Date) "DEBUG: Getting Administrator's password"
    $password = $null
    for ($attempt = 0; $attempt -lt 10; $attempt++) {
        try {
            $password = salt-call pillar.get --out newline_values_only "data:windows:users:Administrator:password"
            $password = $password.Trim()
            break
        }
        catch {
            Write-Host "Failed to get root password: $_, sleeping.."
            Start-Sleep -s 30
        }
    }
    if (($password -eq $null) -or ($password -eq "")) {
        throw "Failed to get Administrator password from salt within timeout"
    }
    return $password
}

function ChangeUserPassword {
    param(
        [string]$username,
        [securestring]$password
    )
    # change Administrator password
    Write-Host $(Get-Date) "DEBUG: Change users password for user $username"
    $computername = $env:computername
    $username = "Administrator"
    Set-LocalUser -Name $username -Password $password -PasswordNeverExpires $true
}

function SetSaltStartupAccount {
    param(
        [string]$username,
        [string]$password
    )
    Write-Host $(Get-Date) "DEBUG: Change salt minion startup type"
    $service = gwmi win32_service -filter "name='salt-minion'"
    $service.change($null, $null, $null, $null, $null, $null, "$env:computername\$username", $password)
}

###############

ConfigureNetwork
SetHostName
ConfigureMinion
EjectCDImage
SetDefaultFirewallRules
$password = GetAdminPassword
$pw = ConvertTo-SecureString -AsPlainText -Force $password
ChangeUserPassword 'Administrator' $pw
SetSaltStartupAccount 'Administrator' $password
Write-Host $(Get-Date) "DEBUG: Set salt-minion startup type"
Set-Service -Name salt-minion -StartupType Automatic
