#!/usr/bin/env python

from datetime import datetime
from time import time
import sys
import json

try:
    F = open("/etc/qloud/meta.json","r")
    json_qloud = F.read()
    start_time = json.loads(json_qloud)["app_start_time"].split(".")[0]
    date_object = datetime.strptime(start_time.split(".")[0],'%Y-%m-%dT%H:%M:%S')
    current_time = time()
    uptime = int(time()) - int(date_object.strftime("%s"))
except:
    f = open("/proc/uptime", "r")
    uptime = f.readline().split(".")[0]

print uptime

