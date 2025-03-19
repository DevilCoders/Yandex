Write-Output $(Get-Date) "DEBUG: Starting post-restart adapters check" | Out-File "C:\Logs\ClearNICs.log" -Append 
$Devs = Get-PnpDevice -Class net | Where-Object Status -EQ Unknown | Select-Object FriendlyName, InstanceId

if ($Devs) {
        ForEach ($Dev in $Devs) {
                Write-Output "Removing $($Dev.FriendlyName)" | Out-File "C:\Logs\ClearNICs.log" -Append 
                $RemoveKey = "HKLM:\SYSTEM\CurrentControlSet\Enum\$($Dev.InstanceId)"
                Get-Item $RemoveKey | Select-Object -ExpandProperty Property | ForEach-Object { Remove-ItemProperty -Path $RemoveKey -Name $_ -Verbose } | Out-File "C:\Logs\ClearNICs.log" -Append 
        }
        Restart-Computer -Force | Out-File "C:\Logs\ClearNICs.log" -Append 
}
else {
        $numnic = (Get-NetIPAddress | Where-Object { $_.PrefixOrigin -eq 'DHCP' -and $_.AddressFamily -eq 'IPv4' } | Measure-Object).Count
        if ($numnic -eq 1) {
                $extInterfaceId = (Get-NetIPAddress -PrefixOrigin Dhcp -AddressFamily IPv4).InterfaceIndex
                $intInterfaceId = (Get-NetAdapter | Where-Object { $_.InterfaceIndex -ne $extInterfaceId }).InterfaceIndex
                # set net interface aliases.  eth1 - internal net
                if (-not (Get-NetAdapter | Where-Object { $_.Name -eq 'eth0' })) {
                        Write-Output $(Get-Date) "DEBUG: Set network interface alias for eth0" | Out-File "C:\Logs\ClearNICs.log" -Append 
                        Get-NetAdapter -ifIndex $extInterfaceId | Rename-NetAdapter -NewName eth0 -Confirm:$false | Out-File "C:\Logs\ClearNICs.log" -Append 
                }
                if (-not (Get-NetAdapter | Where-Object { $_.Name -eq 'eth1' })) {
                        Write-Output $(Get-Date) "DEBUG: Set network interface alias for eth1" | Out-File "C:\Logs\ClearNICs.log" -Append 
                        Get-NetAdapter -ifIndex $intInterfaceId | Rename-NetAdapter -NewName eth1 -Confirm:$false | Out-File "C:\Logs\ClearNICs.log" -Append 
                }
        }
        else {
                GetNetAdapter | Select-Object -First 1 | Rename-NetAdapter -NewName eth1 -Confirm:$false
        }
}
Get-NetFirewallRule sshd | Get-NetFirewallInterfaceFilter | Set-NetFirewallInterfaceFilter -InterfaceAlias eth1
Get-NetFirewallRule -DisplayName mdb_intranet_tcp443_out|Get-NetFirewallInterfaceFilter|Set-NetFirewallInterfaceFilter -InterfaceAlias eth1
Remove-Item  "c:\salt\conf\minion.d\_schedule.conf" -ErrorAction SilentlyContinue