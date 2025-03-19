<#
Selfdns-client plugin for ipv6only network like Control plane
Can be used on windows only
Author: @trootnev
#>

$ErrorActionPreference = "Stop"

# have to be defined in each plugin
$plugin_vers = "0.1-mdb"

# host, which will be used below to determine default route src addresses
$route_host = "dns-api.yandex.net"

#get fqdn from minion config as it is a place where it comes first
#first need to make sure the file is in place
if (-not(Test-Path -Path C:\salt\conf\hostname)) {
    throw ("FQDN not found in C:\salt\conf\hostname")
}

$local_hostname = (Get-Content C:\Salt\conf\hostname).Trim()

#need to make sure that we have an FQDN in this file.
#we can basically count dots.
if (($local_hostname.split('.')).Count -lt 3) {
    throw ("Localhost name doesn't look like FQDN.")
}

#get and IP adress of the remote host that will be used
#to determine which local IP address to report
$ipv6_host = (Resolve-DnsName $route_host -Type AAAA).IpAddress

if (-not ($ipv6_host)) {
    throw ("Seems like name resolution is broken")
}
else {
    $ipv6_address = (Find-NetRoute -RemoteIPAddress $ipv6_host).IpAddress
}
if (-not $ipv6_address) {
    throw ("Can't find route to $route_host")
}
else {
    #this is a successul operation's exit point
    Write-Output "$plugin_vers $local_hostname $ipv6_address"
}
