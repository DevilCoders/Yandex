#!/usr/bin/env python2

from subprocess import *
import re
import os

# These elements light critical monitoring for corresponding shelves
force_crit = {
 "all":      ["Temperature sensor"],
 "4U60swap": ["Power supply"]
}

crit_temperatures = {
        "Rack 2U30 Exp": 62,
        "HBA": 92
}

fp = open(os.devnull, 'w')
sgmap = Popen("find_shelves", shell=True, stdout=PIPE, stderr=fp).stdout
shelves = sgmap.read().splitlines()
criticals = []
warns = []

hbas = int(Popen("lsiutil -b | grep 'MPT Port' | awk '{print $1}'", shell=True, stdout=PIPE).stdout.readline())
for h in range(hbas):
    hba_temp = int(Popen("lsiutil -p {} -a 25,2,0,0 | grep IOCTemperature: | awk '{{print $NF}}'".format(h+1), shell=True, stdout=PIPE).stdout.readline(),16)
    if hba_temp != 0:
        if hba_temp == crit_temperatures["HBA"]:
            warns.append("HBA {} temperature: {}".format(h,hba_temp))
        elif hba_temp > crit_temperatures["HBA"]:
            criticals.append("HBA {} overheat: {}".format(h,hba_temp))

for s in shelves:
    sgses = Popen("sg_ses -p 0x2 {0}".format(s), shell=True, stdout=PIPE, stderr=fp).stdout
    elem = ""
    head = sgses.readline()
    crit_elements = set()
    for k in force_crit.keys():
        if k == "all" or k in head:
            crit_elements.update(force_crit[k])

    for line in sgses:
        ret = re.search("Element type: ([^,]+)", line)
        if ret:
            elem = ret.group(1)
            continue
        ret = re.search("status: (critical|noncritical)", line, re.I)
        if ret:
            status = ret.group(1)
            if status.lower() == "noncritical" or elem not in crit_elements:
                warns.append(elem)
            else:
                criticals.append(elem)
            continue
        ret = re.search("Temperature=(\d+)", line)
        if ret:
            temperature = int(ret.group(1))
            for shelf_name in crit_temperatures:
                if shelf_name in head:
                    if temperature > crit_temperatures[shelf_name]:
                        criticals.append("{} temperature {} is too high".format(s, temperature))
                    elif temperature == crit_temperatures[shelf_name]:
                        warns.append("{} temperature {} is too high".format(s, temperature))

fp.close()

result = "OK"
code = 0

if len(warns) > 0:
    code = 1
    result = "warn: " + ", ".join(warns)


if len(criticals) > 0:
    result2 = "crit: " + ", ".join(criticals)
    if code == 0:
        result = result2
    else:
        result = result2 +"; " + result
    code = 2

print "{0};{1}".format(code,result),

