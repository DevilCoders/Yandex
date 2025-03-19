#! /usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import argparse
import socket
import urllib
import re
import subprocess

def createParser ():
    parser = argparse.ArgumentParser()
    parser.add_argument ('--balancer', '-b', required=True)
    parser.add_argument ('--port', '-p', default='80')

    return parser

parser = createParser()
namespace = parser.parse_args(sys.argv[1:])

if namespace.balancer:
	balancer = namespace.balancer
	b_port = namespace.port

hostname = socket.gethostname()
my_ipv4 = socket.gethostbyname(hostname)

def get_b_ip(balancer):
    ips = socket.getaddrinfo(balancer, 80, 0, 0, socket.SOL_TCP)
    address = []
    for i in ips:
        address.append(i[4][0])

    return address

# Get vip from racktables
def get_vip(ip, port):
	c = urllib.urlopen('https://racktables.yandex.net/export/vip_on_balancers.php?vip=' + ip + '&vport=' + port).read()
	array = c.split('\n')
	if array == ['']:
		print "2; Not found balancers in racktables"
		raise SystemExit(1)

	for line in array:
		p = re.compile(hostname + "\((.*)/.*\).*")
		aa = p.search(line)
		if aa:
			bb, cc = aa.span()
			r = re.compile (r'[\(/\)]')
			array_vip = r.split(line[1:cc])
			vip = array_vip[1]
			
			return vip

b_ips = get_b_ip(balancer)

for ip in b_ips:
	vip = get_vip(ip, b_port)
	check_int = subprocess.call("ip a sh | grep %s 2>&1 >/dev/null" % vip, shell=True)
	if check_int != 0:
		print "2; Balancer ip not configured"
		raise SystemExit(1)

print "0; Balancer ip's UP"
