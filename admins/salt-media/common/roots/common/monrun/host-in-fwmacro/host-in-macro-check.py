#!/usr/bin/env python
# -*- coding: utf-8 -*-

import json, os, sys, socket
import requests, ipaddr
from yaml import load

RO_ADMIN = "http://ro.admin.yandex-team.ru/api/firewall/expand_macro.sbml?macro={0}"
HBF_API = "http://hbf.yandex.net/macros/{0}"
DEF_CONF = "/etc/monitoring/host-in-macro-check.yaml"

class Macro():
    def __init__(self):
        self.hosts = set()
        self.networks = []

    def check_host(self, host):
        return host in self.hosts

class CheckMacro():

    def __init__(self):
        self._warn = []
        self.conf = {}
        self.conf_path = DEF_CONF

	self.macros = {}
        self.pid_parts = []

        try:
            if len(sys.argv) > 2:
                self.conf_path = sys.argv[2]
            if os.path.exists(self.conf_path):
                self.conf = load(open(self.conf_path))
        except:
            e = sys.exc_info()
            self.warn("1;Exception {0[0].__name__}: {0[1]}!", e)

        self.names = self.conf.get('macro')
        self.api_type = self.conf.get('api_type')

        if not self.names:
            print("0;OK, Not configured!")
            sys.exit(0)

	if isinstance(self.names, basestring):
            self.names = [self.names]

	self.name = ', '.join(self.names)

        if self.api_type == 'HBF':
            self.api = self.conf.get("api", HBF_API)
        else:
            self.api = self.conf.get("api", RO_ADMIN)

        for macro in self.names:
            self.expand_macro(macro)

    def expand_macro(self,name):
	url = self.api.format(name)
        resp = requests.get(url)
        if resp.status_code == requests.codes.okay:
            if self.api_type == 'HBF':
                try:
                    macro = set(json.loads(resp.text))
                except Exception as e:
                    self.warn("1; hbf API answer cant parsed, ERR: {0}!" % e)
            else:
                if resp.text.startswith('ok'):
                    macro = set(resp.text[2:].split())
                    if 'or' in macro:
                        macro.remove('or')
                else:
                    self.warn("1;ro.admin ERR: {0}!", resp.text.strip())
        else:
            self.warn("1;ro.admin ERR: {0.status_code} - {0.reason}!", resp)

	self.macros[name] = Macro()

        for item in macro:
            if '/' in item:
                if '@' in item:
                    pid = item.split("@")[0]
                    l = len(pid)
                    if l > 4:
                        pid_part = pid[:(l-4)] + ":" + pid[(l-4):]
                        pid = pid_part
                    # now make data for simplify selfcheck that our pid part exist in address
                    self.pid_parts.append(pid)
                else:
                    self.macros[name].networks.append(ipaddr.IPNetwork(item))
            else:
                self.macros[name].hosts.add(item)

    def check_host(self, host):
        return any((macro.check_host(host) for macro in self.macros.itervalues()))

    def contains(self, item):
        return any((self.contains_one(item, macro) for macro in self.macros.itervalues()))

        #for macro in self.macros.values():
        #    if self.contains_one(item, macro):
        #        return True
        #return False

    def contains_one(self, item, macro):
        if item in macro.hosts:
            return True

        try:
            ip = ipaddr.IPAddress(item.split('/')[0])
        except:
            e = sys.exc_info()
            w = "{0[0].__name__}: {0[1]}!".format(e)
            self._warn.append(w)
            return False

        for net in macro.networks:
            if ip in net:
                return True
        for pid_part in self.pid_parts:
            if pid_part in str(ip):
                return True
        return False

    def err(self, msg):
        sys.exit(1)

    def warn(self, msg, data):
        print(msg.format(data))
        sys.exit(1)


def get_dns_ips():
    "Резолвит hostname и возвращает полученные ip адреса"
    ips = set([i[4][0] for i in socket.getaddrinfo(socket.getfqdn(), None)])
    return ips


# from subprocess import Popen, PIPE
# ips = Popen("ip -o a s scope global".split(), stderr=PIPE, stdout=PIPE)

if __name__ == '__main__':
    ok_ips = []
    hostname = socket.getfqdn()

    macro = CheckMacro()
    my_ips = get_dns_ips()
    for ip in my_ips:
        if macro.contains(ip):
            ok_ips.append(ip)

    not_ok_ips = my_ips - set(ok_ips)
    if macro.check_host(hostname):
        ok_ips = [ hostname ]  # hostname для краткости
        not_ok_ips = ()  # если hostname входит в макрос - это можно затереть

    if ok_ips:
        if not not_ok_ips:
            msg = "0;{0} in {1}".format(','.join(ok_ips), macro.name)
            if macro._warn:
                msg = "1;{0}{1}".format(','.join(macro._warn), msg)
        else:
            msg = "2;Host not belong fw macro {0} by {1}".format(macro.name,
                                                                 ",".join(not_ok_ips))
    else:
        msg = "2;Host not belong fw macro {0}".format(macro.name)

    print msg

