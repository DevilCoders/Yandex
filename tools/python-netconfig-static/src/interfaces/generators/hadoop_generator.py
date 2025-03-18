def internalHDPIP(fqdn):
    facename = fqdn.split('.')[0]
    if facename[0] == 'n':
        return '192.168.100.' + facename[1:]
    else:
        return '192.168.99.0' + facename[1:]


def shortname(fqdn):
    return fqdn.split('.')[0]


def generate_hosts(hostlist):
    result = ""
    for i in hostlist:
        result += internalHDPIP(i) + ' ' + i + ' ' + shortname(i) + "\n"
    result += "127.0.0.1 localhost localhost.localdomain" + "\n"
    result += "::1 localhost localhost.localdomain" + "\n"
    return result
