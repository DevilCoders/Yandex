class InterfaceAction(object):
    def __init__(self):
        self._ip_version = None
        self._comment = None

    def comment(self, comm):
        self._comment = comm
        return self

    def get_comment(self):
        return self._comment

    def ip_version(self, ip_version):
        self._ip_version = ip_version
        return self

    def ip4(self):
        self.ip_version(4)
        return self

    def ip6(self):
        self.ip_version(6)
        return self

    def get_sbin_ip(self):
        if self._ip_version == 6:
            return "/sbin/ip -6"
        else:
            return "/sbin/ip"

    def get_sbin_iptables(self):
        if self._ip_version == 6:
            return "/sbin/ip6tables"
        else:
            return "/sbin/iptables"
