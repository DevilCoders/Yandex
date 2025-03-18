#!/usr/bin/env python2.7

import sys
import requests

# import urllib3
# urllib3.disable_warnings()

url = "https://metrics-calculation.qe.yandex-team.ru/api/qex/json/9404954"
print sys.version_info
print "Downloading serpset"
resp = requests.get(url, verify=False)
assert resp.status_code == 200
print "OK"
