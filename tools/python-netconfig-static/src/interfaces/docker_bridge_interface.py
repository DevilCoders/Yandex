
from interface import Interface


class DockerBridgeInterface(Interface):
    def __init__(self, *args, **kwargs):
        super(DockerBridgeInterface, self).__init__(*args, **kwargs)

    def write(self, out, format):
        out.write("auto " + self.name + "\n")
        out.write("iface " + self.name + " inet static\n")
        out.write("\taddress 172.17.42.1\n")
        out.write("\tnetmask 16\n")
        out.write("\tbridge_ports none\n")
        out.write("\tbridge_stp off\n")
        out.write("\tbridge_fd 0\n")
        out.write("iface " + self.name + " inet6 static\n")
        out.write("\taddress fdfe:dead:beef:cafe::1\n")
        out.write("\tnetmask 64\n")
        out.write("\tpost-up /usr/sbin/service radvd restart\n")
