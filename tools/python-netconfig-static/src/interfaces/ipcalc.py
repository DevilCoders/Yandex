
from ipaddr import IPAddress, IPNetwork


def is_in_net(ip, net):
    net = IPNetwork(net)
    ip = IPAddress(ip)
    return net.Contains(ip)
