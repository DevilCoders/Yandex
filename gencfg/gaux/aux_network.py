# coding: utf8
import netaddr


def get_first_network_address(net_mask):
    network = netaddr.IPNetwork(net_mask)
    return network.iter_hosts().next().format(netaddr.ipv6_full)
