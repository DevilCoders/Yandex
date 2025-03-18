"""Auxiliarily function for manipulation with ipv4-tunnels (GENCFG-1077)"""


IPV4_NETS = {
    'myt': '178.154.167.0/24',
    'iva': '178.154.167.0/24',
    'fol': '178.154.167.0/24',
    'ugrb': '178.154.167.0/24',
    'sas': '93.158.129.0/24',
    'man1': '87.250.245.0/24',
    'man2': '87.250.245.0/24',
    'man-4_b.1.09': '5.45.225.128/25',
    'vla': '93.158.163.0/24'
}


class THostTunnelInfo(object):
    """Class with host tunnel info"""

    __slots__ = ('ip', 'hostname', 'groupname')

    def __init__(self, ip, groupname, hostname):
        self.ip = ip
        self.hostname = hostname
        self.groupname = groupname
