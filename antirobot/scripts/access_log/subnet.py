#from SubnetTree import SubnetTree
#
#def subnet(*args):
#    st = SubnetTree()
#    for net in args:
#        st.insert(net)
#    return st

from .ipaddr import IPAddress, IPNetwork
from itertools import dropwhile

class subnet(object):
    def __init__(self, *args):
        self.networks = map(IPNetwork, args)

    def __contains__(self, ip):
        ip = IPAddress(ip)
        nets = iter(self.networks)
        try:
            next(dropwhile(lambda net: ip not in net, nets))
        except StopIteration:
            return False
        return True
