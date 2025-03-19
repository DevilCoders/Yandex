# Log execution
Start-Transcript -IncludeInvocationHeader -Append -NoClobber -Path "C:\Logs\SetupCompelete.log"

# Stop at every error
$ErrorActionPreference = "Stop"


# Stop and clean salt-minion if any
Write-Host $(Get-Date) "DEBUG: Stop and clean minion"

function SaveLogs {
    Copy-Item C:\windows\system32\winevt\Logs\Application.evtx C:\Logs\Application_$(Get-Date -Format FileDateTime).evtx
    Copy-Item C:\windows\system32\winevt\Logs\System.evtx C:\Logs\System_$(Get-Date -Format FileDateTime).evtx
}


function StopSaltMinion {
    Set-Service salt-minion -StartupType Disabled
    $attempts = 0
    While ($attempts -lt 20 -and (Get-Service salt-minion).status -eq 'Running') {
        try {
            C:\salt\bin\ssm.exe stop salt-minion
            Remove-Item 'C:\salt\var\log\salt\minion' -Force
        }
        catch {
            $attempts++
        }
        if ($attempts -eq 20) {
            try {
                #You've been warned, ye little dirty minion. Now you get what you deserve.
                SaveLogs
                Get-Process ssm | Stop-Process -Force
                Get-Process python | Stop-Process -Force
                Remove-Item 'C:\salt\var\log\salt\minion' -Force
            }
            catch {
                #What the hell are you?
                SaveLogs
                Write-Host $(Get-Date) "DEBUG: Something went clearly wrong while trying to stop salt-minion!"
            }
            
        }
    }
}

function WhereIAm {
    #number of net adapters can give us a clue where we are.
    $numNic = (get-netadapter | Measure-Object).Count
    Write-Host $(Get-Date) "DEBUG: Checking environment"
    $cnt = (Get-NetIpAddress | Where-Object { $_.PrefixOrigin -eq 'Dhcp' -and $_.AddressFamily -eq 'IPv4' } | Measure-Object).Count
    if ($numNic -eq 2) {
        # check if we are in compute or on test server
        if ($cnt -gt 0) {
            Write-Host "IPv4 found, running in compute"
            $where = 1 #'compute'
            $test_server = 0
        }
        else {
            Write-Host "metadata not found, we are on test server"
            & "C:\Program Files\Mdb\SetupForTestRun.ps1" 1
            $test_server = 1
            exit
        }
    }
    if ($numNic -eq 1) {
        
        if ($cnt -gt 0) {
            Write-Host "IPv4 found, running in control-plane"
            $where = 2 #'control-plane'
            $test_server = 0
        }
        else {
            Write-Host "running on a test server mocking control-plane"
            & "C:\Program Files\Mdb\SetupForTestRun.ps1" 2
            $test_server = 1
        }
        
    }
    return $where, $test_server
}

function ChangeAccountTokenPolicy {
    Write-Host $(Get-Date) "DEBUG: Changing account token policy"
    New-ItemProperty -Path HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\System -Name LocalAccountTokenFilterPolicy -Value 1
}

function SetNICAlias {
    param (
        [bool]$noIPv6,
        [int]$where
    )
    if ($where -eq 1) {
        Write-Host $(Get-Date) "DEBUG: Set network interface aliaces"
        $intInterfaceId = Get-NetIpAddress -PrefixOrigin Dhcp -AddressFamily IPv6 | Select-Object -ExpandProperty InterfaceIndex
        Get-NetAdapter -ifIndex $intInterfaceId | Rename-NetAdapter -NewName eth1 -Confirm:$false
        $extInterfaceId = Get-NetIpAddress -PrefixOrigin Dhcp -AddressFamily IPv4 | Where-Object { $_.InterfaceIndex -notin $intInterfaceId } | Select-Object -ExpandProperty InterfaceIndex
        Get-NetAdapter -ifIndex $extInterfaceId | Rename-NetAdapter -NewName eth0 -Confirm:$false
    }
    else {
        Get-NetAdapter | Select-Object -First 1 | Rename-NetAdapter -NewName eth1 -Confirm:$false
    }

}

