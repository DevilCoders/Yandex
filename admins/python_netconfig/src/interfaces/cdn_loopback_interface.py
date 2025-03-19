from interface import Interface


class CdnLoopbackInterface(Interface):
    def __init__(self, *args, **kwargs):
        super(CdnLoopbackInterface, self).__init__(*args, **kwargs)
        self.netmask = '255.255.255.255'


#  def write(self, out, format):
#    for comment in self.comments:
#      out.write('# ' + comment + "\n")
#      out.write('iface ' + self.name + " inet static\n")
#      if self.address:
#        out.write("\taddress " + self.address + "\n")
#      if self.netmask:
#        out.write("\tnetmask " + self.netmask() + "\n")
