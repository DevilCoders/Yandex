#!/usr/bin/env python

import os
import sys
import socket


# Script can be called from any directory
try:
    sys.path.append(os.path.dirname(__file__))
except:
    pass

import interfaces

from optparse import OptionParser
from interfaces.generation_parameters import GenerationParameters
from interfaces.network_info import NetworkInfo
from interfaces.generators.interfaces_generator import InterfacesGenerator
from interfaces.conductor import Conductor
from interfaces.utils.local import get_first_uplink_name


CONDUCTOR_ENV = 'production'


def build_parser():
    parser = OptionParser()
    parser.add_option("", "--fqdn", dest="fqdn",
                      help="manual FQDN declaration", type="str", default=socket.getfqdn())
    parser.add_option("-o", "--output", dest="output",
                      help="save output to <filename>", type="str", default=None)
    parser.add_option("-j", "--no-jumbo", dest="nojumbo", help="force switch off jumboframes", action="store_true",
                      default=False)
    parser.add_option("-m", "--mtu", dest="mtu", help="set mtu. Default is 8950 (or 1450 if --no-jumbo is set)",
                      default=0, type="int")
    parser.add_option("-i", "--iface", dest="iface", help="default interface name. eth0 if none is given",
                      default="eth0",
                      type="str")
    parser.add_option("-B", "--balancers", dest="do10", help="generate 10x interfaces for balancers", default=None,
                      action="store_true")
    parser.add_option("-b", "--bridged", dest="bridged", help="bridged interface generation", default=None,
                      action="store_true")
    parser.add_option("-v", "--virtual", dest="virtual",
                      help="set host as virtual", default=None, action="store_true")
    parser.add_option("-q", "--multiqueue", dest="multiqueue",
                      help="host is an openstack guest with multiqueue VirtIO-net enabled. Implies -v", default=None,
                      action="store_true")
    parser.add_option("-f", "--fastbone", dest="fastbone",
                      help="fastbone interface name.", default=None, type="str")
    parser.add_option("-M", "--no-antimartians", dest="noantimartians",
                      help="force switch off antimartian dummies generation", action="store_true", default=None)
    parser.add_option("", "--narod-misc-dom0", dest="narodmiscdom0", help="narod misc dom0 interface", type="str",
                      default=None)
    parser.add_option("-l", "--no-lo", dest="nolo", help="no loopback interface generation", action="store_true",
                      default=False)
    parser.add_option("", "--bond-interfaces", dest="bonding",
                      help="bond interfaces", type="str", default=None)
    parser.add_option("", "--format", dest="output_format",
                      help="output format for network configuration. Available options: ubuntu (default), redhat",
                      type="str", default="ubuntu")
    parser.add_option("", "--scan", dest="scan",
                      help="scan link interface", action="store_true")
    parser.add_option("", "--debug", dest="debug",
                      help="enable debug messages", action="store_true")
    parser.add_option("", "--plugin-dir", dest="plugin_dir", help="specify plugin directory", type="str",
                      default="/etc/netconfig.d")
    parser.add_option("", "--host-data", dest="host_data",
                      type="str", default=None)
    parser.add_option("", "--dns-data", dest="dns_data",
                      type="str", default=None)
    parser.add_option("", "--use-tables", dest="use_tables", help="use routing tables for fastbone interfaces",
                      default=False, action="store_true")
    return parser


def generate(options):
    conductor = Conductor(CONDUCTOR_ENV, options.host_data)
    host_info = conductor.host_info(options.fqdn)
    if interfaces.DEBUG:
        print "Host information from conductor for %s" % options.fqdn
        print repr(host_info)
        print "--------------------------"
    if options.scan:
        uplink = get_first_uplink_name()
        if uplink:
            options.iface = uplink
    params = GenerationParameters(options, host_info)
    generator = InterfacesGenerator(params, options.plugin_dir)
    network_info = NetworkInfo(conductor, options.dns_data)
    interfaces.output.format = options.output_format
    bridges = []
    ifaces = generator.generate(network_info)
    for i in ifaces:
        if i.is_bridge() and i.bridge_ports in bridges:
            i.render_bridge_params = False
        i.generate_actions()
        if i.is_bridge():
            bridges.append(i.bridge_ports)
    return ifaces
