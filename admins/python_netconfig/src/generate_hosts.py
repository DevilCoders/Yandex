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
import interfaces.generators.hadoop_generator

from optparse import OptionParser
from interfaces.conductor import Conductor
from interfaces.output import write_all

CONDUCTOR_ENV = 'production'

parser = OptionParser()
parser.add_option("", "--fqdn", dest="fqdn", help="manual FQDN declaration", type="str", default=socket.getfqdn())
parser.add_option("-o", "--output", dest="output", help="save output to <filename>", type="str", default=None)
parser.add_option("", "--debug", dest="debug", help="enable debug messages", action="store_true")

(options, args) = parser.parse_args()

try:
    if options.debug:
        interfaces.DEBUG = True
    conductor = Conductor(CONDUCTOR_ENV)
    host_group = conductor.host2group(options.fqdn)
    if host_group is None:
        raise Exception('Host not found in conductor')
    hosts = conductor.groups2hosts(host_group)
    result = interfaces.generators.hadoop_generator.generate_hosts(hosts)
    if interfaces.DEBUG:
        print "Host information from conductor for %s" % options.fqdn
        # print repr(host_info)
        print "--------------------------"
    write_all(options.output, result)
except:
    raise
#   sys.stderr.write("ERROR: Conductor knows nothing about host '" + options.fqdn + "'.\n  Try to use --fqdn option to set hostname manually or check your server really exists in conductor db.\n")
#   exit(1)