function ConfigureNetwork {
    param(
        [bool]$noIPv6,
        [int]$where
    )
    SetNICAlias $noIPv6 $where
    if ($where -eq 1) {
        # get external gateway and dns servers
        Write-Host $(Get-Date) "DEBUG: Get external Gateway and DNS"
        $extGw = Get-NetIPConfiguration -InterfaceAlias eth0 | Select-Object -ExpandProperty IPv4DefaultGateway
        $extDns = Get-NetIPConfiguration -InterfaceAlias eth0 | Select-Object -ExpandProperty DNSServer | Select-Object -ExpandProperty ServerAddresses
        
        # set proper route to 169.254.169.254
        Write-Host $(Get-Date) "DEBUG: Set proper route to metadata server"
        try {
            Find-NetRoute -RemoteIPAddress 169.254.169.254 | Where-Object { $_.NextHop -eq '0.0.0.0' } | Remove-NetRoute -Confirm:$false
        }
        catch {}
        New-NetRoute -DestinationPrefix 169.254.169.254/32 -InterfaceAlias eth0 -NextHop $extGw.NextHop
        
        # setup DNS
        Write-Host $(Get-Date) "DEBUG: Setup DNS"
        Add-DnsClientNrptRule -NameServers @("2a02:6b8::1:1", "2a02:6b8:0:3400::1") -Namespace "."
        Add-DnsClientNrptRule -NameServers @("2a02:6b8::1:1", "2a02:6b8:0:3400::1") -Namespace ".ydb.mdb.cloud-preprod.yandex.net"
        Add-DnsClientNrptRule -NameServers @("2a02:6b8::1:1", "2a02:6b8:0:3400::1") -Namespace ".ydb.mdb.yandexcloud.net"
        Add-DnsClientNrptRule -NameServers $extDns -Namespace ".mdb.cloud-preprod.yandex.net"
        Add-DnsClientNrptRule -NameServers $extDns -Namespace ".mdb.yandexcloud.net"
        Add-DnsClientNrptRule -NameServers $extDns -Namespace "168.192.in-addr.arpa"
        Add-DnsClientNrptRule -NameServers $extDns -Namespace "172.in-addr.arpa"
        Add-DnsClientNrptRule -NameServers $extDns -Namespace "10.in-addr.arpa"
        Set-DnsClientNrptGlobal -QueryPolicy "QueryBoth"
        if ($noIPv6) {
            Write-Host $(Get-Date) "DEBUG: Disable ipv6 on eth0"
            Disable-NetAdapterBinding -Name eth0 -ComponentID ms_tcpip6 -PassThru
        }

        Write-Host $(Get-Date) "DEBUG: Disable ipv4 on eth1"
        Disable-NetAdapterBinding -Name eth1 -ComponentID ms_tcpip -PassThru
    }
    if ($where -eq 2) {
        
    }
}

function SetDefaultFirewallRules {
    param(
        [int]$where
    )
    if ($where -eq 1) {
        New-NetFirewallRule -DisplayName mdb_intranet_tcp80_out -RemotePort 80 -Protocol tcp -RemoteAddress any -InterfaceAlias eth1 -Direction Outbound -Action Allow
        New-NetFirewallRule -DisplayName mdb_intranet_tcp443_out -RemotePort 443 -Protocol tcp -RemoteAddress any -InterfaceAlias eth1 -Direction Outbound -Action Allow
        New-NetFirewallRule -DisplayName mdb_ssh_tcp_out -RemotePort "22" -Protocol tcp -RemoteAddress any -InterfaceAlias eth0 -Direction Outbound -Action Allow
        New-NetFirewallRule -DisplayName mdb_intranet_tcp8443_out -RemotePort 8443 -Protocol tcp -RemoteAddress any -InterfaceAlias eth1 -Direction Outbound -Action Allow    
    }

}


function GetMetadata($path) {
    $uri = "http://169.254.169.254/latest/$path/"
    Invoke-WebRequest -Uri $uri -TimeoutSec:10 -OutFile "$ENV:TEMP\metadata.out" -UseBasicParsing
    $content = Get-Content -Path "$ENV:TEMP\metadata.out" -Raw
    Remove-Item -Path "$ENV:TEMP\metadata.out"
    return $content
}

