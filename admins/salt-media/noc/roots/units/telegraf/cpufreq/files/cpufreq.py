#!/usr/bin/python3
import re
import json
cpuinfo = open('/proc/cpuinfo').read()
dataplane = open('/etc/yanet/dataplane.conf').read()
ports = json.loads(dataplane).get('ports', [])
pin_ports = []
for p in ports:
    pin_ports += p.get('coreIds', [])
ids = re.findall(r'processor\s*:\s(\d+)', cpuinfo)
mhz = re.findall(r'cpu MHz\s*:\s(\d+)', cpuinfo)
cpu_mhz = {idx: mhz for idx, mhz in zip(ids, mhz)}
res = []
for p in cpu_mhz:
    if int(p) in pin_ports:
        res.append('cpuid_pin_{}={}'.format(p, cpu_mhz[p]))
    else:
        res.append('cpuid_notpin_{}={}'.format(p, cpu_mhz[p]))
print('cpufreq {}'.format(','.join(res)))
