#!/usr/bin/env python
import requests
import sys
import json

prefix = "/etc/cocaine/zk-dc-conf."
curl = "http://c.yandex-team.ru/api-cached/groups2hosts/" + sys.argv[1] + "?format=json&fields=root_datacenter_name,fqdn"
try:
    r = requests.get(curl)
except:
    print "2;problem with conductor"
    exit(1)
zoo = json.loads(r.text)
zk_dc = {}

for i in zoo:
    try:
        zk_dc[i["root_datacenter_name"]] = zk_dc[i["root_datacenter_name"]] + ",\n{\"host\":\"" + i["fqdn"] + "\", \"port\": 2181}"
    except:
        zk_dc[i["root_datacenter_name"]] = "{\"host\":\"" + i["fqdn"] + "\", \"port\": 2181}"

for dc in zk_dc:
    f = open(prefix + dc, 'w')
    f.write(zk_dc[dc])
    f.close
    print "Create " + prefix + dc + " config:"
    print zk_dc[dc] + "\n"
