#!/usr/bin/env python
import requests
import json
warning_percent = float('50.0')
fail_percent = float('70.0')
fail = '2;'
warning = '1;'
curl = "http://localhost:8080/v1/status/echo"
try:
    r = requests.get(curl,timeout=10)
except:
    print "1;problem with /v1/status/echo"
    exit(0)
leadership = json.loads(r.text)['leadership']
code = 0
workers = json.loads(r.text)['workers']
for l in leadership:
    if "offload" in l:
        try:
            if leadership[l] and workers['clusters'][l]['acknowledged'] == 0:
                code = 2
        except:
            continue
if code == 2:
    print '2;0 workers on echo app'
else:
    print '0;Ok'

