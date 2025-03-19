#!/usr/bin/python

import json
import commands
import socket

l3path_place = "/usr/lib/yandex-netconfig/patches/srw_dyn.json"

path = {
    "data": {}
}

dc = {
    "f": "MYT",
    "h": "SAS",
    "e": "IVA",
    "i": "MAN",
    "a": "VLA"
}

ifname = "eth0"
nvlan = 639
comments = "SRWMTN"
tvlan = "688"

cmd = "/usr/bin/timeout -s KILL 5 /sbin/ip -6 -o addr list %s | /bin/grep global | /usr/bin/head -n1" % ifname
addr = commands.getoutput(cmd).split()[3]
#print addr
hostname = socket.gethostname()
dc_mark = hostname.split(".")[0][-1]

path["data"][addr] = {
    "vlan": nvlan,
    "datacenter_name": dc[dc_mark],
    "comments": comments,
    "backbone_routes": "bb_default",
    "backbone_vlans": {
        tvlan: {
            "routes": "bb_default",
            "mtn": 1,
            "host64": 1
        }
    }
}

with open(l3path_place, 'w') as f:
    json.dump(path, f, sort_keys = True, indent = 4, ensure_ascii = False)
#    f.write('\n')