function SetHostName {
    param (
        $metadata
    )
    # get hostname from metadata
    Write-Host $(Get-Date) "DEBUG: Get compute metadata"
    $fqdn = GetMetadata("meta-data/hostname")
    Write-Host "Metadata found, hostname: $fqdn"

    # set hostname
    Write-Host $(Get-Date) "DEBUG: Set hostname"
    $hostname = $fqdn.split(".", 2)[0]
    $domain = $fqdn.split(".", 2)[1]
    #cut off last 15 symbols of the name as it is not recommended to have longer hostnames in windows
    if ($hostname.length -gt 15) {
        $short_hn = $hostname.substring($hostname.length - 15)
        while ($short_hn[0] -notmatch "^[a-z0-9]") {
            $short_hn = $short_hn.substring(1)
        }
        $hostname = $short_hn
    }
    Write-Host "hostname: $hostname domain: $domain"
    Rename-Computer $hostname -Force

        
    Write-Host $(Get-Date) "DEBUG: Set workgroup name"
    Add-computer -WorkgroupName "db.yandex.net"

    Write-Host $(Get-Date) "DEBUG: Set tcpip hostname and domain"
    Set-ItemProperty -Path HKLM:\system\CurrentControlSet\Services\tcpip\parameters -Name "Hostname" -Value $hostname
    Set-ItemProperty -Path HKLM:\system\CurrentControlSet\Services\tcpip\parameters -Name "NV Hostname" -Value $hostname
    Set-ItemProperty -Path HKLM:\system\CurrentControlSet\Services\tcpip\parameters -Name "Domain" -Value $domain
    Set-ItemProperty -Path HKLM:\system\CurrentControlSet\Services\tcpip\parameters -Name "NV Domain" -Value $domain

}

function SetMinionConfig {
    param(
        $metadata
    )
    
    Write-Host $(Get-Date) "DEBUG: Getting user-data metadata"
    $fqdn = GetMetadata("meta-data/hostname")
    $userdata = GetMetadata("user-data")
    $userdata = ConvertFrom-Yaml $userdata
    Write-Host ("Setting minion configuration: version: " + $userdata.deploy_version + " uri: " + $userdata.mdb_deploy_api_host)
    Set-Content -Path "C:\salt\conf\minion_id" -Value $fqdn
    Set-Content -Path "C:\salt\conf\hostname" -Value $fqdn
    Set-Content -Path "C:\salt\conf\deploy_version" -Value $userdata.deploy_version
    Set-Content -Path "C:\salt\conf\mdb_deploy_api_host" -Value $userdata.mdb_deploy_api_host

}

function GetAdminPassword {
    # updating Administrator password
    Write-Host $(Get-Date) "DEBUG: Getting Administrator's password"
    $password = $null
    $attempt = 0
    while ($password -eq $null -and $attempt -lt 300) {
        try {
            $result = salt-call pillar.get --out=newline_values_only --return="" "data:windows:users:Administrator:password"
            if ($LASTEXITCODE -eq 0 -and $result -ne "") {
                $password = $result.Trim()
            }
            else {
                throw "salt-call error: exicode $LASTEXITCODE, ret:$result"
            }
        }
        catch {
            Write-Host "Failed to get root password: $_, sleeping.."
            Start-Sleep -s 10
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

StopSaltMinion
$where, $test_server = WhereIAm
if ($test_server -eq 0) {
    ChangeAccountTokenPolicy
    ConfigureNetwork $True $where
    SetDefaultFirewallRules $where
    $metadata = GetMetadata
    SetHostName $metadata
    SetMinionConfig $metadata
    $password = GetAdminPassword
    $pw = ConvertTo-SecureString -AsPlainText -Force $password
    ChangeUserPassword 'Administrator' $pw
    SetSaltStartupAccount 'Administrator' $password
    Write-Host $(Get-Date) "DEBUG: Set salt-minion startup type"
    Set-Service -Name salt-minion -StartupType Automatic

    if ($where -eq 1) {
        Set-NetFirewallProfile -Name Public -DefaultOutboundAction Block
        Set-NetFirewallProfile -Name Private -DefaultOutboundAction Block
    }
    if ($where -eq 1) {
        Get-NetFirewallRule | Where-Object { $_.DisplayName -notlike 'mdb_*' -and $_.DisplayName -notlike "OpenSSH Server (sshd)" -and $_.Enabled -eq 'true' } | disable-netfirewallrule
    }
    else {
        Get-NetFirewallRule | Where-Object { $_.DisplayName -notlike 'mdb_*' -and $_.DisplayName -notlike "OpenSSH Server (sshd)" -and $_.Enabled -eq 'true' -and $_.Direction -like 'Inbound' } | disable-netfirewallrule
    }
}
